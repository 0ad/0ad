/* Copyright (C) 2019 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include <vector>
#include <set>
#include <map>
#include <stack>
#include <algorithm>

#include "maths/MD5.h"
#include "ps/CacheLoader.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "RelaxNG.h"
#include "Xeromyces.h"

#include <libxml/parser.h>

static CMutex g_ValidatorCacheLock;
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
	CScopeLock lock(g_ValidatorCacheLock);
	g_ValidatorCache.insert(std::make_pair(std::string(), RelaxNGValidator()));
	g_XeromycesStarted = true;
}

void CXeromyces::Terminate()
{
	ENSURE(g_XeromycesStarted);
	g_XeromycesStarted = false;
	ClearSchemaCache();
	CScopeLock lock(g_ValidatorCacheLock);
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
		CScopeLock lock(g_ValidatorCacheLock);
		std::map<const std::string, RelaxNGValidator>::iterator it = g_ValidatorCache.find(name);
		if (it != g_ValidatorCache.end())
			g_ValidatorCache.erase(it);
		g_ValidatorCache.insert(std::make_pair(name, validator));
	}
	return true;
}

bool CXeromyces::ValidateEncoded(const std::string& name, const std::wstring& filename, const std::string& document)
{
	CScopeLock lock(g_ValidatorCacheLock);
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
		CScopeLock lock(g_ValidatorCacheLock);
		validatorGrammarHash = GetValidator(validatorName).GetGrammarHash();
	}
	VfsPath xmbPath;
	Status ret = cacheLoader.TryLoadingCached(filename, validatorGrammarHash, XMBVersion, xmbPath);

	if (ret == INFO::OK)
	{
		// Found a cached XMB - load it
		if (ReadXMBFile(vfs, xmbPath))
			return PSRETURN_OK;
		// If this fails then we'll continue and (re)create the loose cache -
		// this failure legitimately happens due to partially-written XMB files.
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
		CScopeLock lock(g_ValidatorCacheLock);
		RelaxNGValidator& validator = GetValidator(validatorName);
		if (validator.CanValidate() && !validator.ValidateEncoded(doc))
			// For now, log the error and continue, in the future we might fail
			LOGERROR("CXeromyces: failed to validate XML file %s", filename.string8());
	}

	WriteBuffer writeBuffer;
	CreateXMB(doc, writeBuffer);

	xmlFreeDoc(doc);

	// Save the file to disk, so it can be loaded quickly next time
	vfs->CreateFile(xmbPath, writeBuffer.Data(), writeBuffer.Size());

	m_XMBBuffer = writeBuffer.Data(); // add a reference

	// Set up the XMBFile
	const bool ok = Initialise((const char*)m_XMBBuffer.get());
	ENSURE(ok);

	return PSRETURN_OK;
}

bool CXeromyces::ReadXMBFile(const PIVFS& vfs, const VfsPath& filename)
{
	size_t size;
	if(vfs->LoadFile(filename, m_XMBBuffer, size) < 0)
		return false;
	// if the game crashes during loading, (e.g. due to driver bugs),
	// it sometimes leaves empty XMB files in the cache.
	// reporting failure will cause our caller to re-generate the XMB.
	if(size == 0)
		return false;
	ENSURE(size >= 4); // make sure it's at least got the initial header

	// Set up the XMBFile
	if(!Initialise((const char*)m_XMBBuffer.get()))
		return false;

	return true;
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
		CScopeLock lock(g_ValidatorCacheLock);
		RelaxNGValidator& validator = GetValidator(validatorName);
		if (validator.CanValidate() && !validator.ValidateEncoded(doc))
			// For now, log the error and continue, in the future we might fail
			LOGERROR("CXeromyces: failed to validate XML string");
	}

	WriteBuffer writeBuffer;
	CreateXMB(doc, writeBuffer);

	xmlFreeDoc(doc);

	m_XMBBuffer = writeBuffer.Data(); // add a reference

	// Set up the XMBFile
	const bool ok = Initialise((const char*)m_XMBBuffer.get());
	ENSURE(ok);

	return PSRETURN_OK;
}


static void FindNames(const xmlNodePtr node, std::set<std::string>& elementNames, std::set<std::string>& attributeNames)
{
	elementNames.insert((const char*)node->name);

	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
		attributeNames.insert((const char*)attr->name);

	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			FindNames(child, elementNames, attributeNames);
}

static void OutputElement(const xmlNodePtr node, WriteBuffer& writeBuffer,
	std::map<std::string, u32>& elementIDs,
	std::map<std::string, u32>& attributeIDs
)
{
	// Filled in later with the length of the element
	size_t posLength = writeBuffer.Size();
	writeBuffer.Append("????", 4);

	writeBuffer.Append(&elementIDs[(const char*)node->name], 4);

	u32 attrCount = 0;
	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
		++attrCount;
	writeBuffer.Append(&attrCount, 4);

	u32 childCount = 0;
	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			++childCount;
	writeBuffer.Append(&childCount, 4);

	// Filled in later with the offset to the list of child elements
	size_t posChildrenOffset = writeBuffer.Size();
	writeBuffer.Append("????", 4);


	// Trim excess whitespace in the entity's text, while counting
	// the number of newlines trimmed (so that JS error reporting
	// can give the correct line number within the script)

	std::string whitespace = " \t\r\n";
	std::string text;
	for (xmlNodePtr child = node->children; child; child = child->next)
	{
		if (child->type == XML_TEXT_NODE)
		{
			xmlChar* content = xmlNodeGetContent(child);
			text += std::string((const char*)content);
			xmlFree(content);
		}
	}

	u32 linenum = xmlGetLineNo(node);

	// Find the start of the non-whitespace section
	size_t first = text.find_first_not_of(whitespace);

	if (first == text.npos)
		// Entirely whitespace - easy to handle
		text = "";

	else
	{
		// Count the number of \n being cut off,
		// and add them to the line number
		std::string trimmed (text.begin(), text.begin()+first);
		linenum += std::count(trimmed.begin(), trimmed.end(), '\n');

		// Find the end of the non-whitespace section,
		// and trim off everything else
		size_t last = text.find_last_not_of(whitespace);
		text = text.substr(first, 1+last-first);
	}


	// Output text, prefixed by length in bytes
	if (text.length() == 0)
	{
		// No text; don't write much
		writeBuffer.Append("\0\0\0\0", 4);
	}
	else
	{
		// Write length and line number and null-terminated text
		u32 nodeLen = u32(4 + text.length()+1);
		writeBuffer.Append(&nodeLen, 4);
		writeBuffer.Append(&linenum, 4);
		writeBuffer.Append((void*)text.c_str(), nodeLen-4);
	}

	// Output attributes
	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
	{
		writeBuffer.Append(&attributeIDs[(const char*)attr->name], 4);

		xmlChar* value = xmlNodeGetContent(attr->children);
		u32 attrLen = u32(xmlStrlen(value)+1);
		writeBuffer.Append(&attrLen, 4);
		writeBuffer.Append((void*)value, attrLen);
		xmlFree(value);
	}

	// Go back and fill in the child-element offset
	u32 childrenOffset = (u32)(writeBuffer.Size() - (posChildrenOffset+4));
	writeBuffer.Overwrite(&childrenOffset, 4, posChildrenOffset);

	// Output all child elements
	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			OutputElement(child, writeBuffer, elementIDs, attributeIDs);

	// Go back and fill in the length
	u32 length = (u32)(writeBuffer.Size() - posLength);
	writeBuffer.Overwrite(&length, 4, posLength);
}

PSRETURN CXeromyces::CreateXMB(const xmlDocPtr doc, WriteBuffer& writeBuffer)
{
	// Header
	writeBuffer.Append(UnfinishedHeaderMagicStr, 4);
	// Version
	writeBuffer.Append(&XMBVersion, 4);

	u32 i;

	// Find the unique element/attribute names
	std::set<std::string> elementNames;
	std::set<std::string> attributeNames;
	FindNames(xmlDocGetRootElement(doc), elementNames, attributeNames);

	std::map<std::string, u32> elementIDs;
	std::map<std::string, u32> attributeIDs;

	// Output element names
	i = 0;
	u32 elementCount = (u32)elementNames.size();
	writeBuffer.Append(&elementCount, 4);
	for (const std::string& n : elementNames)
	{
		u32 textLen = (u32)n.length()+1;
		writeBuffer.Append(&textLen, 4);
		writeBuffer.Append((void*)n.c_str(), textLen);
		elementIDs[n] = i++;
	}

	// Output attribute names
	i = 0;
	u32 attributeCount = (u32)attributeNames.size();
	writeBuffer.Append(&attributeCount, 4);
	for (const std::string& n : attributeNames)
	{
		u32 textLen = (u32)n.length()+1;
		writeBuffer.Append(&textLen, 4);
		writeBuffer.Append((void*)n.c_str(), textLen);
		attributeIDs[n] = i++;
	}

	OutputElement(xmlDocGetRootElement(doc), writeBuffer, elementIDs, attributeIDs);

	// file is now valid, so insert correct magic string
	writeBuffer.Overwrite(HeaderMagicStr, 4, 0);

	return PSRETURN_OK;
}
