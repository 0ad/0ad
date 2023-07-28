/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <stack>
#include <algorithm>

#include "maths/MD5.h"
#include "ps/CacheLoader.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"

#include "RelaxNG.h"
#include "Xeromyces.h"

#include <libxml/parser.h>

static std::mutex g_ValidatorCacheLock;
static std::map<const std::string, RelaxNGValidator> g_ValidatorCache;
static bool g_XeromycesStarted = false;

static void errorHandler(void* UNUSED(userData), xmlErrorPtr error)
{
	// Strip a trailing newline
	std::string message = error->message;
	if (message.length() > 0 && message[message.length()-1] == '\n')
		message.erase(message.length()-1);

	LOGERROR("CXeromyces: Parse %s: %s:%d: %s",
		error->level == XML_ERR_WARNING ? "warning" : "error",
		error->file, error->line, message);
	// TODO: The (non-fatal) warnings and errors don't get stored in the XMB,
	// so the caching is less transparent than it should be
}

void CXeromyces::Startup()
{
	ENSURE(!g_XeromycesStarted);
	xmlInitParser();
	xmlSetStructuredErrorFunc(NULL, &errorHandler);
	std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
	g_ValidatorCache.insert(std::make_pair(std::string(), RelaxNGValidator()));
	g_XeromycesStarted = true;
}

void CXeromyces::Terminate()
{
	ENSURE(g_XeromycesStarted);
	g_XeromycesStarted = false;
	ClearSchemaCache();
	std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
	g_ValidatorCache.clear();
	xmlSetStructuredErrorFunc(NULL, NULL);
	xmlCleanupParser();
}

bool CXeromyces::AddValidator(const PIVFS& vfs, const std::string& name, const VfsPath& grammarPath)
{
	ENSURE(g_XeromycesStarted);

	RelaxNGValidator validator;
	if (!validator.LoadGrammarFile(vfs, grammarPath))
	{
		LOGERROR("CXeromyces: failed adding validator for '%s'", grammarPath.string8());
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
		std::map<const std::string, RelaxNGValidator>::iterator it = g_ValidatorCache.find(name);
		if (it != g_ValidatorCache.end())
			g_ValidatorCache.erase(it);
		g_ValidatorCache.insert(std::make_pair(name, validator));
	}
	return true;
}

bool CXeromyces::ValidateEncoded(const std::string& name, const std::string& filename, const std::string& document)
{
	std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
	return GetValidator(name).ValidateEncoded(filename, document);
}

/**
 * NOTE: Callers MUST acquire the g_ValidatorCacheLock before calling this.
 */
RelaxNGValidator& CXeromyces::GetValidator(const std::string& name)
{
	if (g_ValidatorCache.find(name) == g_ValidatorCache.end())
		return g_ValidatorCache.find("")->second;
	return g_ValidatorCache.find(name)->second;
}

PSRETURN CXeromyces::Load(const PIVFS& vfs, const VfsPath& filename, const std::string& validatorName /* = "" */)
{
	ENSURE(g_XeromycesStarted);

	CCacheLoader cacheLoader(vfs, L".xmb");

	MD5 validatorGrammarHash;
	{
		std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
		validatorGrammarHash = GetValidator(validatorName).GetGrammarHash();
	}
	VfsPath xmbPath;
	Status ret = cacheLoader.TryLoadingCached(filename, validatorGrammarHash, XMBStorage::XMBVersion, xmbPath);

	if (ret == INFO::OK)
	{
		// Found a cached XMB - load it
		if (m_Data.ReadFromFile(vfs, xmbPath) && Initialise(m_Data))
			return PSRETURN_OK;
		// If this fails then we'll continue and (re)create the loose cache -
		// this failure legitimately happens due to partially-written XMB files or XMB version upgrades.
		// NB: at this point xmbPath may point to an archived file, we want to write a loose cached version.
		xmbPath = cacheLoader.LooseCachePath(filename, validatorGrammarHash, XMBStorage::XMBVersion);
	}
	else if (ret == INFO::SKIPPED)
	{
		// No cached version was found - we'll need to create it
	}
	else
	{
		ENSURE(ret < 0);

		// No source file or archive cache was found, so we can't load the
		// XML file at all
		LOGERROR("CCacheLoader failed to find archived or source file for: \"%s\"", filename.string8());
		return PSRETURN_Xeromyces_XMLOpenFailed;
	}

	// XMB isn't up to date with the XML, so rebuild it
	return ConvertFile(vfs, filename, xmbPath, validatorName);
}

bool CXeromyces::GenerateCachedXMB(const PIVFS& vfs, const VfsPath& sourcePath, VfsPath& archiveCachePath, const std::string& validatorName /* = "" */)
{
	CCacheLoader cacheLoader(vfs, L".xmb");

	archiveCachePath = cacheLoader.ArchiveCachePath(sourcePath);

	return (ConvertFile(vfs, sourcePath, VfsPath("cache") / archiveCachePath, validatorName) == PSRETURN_OK);
}

PSRETURN CXeromyces::ConvertFile(const PIVFS& vfs, const VfsPath& filename, const VfsPath& xmbPath, const std::string& validatorName)
{
	CVFSFile input;
	if (input.Load(vfs, filename))
	{
		LOGERROR("CXeromyces: Failed to open XML file %s", filename.string8());
		return PSRETURN_Xeromyces_XMLOpenFailed;
	}

	xmlDocPtr doc = xmlReadMemory((const char*)input.GetBuffer(), input.GetBufferSize(), CStrW(filename.string()).ToUTF8().c_str(), NULL,
		XML_PARSE_NONET|XML_PARSE_NOCDATA);
	if (!doc)
	{
		LOGERROR("CXeromyces: Failed to parse XML file %s", filename.string8());
		return PSRETURN_Xeromyces_XMLParseError;
	}

	{
		std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
		RelaxNGValidator& validator = GetValidator(validatorName);
		if (validator.CanValidate() && !validator.ValidateEncoded(doc))
		{
			LOGERROR("CXeromyces: failed to validate XML file %s", filename.string8());
			return PSRETURN_Xeromyces_XMLValidationFailed;
		}
	}

	m_Data.LoadXMLDoc(doc);
	xmlFreeDoc(doc);

	// Save the file to disk, so it can be loaded quickly next time.
	// Don't save if invalid, because we want the syntax error every program start.
	vfs->CreateFile(xmbPath, m_Data.m_Buffer, m_Data.m_Size);

	// Set up the XMBData
	const bool ok = Initialise(m_Data);
	ENSURE(ok);

	return PSRETURN_OK;
}

PSRETURN CXeromyces::LoadString(const char* xml, const std::string& validatorName /* = "" */)
{
	ENSURE(g_XeromycesStarted);

	xmlDocPtr doc = xmlReadMemory(xml, (int)strlen(xml), "(no file)", NULL, XML_PARSE_NONET|XML_PARSE_NOCDATA);
	if (!doc)
	{
		LOGERROR("CXeromyces: Failed to parse XML string");
		return PSRETURN_Xeromyces_XMLParseError;
	}

	{
		std::lock_guard<std::mutex> lock(g_ValidatorCacheLock);
		RelaxNGValidator& validator = GetValidator(validatorName);
		if (validator.CanValidate() && !validator.ValidateEncoded(doc))
		{
			LOGERROR("CXeromyces: failed to validate XML string");
			return PSRETURN_Xeromyces_XMLValidationFailed;
		}
	}

	m_Data.LoadXMLDoc(doc);
	xmlFreeDoc(doc);

	// Set up the XMBData
	const bool ok = Initialise(m_Data);
	ENSURE(ok);

	return PSRETURN_OK;
}
