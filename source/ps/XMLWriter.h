#ifndef XMLWRITER_H
#define XMLWRITER_H

/*

System for writing simple XML files, with human-readable formatting.

Example usage:

	XML_Start("utf-8");
	XML_Doctype("Scenario", "/maps/scenario.dtd");

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

// Encoding should usually be "iso-8859-1" or "utf-8"
#define XML_Start(encoding) XMLWriter_File xml_file_ (encoding)

// Set the doctype/DTD (e.g. "Object", "/art/actors/object.dtd")
#define XML_Doctype(type, dtd) xml_file_.Doctype(type, dtd)

// Add a comment to the XML file: <!-- text -->
#define XML_Comment(text) xml_file_.Comment(text)

// Start a new element: <name ...>
#define XML_Element(name) XMLWriter_Element xml_element_ (xml_file_, name)

// Add text to the interior of the current element: ...>text</...>
#define XML_Text(text) xml_element_.Text(text)

// Add an attribute to the current element: <... name="value" ...>
#define XML_Attribute(name, value) xml_element_.Attribute(name, value)

// Add a 'setting': <name>value</name>
#define XML_Setting(name, value) xml_element_.Setting(name, value)

// Output the XML file to a VFS handle, which must be opened with FILE_WRITE|FILE_NO_AIO.
// Returns true on success, false (and logs an error) on failure.
#define XML_StoreVFS(handle) xml_file_.StoreVFS(handle)


#include "ps/CStr.h"
#include "lib/res/handle.h"

class XMLWriter_Element;

class XMLWriter_File
{
public:
	XMLWriter_File(const char* encoding);

	void Doctype(const char* type, const char* dtd);
	void Comment(const char* text);

	bool StoreVFS(Handle h);

	CStr HACK_GetData() { return m_Data; }

private:

	friend class XMLWriter_Element;

	void ElementStart(XMLWriter_Element* element, const char* name);
	void ElementText(const char* text);
	template <typename T> void ElementAttribute(const char* name, T& value, bool newelement);
	void ElementClose();
	void ElementEnd(const char* name, int type);

	CStr Indent();

	CStr m_Data;
	int m_Indent;
	XMLWriter_Element* m_LastElement;
};

class XMLWriter_Element
{
public:
	XMLWriter_Element(XMLWriter_File& file, const char* name);
	~XMLWriter_Element();

	void Text(const char* text);
	template <typename T> void Attribute(const char* name, T value) { m_File->ElementAttribute(name, value, false); }
	template <typename T> void Setting(const char* name, T value) { m_File->ElementAttribute(name, value, true); }
	void Close(int type);

private:

	friend class XMLWriter_File;

	XMLWriter_File* m_File;
	CStr m_Name;
	int m_Type;
};

#endif // XMLWRITER_H
