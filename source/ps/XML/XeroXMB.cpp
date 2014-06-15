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

#include "precompiled.h"

#include "Xeromyces.h"

#include "lib/byte_order.h"	// FOURCC_LE

// external linkage (also used by Xeromyces.cpp)
const char* HeaderMagicStr = "XMB0";
const char* UnfinishedHeaderMagicStr = "XMBu";
// Arbitrary version number - change this if we update the code and
// need to invalidate old users' caches
const u32 XMBVersion = 3;

// Warning: May contain traces of pointer abuse

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

	u32 Version = *(u32*)m_Pointer;
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
	u32 ElementNameCount = *(u32*)m_Pointer; m_Pointer += 4;
	for (i = 0; i < ElementNameCount; ++i)
		m_ElementNames[ReadZStr8()] = i;

	u32 AttributeNameCount = *(u32*)m_Pointer; m_Pointer += 4;
	for (i = 0; i < AttributeNameCount; ++i)
		m_AttributeNames[ReadZStr8()] = i;
#else
	// Ignore all the names for now, and skip over them
	// (remembering the position of the first)
	m_ElementNameCount = *(int*)m_Pointer; m_Pointer += 4;
	m_ElementPointer = m_Pointer;
	for (i = 0; i < m_ElementNameCount; ++i)
		m_Pointer += 4 + *(int*)m_Pointer; // skip over the string

	m_AttributeNameCount = *(int*)m_Pointer; m_Pointer += 4;
	m_AttributePointer = m_Pointer;
	for (i = 0; i < m_AttributeNameCount; ++i)
		m_Pointer += 4 + *(int*)m_Pointer; // skip over the string
#endif

	return true;	// success
}

std::string XMBFile::ReadZStr8()
{
	int Length = *(int*)m_Pointer;
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
		if (*(int*)Pos == len && strncasecmp(Pos+4, Name, len) == 0)
			return i;
		// If not, jump to the next string
		Pos += 4 + *(int*)Pos;
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
		if (*(int*)Pos == len && strncasecmp(Pos+4, Name, len) == 0)
			return i;
		// If not, jump to the next string
		Pos += 4 + *(int*)Pos;
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
		Pos += 4 + *(int*)Pos;
	return std::string(Pos+4);
}

std::string XMBFile::GetAttributeString(const int ID) const
{
	const char* Pos = m_AttributePointer;
	for (int i = 0; i < ID; ++i)
		Pos += 4 + *(int*)Pos;
	return std::string(Pos+4);
}



int XMBElement::GetNodeName() const
{
	if (m_Pointer == NULL)
		return -1;

	return *(int*)(m_Pointer + 4); // == ElementName
}

XMBElementList XMBElement::GetChildNodes() const
{
	if (m_Pointer == NULL)
		return XMBElementList(NULL, 0);

	return XMBElementList(
		m_Pointer + 20 + *(int*)(m_Pointer + 16), // == Children[]
		*(int*)(m_Pointer + 12) // == ChildCount
	);
}

XMBAttributeList XMBElement::GetAttributes() const
{
	if (m_Pointer == NULL)
		return XMBAttributeList(NULL, 0);

	return XMBAttributeList(
		m_Pointer + 24 + *(int*)(m_Pointer + 20), // == Attributes[]
		*(int*)(m_Pointer + 8) // == AttributeCount
	);
}

CStr8 XMBElement::GetText() const
{
	// Return empty string if there's no text
	if (m_Pointer == NULL || *(int*)(m_Pointer + 20) == 0)
		return CStr8();

	return CStr8(m_Pointer + 28);
}

int XMBElement::GetLineNumber() const
{
	// Make sure there actually was some text to record the line of
	if (m_Pointer == NULL || *(int*)(m_Pointer + 20) == 0)
		return -1;
	else
		return *(int*)(m_Pointer + 24);
}

XMBElement XMBElementList::GetFirstNamedItem(const int ElementName) const
{
	const char* Pos = m_Pointer;

	// Maybe not the cleverest algorithm, but it should be
	// fast enough with half a dozen attributes:
	for (int i = 0; i < Count; ++i)
	{
		int Length = *(int*)Pos;
		int Name = *(int*)(Pos+4);
		if (Name == ElementName)
			return XMBElement(Pos);
		Pos += Length;
	}

	// Can't find element
	return XMBElement();
}

XMBElement XMBElementList::Item(const int id)
{
	ENSURE(id >= 0 && id < Count && "Element ID out of range");
	const char* Pos;

	// If access is sequential, don't bother scanning
	// through all the nodes to find the next one
	if (id == m_LastItemID+1)
	{
		Pos = m_LastPointer;
		Pos += *(int*)Pos; // skip over the last node
	}
	else
	{
		Pos = m_Pointer;
		// Skip over each preceding node
		for (int i=0; i<id; ++i)
			Pos += *(int*)Pos;
	}
	// Cache information about this node
	m_LastItemID = id;
	m_LastPointer = Pos;

	return XMBElement(Pos);
}

CStr8 XMBAttributeList::GetNamedItem(const int AttributeName) const
{
	const char* Pos = m_Pointer;

	// Maybe not the cleverest algorithm, but it should be
	// fast enough with half a dozen attributes:
	for (int i = 0; i < Count; ++i)
	{
		if (*(int*)Pos == AttributeName)
			return CStr8(Pos+8);
		Pos += 8 + *(int*)(Pos+4); // Skip over the string
	}

	// Can't find attribute
	return CStr8();
}

XMBAttribute XMBAttributeList::Item(const int id)
{
	ENSURE(id >= 0 && id < Count && "Attribute ID out of range");
	const char* Pos;

	// If access is sequential, don't bother scanning through
	// all the nodes to find the right one
	if (id == m_LastItemID+1)
	{
		Pos = m_LastPointer;
		// Skip over the last attribute
		Pos += 8 + *(int*)(Pos+4);
	}
	else
	{
		Pos = m_Pointer;
		// Skip over each preceding attribute
		for (int i=0; i<id; ++i)
			Pos += 8 + *(int*)(Pos+4); // skip ID, length, and string data
	}
	// Cache information about this attribute
	m_LastItemID = id;
	m_LastPointer = Pos;

	return XMBAttribute(*(int*)Pos, CStr8(Pos+8));
}
