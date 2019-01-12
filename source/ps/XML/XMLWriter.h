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

#ifndef INCLUDED_XMLWRITER
#define INCLUDED_XMLWRITER

/*
 *
 *System for writing simple XML files, with human-readable formatting.
 *
 *Example usage:
 *
 *	XMLWriter_File exampleFile;
 *	{
 *		XMLWriter_Element scenarioTag (exampleFile,"Scenario");
 *		{
 *			XMLWriter_Element entitiesTag (exampleFile,"Entities");
 *			for (...)
 *			{
 *				XMLWriter_Element entityTag (exampleFile,"Entity");
 *				{
 *					XMLWriter_Element templateTag (exampleFile,"Template");
 *					templateTag.Text(entity.name);
 *				}
 *				// Or equivalently:
 *				templateTag.Setting("Template", entity.name);
 *				{
 *					XMLWriter_Element positionTag (exampleFile,"Position");
 *					positionTag.Attribute("x", entity.x);
 *					positionTag.Attribute("y", entity.y);
 *					positionTag.Attribute("z", entity.z);
 *				}
 *				{
 *					XMLWriter_Element orientationTag (exampleFile,"Orientation");
 *					orientationTag.Attribute("angle", entity.angle);
 *				}
 *			}
 *		}
 *	}
 *	exampleFile.StoreVFS(g_VFS, "/test.xml");
 *
 *	In general, "{ XML_Element(name); ... }" means "<name> ... </name>" -- the
 *	scoping braces are important to indicate where an element ends. If you don't put
 *	them the tag won't be closed until the object's destructor is called, usually
 *	when it goes out of scope.
 *	xml_element_.Attribute/xml_element_.Setting are templated. To support more types, alter the
 *	end of XMLWriter.cpp.
 */

#include "ps/CStr.h"
#include "lib/file/vfs/vfs.h"

class XMBElement;
class XMBFile;
class XMLWriter_Element;

class XMLWriter_File
{
public:
	XMLWriter_File();

	void SetPrettyPrint(bool enabled) { m_PrettyPrint = enabled; }

	void Comment(const char* text);

	void XMB(const XMBFile& file);

	bool StoreVFS(const PIVFS& vfs, const VfsPath& pathname);
	const CStr& GetOutput();

private:

	friend class XMLWriter_Element;

	void ElementXMB(const XMBFile& file, XMBElement el);

	void ElementStart(XMLWriter_Element* element, const char* name);
	void ElementText(const char* text, bool cdata);
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

	template <typename constCharPtr> void Text(constCharPtr text, bool cdata);
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
