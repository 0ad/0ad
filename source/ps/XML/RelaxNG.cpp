/* Copyright (C) 2013 Wildfire Games.
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

#include <libxml/relaxng.h>
#include <map>

TIMER_ADD_CLIENT(xml_validation);

/*
 * libxml2 leaks memory when parsing schemas: https://bugzilla.gnome.org/show_bug.cgi?id=615767
 * To minimise that problem, keep a global cache of parsed schemas, so we don't
 * leak an indefinitely large amount of memory when repeatedly restarting the simulation.
 */

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
			LOGERROR(L"RelaxNGValidator: Failed to compile schema");
	}

	~RelaxNGSchema()
	{
		if (m_Schema)
			xmlRelaxNGFree(m_Schema);
	}
};

static std::map<std::string, shared_ptr<RelaxNGSchema> > g_SchemaCache;
static CMutex g_SchemaCacheLock;

RelaxNGValidator::RelaxNGValidator() :
	m_Schema(NULL)
{
}

RelaxNGValidator::~RelaxNGValidator()
{
}

bool RelaxNGValidator::LoadGrammar(const std::string& grammar)
{
	TIMER_ACCRUE(xml_validation);

	shared_ptr<RelaxNGSchema> schema;

	{
		CScopeLock lock(g_SchemaCacheLock);
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
	return true;
}

bool RelaxNGValidator::Validate(const std::wstring& filename, const std::wstring& document)
{
	std::string docutf8 = "<?xml version='1.0' encoding='utf-8'?>" + utf8_from_wstring(document);

	return ValidateEncoded(filename, docutf8);
}

bool RelaxNGValidator::ValidateEncoded(const std::wstring& filename, const std::string& document)
{
	TIMER_ACCRUE(xml_validation);

	if (!m_Schema)
	{
		LOGERROR(L"RelaxNGValidator: No grammar loaded");
		return false;
	}

	xmlDocPtr doc = xmlReadMemory(document.c_str(), (int)document.size(), utf8_from_wstring(filename).c_str(), NULL, XML_PARSE_NONET);
	if (doc == NULL)
	{
		LOGERROR(L"RelaxNGValidator: Failed to parse document");
		return false;
	}

	xmlRelaxNGValidCtxtPtr ctxt = xmlRelaxNGNewValidCtxt(m_Schema);
	int ret = xmlRelaxNGValidateDoc(ctxt, doc);
	xmlRelaxNGFreeValidCtxt(ctxt);
	xmlFreeDoc(doc);

	if (ret == 0)
	{
		return true;
	}
	else if (ret > 0)
	{
		LOGERROR(L"RelaxNGValidator: Validation failed");
		return false;
	}
	else
	{
		LOGERROR(L"RelaxNGValidator: Internal error %d", ret);
		return false;
	}
}
