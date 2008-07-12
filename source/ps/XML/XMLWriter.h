#ifndef INCLUDED_XMLWRITER
#define INCLUDED_XMLWRITER

/*

System for writing simple XML files, with human-readable formatting.

Example usage:

	XML_Start();

	{
		XML_Element("Scenario");
		{
			XML_Element("Entities");
			for (...)
			{
				XML_Element("Entity");

				{
					XML_Element("Template");
					XML_Text(entity.name);
				}
				// Or equivalently:
				XML_Setting("Template", entity.name);

				{
					XML_Element("Position");
					XML_Attribute("x", entity.x);
					XML_Attribute("y", entity.y);
					XML_Attribute("z", entity.z);
				}

				{
					XML_Element("Orientation");
					XML_Attribute("angle", entity.angle);
				}
			}
		}
	}

	Handle h = vfs_open("/test.xml", FILE_WRITE|FILE_NO_AIO);
	XML_StoreVFS(h);

In general, "{ XML_Element(name); ... }" means "<name> ... </name>" -- the
scoping braces are important to indicate where an element ends.

XML_Attribute/XML_Setting are templated. To support more types, alter the
end of XMLWriter.cpp.

*/

// Starts generating a new XML file.
#define XML_Start() XMLWriter_File xml_file_

// Set pretty printing (newlines, tabs). Defaults to true.
#define XML_SetPrettyPrint(enabled) xml_file_.SetPrettyPrint(false)

// Add a comment to the XML file: <!-- text -->
#define XML_Comment(text) xml_file_.Comment(text)

// Start a new element: <name ...>
#define XML_Element(name) XMLWriter_Element xml_element_ (xml_file_, name)

// Add text to the interior of the current element: <...>text</...>
#define XML_Text(text) xml_element_.Text(text)

// Add an attribute to the current element: <... name="value" ...>
#define XML_Attribute(name, value) xml_element_.Attribute(name, value)

// Add a 'setting': <name>value</name>
#define XML_Setting(name, value) xml_element_.Setting(name, value)

// Create a VFS file from the XML data.
// Returns true on success, false (and logs an error) on failure.
#define XML_StoreVFS(handle) xml_file_.StoreVFS(handle)

// Returns the contents of the XML file as a UTF-8 byte stream in a const CStr&
// string. (Use CStr::FromUTF8 to get a Unicode string back.)
#define XML_GetOutput() xml_file_.GetOutput()


#include "ps/CStr.h"

class XMLWriter_Element;

class XMLWriter_File
{
public:
	XMLWriter_File();

	void SetPrettyPrint(bool enabled) { m_PrettyPrint = enabled; }

	void Comment(const char* text);

	bool StoreVFS(const char* filename);
	const CStr& GetOutput();

private:

	friend class XMLWriter_Element;

	void ElementStart(XMLWriter_Element* element, const char* name);
	void ElementText(const char* text);
	template <typename T> void ElementAttribute(const char* name, const T& value, bool newelement);
	void ElementClose();
	void ElementEnd(const char* name, int type);

	CStr Indent();

	bool m_PrettyPrint;

	CStr m_Data;
	int m_Indent;
	XMLWriter_Element* m_LastElement;
};

class XMLWriter_Element
{
public:
	XMLWriter_Element(XMLWriter_File& file, const char* name);
	~XMLWriter_Element();

	template <typename constCharPtr> void Text(constCharPtr text);
	template <typename T> void Attribute(const char* name, T value) { m_File->ElementAttribute(name, value, false); }
	template <typename T> void Setting(const char* name, T value) { m_File->ElementAttribute(name, value, true); }
	void Close(int type);

private:

	friend class XMLWriter_File;

	XMLWriter_File* m_File;
	CStr m_Name;
	int m_Type;
};

#endif // INCLUDED_XMLWRITER
