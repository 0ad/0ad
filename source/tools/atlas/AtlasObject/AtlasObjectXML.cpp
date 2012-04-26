/* Copyright (C) 2009 Wildfire Games.
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

#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include <cassert>
#include <cstring>

#include <memory>
#include <fstream>

#include <libxml/parser.h>

// UTF conversion code adapted from http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char trailingBytesForUTF8[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5 };
static const unsigned long offsetsFromUTF8[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL };
class toXmlChar
{
public:
	toXmlChar(const std::wstring& str)
	{
		for (size_t i = 0; i < str.length(); ++i)
		{
			unsigned short bytesToWrite;
			wchar_t ch = str[i];

			if (ch < 0x80) bytesToWrite = 1;
			else if (ch < 0x800) bytesToWrite = 2;
			else if (ch < 0x10000) bytesToWrite = 3;
			else if (ch < 0x110000) bytesToWrite = 4;
			else bytesToWrite = 3, ch = 0xFFFD; // replacement character

			char buf[4];
			char* target = &buf[bytesToWrite];
			// GCC sometimes warns "array subscript is above array bounds [-Warray-bounds]"
			// for the above line, which is a false positive - the C++ standard allows a
			// pointer to just after the last element in an array, as long as it's not
			// dereferenced (which it isn't here)
			switch (bytesToWrite)
			{
			case 4: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
			case 3: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
			case 2: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
			case 1: *--target = (char)(ch | firstByteMark[bytesToWrite]);
			}
			data += std::string(buf, bytesToWrite);
		}
	}
	operator const xmlChar*()
	{
		return (const xmlChar*)data.c_str();
	}

private:
	std::string data;
};

std::wstring fromXmlChar(const xmlChar* str)
{
	std::wstring result;
	const xmlChar* source = str;
	const xmlChar* sourceEnd = str + strlen((const char*)str);
	while (source < sourceEnd)
	{
		unsigned long ch = 0;
		int extraBytesToRead = trailingBytesForUTF8[*source];
		assert(source + extraBytesToRead < sourceEnd);
		switch (extraBytesToRead)
		{
		case 5: ch += *source++; ch <<= 6;
		case 4: ch += *source++; ch <<= 6;
		case 3: ch += *source++; ch <<= 6;
		case 2: ch += *source++; ch <<= 6;
		case 1: ch += *source++; ch <<= 6;
		case 0: ch += *source++;
		}
		ch -= offsetsFromUTF8[extraBytesToRead];
		// Make sure it fits in a 16-bit wchar_t
		if (ch > 0xFFFF)
			ch = 0xFFFD;

		result += (wchar_t)ch;
	}
	return result;
}

// TODO: replace most of the asserts below (e.g. for when it fails to load
// a file) with some proper logging/reporting system

static AtSmartPtr<AtNode> ConvertNode(xmlNodePtr node);

AtObj AtlasObject::LoadFromXML(const std::string& xml)
{
	xmlDocPtr doc = xmlReadMemory(xml.c_str(), xml.length(), "noname.xml", NULL, XML_PARSE_NONET|XML_PARSE_NOCDATA);
	if (doc == NULL)
		return AtObj();
		// TODO: Need to report the error message somehow

	xmlNodePtr root = xmlDocGetRootElement(doc);
	AtObj obj;
	obj.p = ConvertNode(root);

	AtObj rootObj;
	rootObj.set((const char*)root->name, obj);

	xmlFreeDoc(doc);

	return rootObj;
}

// Convert from a DOMElement to an AtNode
static AtSmartPtr<AtNode> ConvertNode(xmlNodePtr node)
{
	AtSmartPtr<AtNode> obj (new AtNode());

	// Loop through all attributes
	for (xmlAttrPtr cur_attr = node->properties; cur_attr; cur_attr = cur_attr->next)
	{
		std::string name ("@");
		name += (const char*)cur_attr->name;
		xmlChar* content = xmlNodeGetContent(cur_attr->children);
		std::wstring value (fromXmlChar(content));
		xmlFree(content);
		
		AtNode* newNode = new AtNode(value.c_str());
		obj->children.insert(AtNode::child_pairtype(
			name.c_str(), AtNode::Ptr(newNode)
		));
	}

	// Loop through all child elements
	for (xmlNodePtr cur_node = node->children; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			obj->children.insert(AtNode::child_pairtype(
				(const char*)cur_node->name, ConvertNode(cur_node)
			));
		}
		else if (cur_node->type == XML_TEXT_NODE)
		{
			xmlChar* content = xmlNodeGetContent(cur_node);
			std::wstring value (fromXmlChar(content));
			xmlFree(content);
			obj->value += value;
		}
	}

	// Trim whitespace surrounding the string value
	const std::wstring whitespace = L" \t\r\n";
	size_t first = obj->value.find_first_not_of(whitespace);
	if (first == std::wstring::npos)
		obj->value = L"";
	else
	{
		size_t last = obj->value.find_last_not_of(whitespace);
		obj->value = obj->value.substr(first, 1+last-first);
	}

	return obj;
}

// Build a DOM node from a given AtNode
static void BuildDOMNode(xmlDocPtr doc, xmlNodePtr node, AtNode::Ptr p)
{
	if (p)
	{
		if (p->value.length())
			xmlNodeAddContent(node, toXmlChar(p->value));

		for (AtNode::child_maptype::const_iterator it = p->children.begin(); it != p->children.end(); ++it)
		{
			// Test for attribute nodes (whose names start with @)
			if (it->first.length() && it->first[0] == '@')
			{
				assert(it->second);
				assert(it->second->children.empty());
				xmlNewProp(node, (const xmlChar*)it->first.c_str()+1, toXmlChar(it->second->value));
			}
			else
			{
				if (node == NULL) // first node in the document - needs to be made the root node
				{
					xmlNodePtr root = xmlNewNode(NULL, (const xmlChar*)it->first.c_str());
					xmlDocSetRootElement(doc, root);
					BuildDOMNode(doc, root, it->second);
				}
				else
				{
					xmlNodePtr child = xmlNewChild(node, NULL, (const xmlChar*)it->first.c_str(), NULL);
					BuildDOMNode(doc, child, it->second);
				}
			}
		}
	}
}

std::string AtlasObject::SaveToXML(AtObj& obj)
{
	if (!obj.p || obj.p->children.size() != 1)
	{
		assert(! "SaveToXML: root must only have one child");
		return "";
	}

	AtNode::Ptr firstChild (obj.p->children.begin()->second);

	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
	BuildDOMNode(doc, NULL, obj.p);

	xmlChar* buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &buf, &size, "utf-8", 1);

	std::string ret((const char*)buf, size);

	xmlFree(buf);
	xmlFreeDoc(doc);

	// TODO: handle errors better

	return ret;
}
