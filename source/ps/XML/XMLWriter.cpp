#include "precompiled.h"

#include "XMLWriter.h"

#include "ps/CLogger.h"
#include "lib/res/file/vfs.h"


// TODO (maybe): Write to the VFS handle frequently, instead of buffering
// the entire file, so that large files get written faster.

enum { EL_ATTR, EL_TEXT, EL_SUBEL };

XMLWriter_File::XMLWriter_File(const char* encoding)
	: m_Indent(0), m_LastElement(NULL)
{
	m_Data = "<?xml version=\"1.0\" encoding=\"";
	m_Data += encoding;
	m_Data += "\" standalone=\"no\"?>\n";
}

void XMLWriter_File::Doctype(const char* type, const char* dtd)
{
	m_Data += "<!DOCTYPE ";
	m_Data += type;
	m_Data += " SYSTEM \"";
	m_Data += dtd;
	m_Data += "\">\n";
}

bool XMLWriter_File::StoreVFS(Handle h)
{
	if (m_LastElement) debug_warn("ERROR: Saving XML while an element is still open");

	FileIOBuf data = (FileIOBuf)m_Data.data();
	int err = vfs_io(h, m_Data.Length(), &data);
	if (err < 0)
	{
		LOG(ERROR, "xml", "Error saving XML data through VFS: %lld", h);
		return false;
	}
	return true;
}

void XMLWriter_File::Comment(const char* text)
{
	ElementStart(NULL, "!-- ");
	m_Data += text;
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
	m_Data += "\n";
	m_Data += Indent();
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
		m_Data += " />";
		break;
	case EL_TEXT:
		m_Data += "</";
		m_Data += name;
		m_Data += ">";
		break;
	case EL_SUBEL:
		m_Data += "\n";
		m_Data += Indent();
		m_Data += "</";
		m_Data += name;
		m_Data += ">";
		break;
	default:
		debug_assert(0);
	}
}

void XMLWriter_File::ElementText(const char* text)
{
	m_Data += text;
}


XMLWriter_Element::XMLWriter_Element(XMLWriter_File& file, const char* name)
	: m_File(&file), m_Name(name), m_Type(EL_ATTR)
{
	m_File->ElementStart(this, name);
}


XMLWriter_Element::~XMLWriter_Element()
{
	m_File->ElementEnd(m_Name, m_Type);
}


void XMLWriter_Element::Close(int type)
{
	m_File->ElementClose();
	m_Type = type;
}

void XMLWriter_Element::Text(const char* text)
{
	Close(EL_TEXT);
	m_File->ElementText(text);
}



template <> void XMLWriter_File::ElementAttribute<CStr>(const char* name, const CStr& value, bool newelement)
{
	if (newelement)
	{
		ElementStart(NULL, name);
		m_Data += ">";
		ElementText(value);
		ElementEnd(name, EL_TEXT);
	}
	else
	{
		debug_assert(m_LastElement && m_LastElement->m_Type == EL_ATTR);
		m_Data += " ";
		m_Data += name;
		m_Data += "=\"";
		m_Data += value;
		m_Data += "\"";
	}
}

// Attribute/setting value-to-string template specialisations.
//
// These only deal with basic types. Anything more complicated should
// be converted into a basic type by whatever is making use of XMLWriter,
// to keep game-related logic out of the not-directly-game-related code here.

// Use CStr's conversion for most types:
#define TYPE2(ID_T, ARG_T) \
template <> void XMLWriter_File::ElementAttribute<ID_T>(const char* name, ARG_T value, bool newelement) \
{ \
	ElementAttribute(name, CStr(value), newelement); \
}
#define TYPE(T) TYPE2(T, const T &)

TYPE(int)
TYPE(unsigned int)
TYPE(float)
TYPE(double)
// This is the effect of doing const T& with T=const char* - char const* const&
// Weird - I know ;-)
TYPE2(const char *, char const* const&) 

// Encode Unicode strings as UTF-8 (though that will only be correct if
// the encoding was set to "utf-8"; it'll look a little odd if you store
// Unicode strings in an iso-8859-1 file, so please don't do that)
template <> void XMLWriter_File::ElementAttribute<CStrW>(const char* name, const CStrW& value, bool newelement)
{
	ElementAttribute(name, value.ToUTF8(), newelement);
}
