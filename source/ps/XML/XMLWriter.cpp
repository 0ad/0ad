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

#include "XMLWriter.h"

#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "lib/utf8.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/sysdep/cpu.h"
#include "maths/Fixed.h"


// TODO (maybe): Write to the file frequently, instead of buffering
// the entire file, so that large files get written faster.

namespace
{
	CStr escapeAttributeValue(const char* input)
	{
		// Spec says:
		//     AttValue ::= '"' ([^<&"] | Reference)* '"'
		// so > is allowed in attribute values, so we don't bother escaping it.

		CStr ret = input;
		ret.Replace("&", "&amp;");
		ret.Replace("<", "&lt;");
		ret.Replace("\"", "&quot;");
		return ret;
	}

	CStr escapeCharacterData(const char* input)
	{
		//     CharData ::= [^<&]* - ([^<&]* ']]>' [^<&]*)

		CStr ret = input;
		ret.Replace("&", "&amp;");
		ret.Replace("<", "&lt;");
		ret.Replace("]]>", "]]&gt;");
		return ret;
	}

	CStr escapeCDATA(const char* input)
	{
		CStr ret = input;
		ret.Replace("]]>", "]]>]]&gt;<![CDATA[");
		return ret;
	}

	CStr escapeComment(const char* input)
	{
		//     Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
		// This just avoids double-hyphens, and doesn't enforce the no-hyphen-at-end
		// rule, since it's only used in contexts where there's already a space
		// between this data and the -->.
		CStr ret = input;
		ret.Replace("--", "\xE2\x80\x90\xE2\x80\x90");
			// replace with U+2010 HYPHEN, because it's close enough and it's
			// probably nicer than inserting spaces or deleting hyphens or
			// any alternative
		return ret;
	}
}

enum { EL_ATTR, EL_TEXT, EL_SUBEL };

XMLWriter_File::XMLWriter_File()
	: m_Indent(0), m_LastElement(NULL),
	m_PrettyPrint(true)
{
	// Encoding is always UTF-8 - that's one of the only two guaranteed to be
	// supported by XML parsers (along with UTF-16), and there's not much need
	// to let people choose another.
	m_Data = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
}

bool XMLWriter_File::StoreVFS(const PIVFS& vfs, const VfsPath& pathname)
{
	if (m_LastElement) debug_warn(L"ERROR: Saving XML while an element is still open");

	const size_t size = m_Data.length();
	shared_ptr<u8> data;
	AllocateAligned(data, size, maxSectorSize);
	memcpy(data.get(), m_Data.data(), size);
	Status ret = vfs->CreateFile(pathname, data, size);
	if (ret < 0)
	{
		LOGERROR("Error saving XML data through VFS: %lld '%s'", (long long)ret, pathname.string8());
		return false;
	}
	return true;
}

const CStr& XMLWriter_File::GetOutput()
{
	return m_Data;
}


void XMLWriter_File::XMB(const XMBFile& file)
{
	ElementXMB(file, file.GetRoot());
}

void XMLWriter_File::ElementXMB(const XMBFile& file, XMBElement el)
{
	XMLWriter_Element writer(*this, file.GetElementString(el.GetNodeName()).c_str());

	XERO_ITER_ATTR(el, attr)
		writer.Attribute(file.GetAttributeString(attr.Name).c_str(), attr.Value);

	XERO_ITER_EL(el, child)
		ElementXMB(file, child);
}

void XMLWriter_File::Comment(const char* text)
{
	ElementStart(NULL, "!-- ");
	m_Data += escapeComment(text);
	m_Data += " -->";
	--m_Indent;
}

CStr XMLWriter_File::Indent()
{
	return std::string(m_Indent, '\t');
}

void XMLWriter_File::ElementStart(XMLWriter_Element* element, const char* name)
{
	if (m_LastElement) m_LastElement->Close(EL_SUBEL);
	m_LastElement = element;

	if (m_PrettyPrint)
	{
		m_Data += "\n";
		m_Data += Indent();
	}
	m_Data += "<";
	m_Data += name;

	++m_Indent;
}

void XMLWriter_File::ElementClose()
{
	m_Data += ">";
}

