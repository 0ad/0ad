/* Copyright (C) 2014 Wildfire Games.
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

/* 
	Xeromyces - XMB reading library
*/

/* 

Brief outline:

XMB is a binary representation of XML, with some limitations
but much more efficiency (particularly for loading simple data
classes that don't need much initialisation).

Main limitations:
 * Can't correctly handle mixed text/elements inside elements -
   "<div> <b> Text </b> </div>" and "<div> Te<b/>xt </div>" are
   considered identical.
 * Tries to avoid using strings - you usually have to load the
   numeric IDs and use them instead.
 * Case-sensitive (but converts all element/attribute names in
   the XML file to lowercase, so you only have to be careful in
   the code)


Theoretical file structure:

XMB_File {
	char Header[4]; // because everyone has one; currently "XMB0"
	u32 Version;

	int ElementNameCount;
	ZStr8 ElementNames[];

	int AttributeNameCount;
	ZStr8 AttributeNames[];

	XMB_Node Root;
}

XMB_Node {
0)	int Length; // of entire struct, so it can be skipped over

4)	int ElementName;

8)	int AttributeCount;
12)	int ChildCount;

16)	int ChildrenOffset; // == sizeof(Text)+sizeof(Attributes)
20)	XMB_Text Text;
	XMB_Attribute Attributes[];
	XMB_Node Children[];

}

XMB_Attribute {
	int Name;
	ZStr8 Value;
}

ZStr8 {
	int Length; // in bytes
	char* Text; // null-terminated UTF8
}

XMB_Text {
20)	int Length; // 0 if there's no text, else 4+sizeof(Text) in bytes including terminator
	// If Length != 0:
24)	int LineNumber; // for e.g. debugging scripts
28)	char* Text; // null-terminated UTF8
}

*/

#ifndef INCLUDED_XEROXMB
#define INCLUDED_XEROXMB

// Define to use a std::map for name lookups rather than a linear search.
// (The map is usually slower.)
//#define XERO_USEMAP 

#include <string>

#ifdef XERO_USEMAP
# include <map>
#endif

#include "ps/CStr.h"

// File headers, to make sure it doesn't try loading anything other than an XMB
extern const char* HeaderMagicStr;
extern const char* UnfinishedHeaderMagicStr;
extern const u32 XMBVersion;

class XMBElement;
class XMBElementList;
class XMBAttributeList;


class XMBFile
{
public:

	XMBFile() : m_Pointer(NULL) {}

	// Initialise from the contents of an XMB file.
	// FileData must remain allocated and unchanged while
	// the XMBFile is being used.
	// @return indication of success; main cause for failure is attempting to
	// load a partially valid XMB file (e.g. if the game was interrupted
	// while writing it), which we detect by checking the magic string.
	// It also fails when trying to load an XMB file with a different version.
	bool Initialise(const char* FileData);

	// Returns the root element
	XMBElement GetRoot() const;

	
	// Returns internal ID for a given element/attribute string.
	int GetElementID(const char* Name) const;
	int GetAttributeID(const char* Name) const;

	// For lazy people (e.g. me) when speed isn't vital:

	// Returns element/attribute string for a given internal ID
	std::string GetElementString(const int ID) const;
	std::string GetAttributeString(const int ID) const;

private:
	const char* m_Pointer;

#ifdef XERO_USEMAP
	std::map<std::string, int> m_ElementNames;
	std::map<std::string, int> m_AttributeNames;
#else
	int m_ElementNameCount;
	int m_AttributeNameCount;
	const char* m_ElementPointer;
	const char* m_AttributePointer;
#endif

	std::string ReadZStr8();
};

class XMBElement
{
public:
	XMBElement()
		: m_Pointer(0) {}

	XMBElement(const char* offset)
		: m_Pointer(offset)	{}

	int GetNodeName() const;
	XMBElementList GetChildNodes() const;
	XMBAttributeList GetAttributes() const;
	CStr8 GetText() const;
	// Returns the line number of the text within this element,
	// or -1 if there is no text
	int GetLineNumber() const;

private:
	// Pointer to the start of the node
	const char* m_Pointer;
};

class XMBElementList
{
public:
	XMBElementList()
		: Count(0), m_Pointer(0), m_LastItemID(-2) {}

	XMBElementList(const char* offset, int count)
		: Count(count),
		  m_Pointer(offset),
		  m_LastItemID(-2) {} // use -2 because it isn't x-1 where x is a non-negative integer

	// Get first element in list with the given name.
	// Performance is linear in the number of elements in the list.
	XMBElement GetFirstNamedItem(const int ElementName) const;

	XMBElement Item(const int id); // returns Children[id]

	int Count;

private:
	const char* m_Pointer;

	// For optimised sequential access:
	int m_LastItemID;
	const char* m_LastPointer;
};


struct XMBAttribute
{
	XMBAttribute() {}
	XMBAttribute(int name, const CStr8& value)
		: Name(name), Value(value) {};

	int Name;
	CStr8 Value; // UTF-8 encoded
};

class XMBAttributeList
{
public:
	XMBAttributeList(const char* offset, int count)
		: Count(count), m_Pointer(offset), m_LastItemID(-2) {};

	// Get the attribute value directly
	CStr8 GetNamedItem(const int AttributeName) const;

	// Returns an attribute by position in the list
	XMBAttribute Item(const int id);

	int Count;

private:
	// Pointer to start of attribute list
	const char* m_Pointer;

	// For optimised sequential access:
	int m_LastItemID;
	const char* m_LastPointer;
};

#endif // INCLUDED_XEROXMB
