/* Copyright (C) 2020 Wildfire Games.
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
		: m_Pointer(offset) {}

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
	XMBElementList(const char* offset, size_t count, const char* endoffset)
		: m_Size(count), m_Pointer(offset), m_CurItemID(0), m_CurPointer(offset), m_EndPointer(endoffset) {}

	// Get first element in list with the given name.
	// Performance is linear in the number of elements in the list.
	XMBElement GetFirstNamedItem(const int ElementName) const;

	// Linear in the number of elements in the list
	XMBElement operator[](size_t id); // returns Children[id]

	class iterator
	{
	public:
		typedef ptrdiff_t difference_type;
		typedef XMBElement value_type;
		typedef XMBElement reference; // Because we need to construct the object
		typedef XMBElement pointer; // Because we need to construct the object
		typedef std::forward_iterator_tag iterator_category;

		iterator(size_t size, const char* ptr, const char* endptr = NULL)
			: m_Size(size), m_CurItemID(endptr ? size : 0), m_CurPointer(endptr ? endptr : ptr), m_Pointer(ptr) {}
		XMBElement operator*() const { return XMBElement(m_CurPointer); }
		XMBElement operator->() const { return **this; }
		iterator& operator++();

		bool operator==(const iterator& rhs) const
		{
			return m_Size == rhs.m_Size &&
			       m_CurItemID == rhs.m_CurItemID &&
			       m_CurPointer == rhs.m_CurPointer;
		}
		bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
	private:
		size_t m_Size;
		size_t m_CurItemID;
		const char* m_CurPointer;
		const char* m_Pointer;
	};
	iterator begin() { return iterator(m_Size, m_Pointer); }
	iterator end() { return iterator(m_Size, m_Pointer, m_EndPointer); }

	size_t size() const { return m_Size; }
	bool empty() const { return m_Size == 0; }

private:
	size_t m_Size;

	const char* m_Pointer;

	// For optimised sequential access:
	size_t m_CurItemID;
	const char* m_CurPointer;

	const char* m_EndPointer;
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
	XMBAttributeList(const char* offset, size_t count, const char* endoffset)
		: m_Size(count), m_Pointer(offset), m_CurItemID(0), m_CurPointer(offset), m_EndPointer(endoffset) {}

	// Get the attribute value directly
	CStr8 GetNamedItem(const int AttributeName) const;

	// Linear in the number of elements in the list
	XMBAttribute operator[](size_t id); // returns Children[id]

	class iterator
	{
	public:
		typedef ptrdiff_t difference_type;
		typedef XMBAttribute value_type;
		typedef XMBAttribute reference; // Because we need to construct the object
		typedef XMBAttribute pointer; // Because we need to construct the object
		typedef std::forward_iterator_tag iterator_category;

		iterator(size_t size, const char* ptr, const char* endptr = NULL)
			: m_Size(size), m_CurItemID(endptr ? size : 0), m_CurPointer(endptr ? endptr : ptr), m_Pointer(ptr) {}
		XMBAttribute operator*() const;
		XMBAttribute operator->() const { return **this; }
		iterator& operator++();

		bool operator==(const iterator& rhs) const
		{
			return m_Size == rhs.m_Size &&
			       m_CurItemID == rhs.m_CurItemID &&
			       m_CurPointer == rhs.m_CurPointer;
		}
		bool operator!=(const iterator& rhs) const { return !(*this == rhs); }
	private:
		size_t m_Size;
		size_t m_CurItemID;
		const char* m_CurPointer;
		const char* m_Pointer;
	};
	iterator begin() const { return iterator(m_Size, m_Pointer); }
	iterator end() const { return iterator(m_Size, m_Pointer, m_EndPointer); }

	size_t size() const { return m_Size; }
	bool empty() const { return m_Size == 0; }

private:
	size_t m_Size;

	// Pointer to start of attribute list
	const char* m_Pointer;

	// For optimised sequential access:
	size_t m_CurItemID;
	const char* m_CurPointer;

	const char* m_EndPointer;
};

#endif // INCLUDED_XEROXMB
