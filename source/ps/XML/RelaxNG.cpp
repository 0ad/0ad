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

#include "RelaxNG.h"

#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"

#include <libxml/relaxng.h>
#include <map>
#include <mutex>

TIMER_ADD_CLIENT(xml_validation);

/*
 * libxml2 leaks memory when parsing schemas: https://bugzilla.gnome.org/show_bug.cgi?id=615767
 * To minimise that problem, keep a global cache of parsed schemas, so we don't
 * leak an indefinitely large amount of memory when repeatedly restarting the simulation.
 */
class RelaxNGSchema;
static std::map<std::string, shared_ptr<RelaxNGSchema> > g_SchemaCache;
static std::mutex g_SchemaCacheLock;

void ClearSchemaCache()
{
	std::lock_guard<std::mutex> lock(g_SchemaCacheLock);
	g_SchemaCache.clear();
}

static void relaxNGErrorHandler(void* UNUSED(userData), xmlErrorPtr error)
{
	// Strip a trailing newline
	std::string message = error->message;
	if (message.length() > 0 && message[message.length()-1] == '\n')
		message.erase(message.length()-1);

	LOGERROR("RelaxNGValidator: Validation %s: %s:%d: %s",
		error->level == XML_ERR_WARNING ? "warning" : "error",
		error->file, error->line, message.c_str());
}

class RelaxNGSchema
{
public:
	xmlRelaxNGPtr m_Schema;

	RelaxNGSchema(const std::string& grammar)
	{
		xmlRelaxNGParserCtxtPtr ctxt = xmlRelaxNGNewMemParserCtxt(grammar.c_str(), (int)grammar.size());
		m_Schema = xmlRelaxNGParse(ctxt);
		xmlRelaxNGFreeParserCtxt(ctxt);

		if (m_Schema == NULL)
			LOGERROR("RelaxNGValidator: Failed to compile schema");
	}

	~RelaxNGSchema()
	{
		if (m_Schema)
			xmlRelaxNGFree(m_Schema);
	}
};

RelaxNGValidator::RelaxNGValidator() :
	m_Schema(NULL)
{
}

RelaxNGValidator::~RelaxNGValidator()
{
}

bool RelaxNGValidator::LoadGrammar(const std::string& grammar)
{
	shared_ptr<RelaxNGSchema> schema;

	{
		std::lock_guard<std::mutex> lock(g_SchemaCacheLock);
		std::map<std::string, shared_ptr<RelaxNGSchema> >::iterator it = g_SchemaCache.find(grammar);
		if (it == g_SchemaCache.end())
		{
			schema = shared_ptr<RelaxNGSchema>(new RelaxNGSchema(grammar));
			g_SchemaCache[grammar] = schema;
		}
		else
		{
			schema = it->second;
		}
	}

	m_Schema = schema->m_Schema;
	if (!m_Schema)
		return false;

	MD5 hash;
	hash.Update((const u8*)grammar.c_str(), grammar.length());
	m_Hash = hash;

	return true;
}

bool RelaxNGValidator::LoadGrammarFile(const PIVFS& vfs, const VfsPath& grammarPath)
{
	CVFSFile file;
	if (file.Load(vfs, grammarPath) != PSRETURN_OK)
		return false;

	return LoadGrammar(file.DecodeUTF8());
}

bool RelaxNGValidator::Validate(const std::wstring& filename, const std::wstring& document) const
{
	std::string docutf8 = "<?xml version='1.0' encoding='utf-8'?>" + utf8_from_wstring(document);

	return ValidateEncoded(filename, docutf8);
}

bool RelaxNGValidator::ValidateEncoded(const std::wstring& filename, const std::string& document) const
{
	TIMER_ACCRUE(xml_validation);

	if (!m_Schema)
	{
		LOGERROR("RelaxNGValidator: No grammar loaded");
		return false;
	}

	xmlDocPtr doc = xmlReadMemory(document.c_str(), (int)document.size(), utf8_from_wstring(filename).c_str(), NULL, XML_PARSE_NONET);
	if (doc == NULL)
	{
		LOGERROR("RelaxNGValidator: Failed to parse document '%s'", utf8_from_wstring(filename).c_str());
		return false;
	}

	bool ret = ValidateEncoded(doc);
	xmlFreeDoc(doc);
	return ret;
}

bool RelaxNGValidator::ValidateEncoded(xmlDocPtr doc) const
{
	xmlRelaxNGValidCtxtPtr ctxt = xmlRelaxNGNewValidCtxt(m_Schema);
	xmlRelaxNGSetValidStructuredErrors(ctxt, &relaxNGErrorHandler, NULL);
	int ret = xmlRelaxNGValidateDoc(ctxt, doc);
	xmlRelaxNGFreeValidCtxt(ctxt);

	if (ret == 0)
	{
		return true;
	}
	else if (ret > 0)
	{
		LOGERROR("RelaxNGValidator: Validation failed for '%s'", doc->name);
		return false;
	}
	else
	{
		LOGERROR("RelaxNGValidator: Internal error %d", ret);
		return false;
	}
}

bool RelaxNGValidator::CanValidate() const
{
	return m_Schema != NULL;
}
