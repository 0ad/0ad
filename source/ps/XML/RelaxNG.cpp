/* Copyright (C) 2010 Wildfire Games.
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

TIMER_ADD_CLIENT(xml_validation);

RelaxNGValidator::RelaxNGValidator() :
	m_Schema(NULL)
{
}

RelaxNGValidator::~RelaxNGValidator()
{
	if (m_Schema)
		xmlRelaxNGFree(m_Schema);
}

bool RelaxNGValidator::LoadGrammar(const std::string& grammar)
{
	TIMER_ACCRUE(xml_validation);

	debug_assert(m_Schema == NULL);

	xmlRelaxNGParserCtxtPtr ctxt = xmlRelaxNGNewMemParserCtxt(grammar.c_str(), (int)grammar.size());
	m_Schema = xmlRelaxNGParse(ctxt);
	xmlRelaxNGFreeParserCtxt(ctxt);

	if (m_Schema == NULL)
	{
		LOGERROR(L"RelaxNGValidator: Failed to compile schema");
		return false;
	}

	return true;
}

bool RelaxNGValidator::Validate(const std::wstring& filename, const std::wstring& document)
{
	TIMER_ACCRUE(xml_validation);

	if (!m_Schema)
	{
		LOGERROR(L"RelaxNGValidator: No grammar loaded");
		return false;
	}

	std::string docutf8 = "<?xml version='1.0' encoding='utf-8'?>" + utf8_from_wstring(document);

	xmlDocPtr doc = xmlReadMemory(docutf8.c_str(), (int)docutf8.size(), utf8_from_wstring(filename).c_str(), NULL, XML_PARSE_NONET);
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
