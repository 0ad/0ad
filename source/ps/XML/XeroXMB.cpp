/* Copyright (C) 2015 Wildfire Games.
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

#include "Xeromyces.h"

#include "lib/byte_order.h"	// FOURCC_LE

// external linkage (also used by Xeromyces.cpp)
const char* HeaderMagicStr = "XMB0";
const char* UnfinishedHeaderMagicStr = "XMBu";
// Arbitrary version number - change this if we update the code and
// need to invalidate old users' caches
const u32 XMBVersion = 3;

template<typename T>
static inline T read(const void* ptr)
{
	T ret;
	memcpy(&ret, ptr, sizeof(T));
	return ret;
}

bool XMBFile::Initialise(const char* FileData)
{
	m_Pointer = FileData;
	char Header[5] = { 0 };
	strncpy_s(Header, 5, m_Pointer, 4);
	m_Pointer += 4;
	// (c.f. @return documentation of this function)
	if(!strcmp(Header, UnfinishedHeaderMagicStr))
		return false;
	ENSURE(!strcmp(Header, HeaderMagicStr) && "Invalid XMB header!");

	u32 Version = read<u32>(m_Pointer);
	m_Pointer += 4;
	if (Version != XMBVersion)
		return false;

	int i;

	// FIXME Check that m_Pointer doesn't end up past the end of the buffer
	// (it shouldn't be all that dangerous since we're only doing read-only
	// access, but it might crash on an invalid file, reading a couple of
	// billion random element names from RAM)

#ifdef XERO_USEMAP
	// Build a std::map of all the names->ids
	u32 ElementNameCount = read<u32>(m_Pointer); m_Pointer += 4;
	for (i = 0; i < ElementNameCount; ++i)
		m_ElementNames[ReadZStr8()] = i;

	u32 AttributeNameCount = read<u32>(m_Pointer); m_Pointer += 4;
	for (i = 0; i < AttributeNameCount; ++i)
		m_AttributeNames[ReadZStr8()] = i;
#else
	// Ignore all the names for now, and skip over them
	// (remembering the position of the first)
	m_ElementNameCount = read<int>(m_Pointer); m_Pointer += 4;
	m_ElementPointer = m_Pointer;
	for (i = 0; i < m_ElementNameCount; ++i)
		m_Pointer += 4 + read<int>(m_Pointer); // skip over the string

	m_AttributeNameCount = read<int>(m_Pointer); m_Pointer += 4;
	m_AttributePointer = m_Pointer;
	for (i = 0; i < m_AttributeNameCount; ++i)
		m_Pointer += 4 + read<int>(m_Pointer); // skip over the string
#endif

	return true;	// success
}

std::string XMBFile::ReadZStr8()
{
	int Length = read<int>(m_Pointer);
	m_Pointer += 4;
	std::string String (m_Pointer); // reads up until the first NULL
	m_Pointer += Length;
	return String;
}

XMBElement XMBFile::GetRoot() const
{
	return XMBElement(m_Pointer);
}


#ifdef XERO_USEMAP

int XMBFile::GetElementID(const char* Name) const
{
	return m_ElementNames[Name];
}

int XMBFile::GetAttributeID(const char* Name) const
{
	return m_AttributeNames[Name];
}

#else // #ifdef XERO_USEMAP

int XMBFile::GetElementID(const char* Name) const
{
	const char* Pos = m_ElementPointer;

	int len = (int)strlen(Name)+1; // count bytes, including null terminator

	// Loop through each string to find a match
	for (int i = 0; i < m_ElementNameCount; ++i)
	{
		// See if this could be the right string, checking its
		// length and then its contents
		if (read<int>(Pos) == len && strncasecmp(Pos+4, Name, len) == 0)
			return i;
		// If not, jump to the next string
		Pos += 4 + read<int>(Pos);
	}
	// Failed
	return -1;
}

int XMBFile::GetAttributeID(const char* Name) const
{
	const char* Pos = m_AttributePointer;

	int len = (int)strlen(Name)+1; // count bytes, including null terminator

	// Loop through each string to find a match
	for (int i = 0; i < m_AttributeNameCount; ++i)
	{
		// See if this could be the right string, checking its
		// length and then its contents
		if (read<int>(Pos) == len && strncasecmp(Pos+4, Name, len) == 0)
			return i;
		// If not, jump to the next string
		Pos += 4 + read<int>(Pos);
	}
	// Failed
	return -1;
}
#endif // #ifdef XERO_USEMAP / #else


// Relatively inefficient, so only use when
// laziness overcomes the need for speed
std::string XMBFile::GetElementString(const int ID) const
{
	const char* Pos = m_ElementPointer;
	for (int i = 0; i < ID; ++i)
		Pos += 4 + read<int>(Pos);
	return std::string(Pos+4);
}

std::string XMBFile::GetAttributeString(const int ID) const
{
	const char* Pos = m_AttributePointer;
	for (int i = 0; i < ID; ++i)
		Pos += 4 + read<int>(Pos);
	return std::string(Pos+4);
}



int XMBElement::GetNodeName() const
{
	if (m_Pointer == NULL)
		return -1;

	return read<int>(m_Pointer + 4); // == ElementName
}

XMBElementList XMBElement::GetChildNodes() const
{
	if (m_Pointer == NULL)
		return XMBElementList(NULL, 0, NULL);

	return XMBElementList(
		m_Pointer + 20 + read<int>(m_Pointer + 16), // == Children[]
		read<int>(m_Pointer + 12), // == ChildCount
		m_Pointer + read<int>(m_Pointer) // == &Children[ChildCount]
	);
}

XMBAttributeList XMBElement::GetAttributes() const
{
	if (m_Pointer == NULL)
		return XMBAttributeList(NULL, 0, NULL);

	return XMBAttributeList(
		m_Pointer + 24 + read<int>(m_Pointer + 20), // == Attributes[]
		read<int>(m_Pointer + 8), // == AttributeCount
		m_Pointer + 20 + read<int>(m_Pointer + 16) // == &Attributes[AttributeCount] ( == &Children[])
	);
}

CStr8 XMBElement::GetText() const
{
	// Return empty string if there's no text
	if (m_Pointer == NULL || read<int>(m_Pointer + 20) == 0)
		return CStr8();

	return CStr8(m_Pointer + 28);
}

int XMBElement::GetLineNumber() const
{
	// Make sure there actually was some text to record the line of
	if (m_Pointer == NULL || read<int>(m_Pointer + 20) == 0)
		return -1;
	else
		return read<int>(m_Pointer + 24);
}

XMBElement XMBElementList::GetFirstNamedItem(const int ElementName) const
{
	const char* Pos = m_Pointer;

	// Maybe not the cleverest algorithm, but it should be
	// fast enough with half a dozen attributes:
	for (size_t i = 0; i < m_Size; ++i)
	{
		int Length = read<int>(Pos);
		int Name = read<int>(Pos+4);
		if (Name == ElementName)
			return XMBElement(Pos);
		Pos += Length;
	}

	// Can't find element
	return XMBElement();
}

XMBElementList::iterator& XMBElementList::iterator::operator++()
{
	m_CurPointer += read<int>(m_CurPointer);
	++m_CurItemID;
	return (*this);
}

XMBElement XMBElementList::operator[](size_t id)
{
	ENSURE(id < m_Size && "Element ID out of range");
	const char* Pos;
	size_t i;

	if (id < m_CurItemID)
	{
		Pos = m_Pointer;
		i = 0;
	}
	else
	{
		// If access is sequential, don't bother scanning
		// through all the nodes to find the next one
		Pos = m_CurPointer;
		i = m_CurItemID;
	}

	// Skip over each preceding node
	for (; i < id; ++i)
		Pos += read<int>(Pos);

	// Cache information about this node
	m_CurItemID = id;
	m_CurPointer = Pos;

	return XMBElement(Pos);
}

CStr8 XMBAttributeList::GetNamedItem(const int AttributeName) const
{
	const char* Pos = m_Pointer;

	// Maybe not the cleverest algorithm, but it should be
	// fast enough with half a dozen attributes:
	for (size_t i = 0; i < m_Size; ++i)
	{
		if (read<int>(Pos) == AttributeName)
			return CStr8(Pos+8);
		Pos += 8 + read<int>(Pos+4); // Skip over the string
	}

	// Can't find attribute
	return CStr8();
}

XMBAttribute XMBAttributeList::iterator::operator*() const
{
	return XMBAttribute(read<int>(m_CurPointer), CStr8(m_CurPointer+8));
}

XMBAttributeList::iterator& XMBAttributeList::iterator::operator++()
{
	m_CurPointer += 8 + read<int>(m_CurPointer+4); // skip ID, length, and string data
	++m_CurItemID;
	return (*this);
}

XMBAttribute XMBAttributeList::operator[](size_t id)
{
	ENSURE(id < m_Size && "Attribute ID out of range");
	const char* Pos;
	size_t i;

	if (id < m_CurItemID)
	{
		Pos = m_Pointer;
		i = 0;
	}
	else
	{
		// If access is sequential, don't bother scanning
		// through all the nodes to find the next one
		Pos = m_CurPointer;
		i = m_CurItemID;
	}

	// Skip over each preceding attribute
	for (; i < id; ++i)
		Pos += 8 + read<int>(Pos+4); // skip ID, length, and string data

	// Cache information about this attribute
	m_CurItemID = id;
	m_CurPointer = Pos;

	return XMBAttribute(read<int>(Pos), CStr8(Pos+8));
}