void XMLWriter_File::ElementEnd(const char* name, int type)
{
	--m_Indent;
	m_LastElement = NULL;

	switch (type)
	{
	case EL_ATTR:
		m_Data += "/>";
		break;
	case EL_TEXT:
		m_Data += "</";
		m_Data += name;
		m_Data += ">";
		break;
	case EL_SUBEL:
		if (m_PrettyPrint)
		{
			m_Data += "\n";
			m_Data += Indent();
		}
		m_Data += "</";
		m_Data += name;
		m_Data += ">";
		break;
	default:
		DEBUG_WARN_ERR(ERR::LOGIC);
	}
}

void XMLWriter_File::ElementText(const char* text, bool cdata)
{
	if (cdata)
	{
		m_Data += "<![CDATA[";
		m_Data += escapeCDATA(text);
		m_Data += "]]>";
	}
	else
	{
		m_Data += escapeCharacterData(text);
	}
}


XMLWriter_Element::XMLWriter_Element(XMLWriter_File& file, const char* name)
	: m_File(&file), m_Name(name), m_Type(EL_ATTR)
{
	m_File->ElementStart(this, name);
}


XMLWriter_Element::~XMLWriter_Element()
{
	m_File->ElementEnd(m_Name.c_str(), m_Type);
}


void XMLWriter_Element::Close(int type)
{
	if (m_Type == type)
		return;

	m_File->ElementClose();
	m_Type = type;
}


// Template specialisations for various string types:

template <> void XMLWriter_Element::Text<const char*>(const char* text, bool cdata)
{
	Close(EL_TEXT);
	m_File->ElementText(text, cdata);
}

template <> void XMLWriter_Element::Text<const wchar_t*>(const wchar_t* text, bool cdata)
{
	Text( CStrW(text).ToUTF8().c_str(), cdata );
}

//

template <> void XMLWriter_File::ElementAttribute<const char*>(const char* name, const char* const& value, bool newelement)
{
	if (newelement)
	{
		ElementStart(NULL, name);
		m_Data += ">";
		ElementText(value, false);
		ElementEnd(name, EL_TEXT);
	}
	else
	{
		ENSURE(m_LastElement && m_LastElement->m_Type == EL_ATTR);
		m_Data += " ";
		m_Data += name;
		m_Data += "=\"";
		m_Data += escapeAttributeValue(value);
		m_Data += "\"";
	}
}

// Attribute/setting value-to-string template specialisations.
//
// These only deal with basic types. Anything more complicated should
// be converted into a basic type by whatever is making use of XMLWriter,
// to keep game-related logic out of the not-directly-game-related code here.

template <> void XMLWriter_File::ElementAttribute<CStr>(const char* name, const CStr& value, bool newelement)
{
	ElementAttribute(name, value.c_str(), newelement);
}
template <> void XMLWriter_File::ElementAttribute<std::string>(const char* name, const std::string& value, bool newelement)
{
	ElementAttribute(name, value.c_str(), newelement);
}
// Encode Unicode strings as UTF-8
template <> void XMLWriter_File::ElementAttribute<CStrW>(const char* name, const CStrW& value, bool newelement)
{
	ElementAttribute(name, value.ToUTF8(), newelement);
}
template <> void XMLWriter_File::ElementAttribute<std::wstring>(const char* name, const std::wstring& value, bool newelement)
{
	ElementAttribute(name, utf8_from_wstring(value).c_str(), newelement);
}

template <> void XMLWriter_File::ElementAttribute<fixed>(const char* name, const fixed& value, bool newelement)
{
	ElementAttribute(name, value.ToString().c_str(), newelement);
}

template <> void XMLWriter_File::ElementAttribute<int>(const char* name, const int& value, bool newelement)
{
	std::stringstream ss;
	ss << value;
	ElementAttribute(name, ss.str().c_str(), newelement);
}

template <> void XMLWriter_File::ElementAttribute<unsigned int>(const char* name, const unsigned int& value, bool newelement)
{
	std::stringstream ss;
	ss << value;
	ElementAttribute(name, ss.str().c_str(), newelement);
}

template <> void XMLWriter_File::ElementAttribute<float>(const char* name, const float& value, bool newelement)
{
	std::stringstream ss;
	ss << value;
	ElementAttribute(name, ss.str().c_str(), newelement);
}

template <> void XMLWriter_File::ElementAttribute<double>(const char* name, const double& value, bool newelement)
{
	std::stringstream ss;
	ss << value;
	ElementAttribute(name, ss.str().c_str(), newelement);
}
