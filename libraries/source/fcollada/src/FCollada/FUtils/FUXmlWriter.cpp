/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUXmlWriter.h"
#include "FUXmlParser.h"
#include "FUStringConversion.h"

#define xcT(text) (const xmlChar*) (text)

// To avoid a nasty 'if' for performance reasons, these are tables of the valid characters
static const bool filenameValidityTable[256] =
{
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 0-15
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 16-31
	false, false, false, false, false, false, false, false, false, false, false, false, false, true , true , true , // 32-47 [45-47 is '-./']
	true , true , true , true , true , true , true , true , true , true , true , false, false, false, false, false, // 48-63 [48-57 are numerals, 58 is ':']
	false, true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , // 64-79 [65-79 is 'A'-'O']
	true , true , true , true , true , true , true , true , true , true , true , false, false, false, false, true , // 80-95 [80-90 is 'P'-'Z'], 95 is '_']
	false, true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , // 96-111 [97-111 is 'a'-'o']
	true , true , true , true , true , true , true , true , true , true , true , false, false, false, false, false, // 112-127 [112-122 is 'p'-'z']
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 128-143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 144-159
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 160-175
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 176-191
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 192-207
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 208-223
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 224-239
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false  // 240-255
};

static const bool xmlValidityTable[256] =
{
	false, false, false, false, false, false, false, false, false, false, true , false, false, true , false, false, // 0-15 [10 is \n, 13 is \r]
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 16-31
	true , true , false, true , true , true , true , false, true , true , true , true , true , true , true , true , // 32-47 [32-33 is ' !', 35-38 is '#$%&', 40-47 is '()*+,-./']
	true , true , true , true , true , true , true , true , true , true , true , true , false, true , false, true , // 48-63 [48-57 are numerals, 58-59 is ':;', 61 is '=', 63 is '?']
	false, true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , // 64-79 [65-79 is 'A'-'O']
	true , true , true , true , true , true , true , true , true , true , true , true , false, true , false, true , // 80-95 [80-90 is 'P'-'Z', 91 is '[', 93 is ']', 95 is '_']
	false, true , true , true , true , true , true , true , true , true , true , true , true , true , true , true , // 96-111 [97-111 is 'a'-'o']
	true , true , true , true , true , true , true , true , true , true , true , true , true , true , false, false, // 112-127 [112-122 is 'p'-'z', 123-125 is '{|}']
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 128-143
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 144-159
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 160-175
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 176-191
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 192-207
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 208-223
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 224-239
	false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false  // 240-255
};

namespace FUXmlWriter
{

	// Create a new XML tree node
	xmlNode* CreateNode(const char* name)
	{
		return xmlNewNode(NULL, xcT(name));
	}

	void RenameNode(xmlNode* node, const char* newName)
	{
		xmlNodeSetName(node, xcT(newName));
	}

	// Create a new XML tree child node, parented to the given XML tree node
	void AddChild(xmlNode* parent, xmlNode* child)
	{
		xmlAddChild(parent, child);
	}

	xmlNode* AddChild(xmlNode* parent, const char* name)
	{
		return (parent != NULL) ? xmlNewChild(parent, NULL, xcT(name), NULL) : NULL;
	}

	xmlNode* AddChild(xmlNode* parent, const char* name, const char* content)
	{
		xmlNode* node = AddChild(parent, name);
		if (node != NULL && content != NULL && *content != 0) AddContent(node, content);
		return node;
	}

#ifdef UNICODE
	xmlNode* AddChild(xmlNode* parent, const char* name, const fstring& content)
	{
		fm::string s = FUStringConversion::ToString(content);
		return AddChild(parent, name, !s.empty() ? s.c_str() : NULL);
	}
#endif

	xmlNode* AddChildOnce(xmlNode* parent, const char* name, const char* content)
	{
		xmlNode* node = NULL;
		if (parent != NULL)
		{
			node = FUXmlParser::FindChildByType(parent, name);
			if (node == NULL) node = AddChild(parent, name, (content == NULL || *content == 0) ? NULL : content);
		}
		return node;
	}

	// Create/Append a new XML tree node, as a sibling to a given XML tree node
	void AddSibling(xmlNode* sibling, xmlNode* dangling)
	{
		xmlAddSibling(sibling, dangling);
	}

	xmlNode* AddSibling(xmlNode* node, const char* name)
	{
		xmlNode* n = CreateNode(name);
		AddSibling(node, n);
		return n;
	}

	xmlNode* InsertChild(xmlNode* parent, xmlNode* sibling, const char* name)
	{
		xmlNode* n;
		if (sibling == NULL || sibling->parent != parent)
		{
			n = AddChild(parent, name);
		}
		else
		{
			n = xmlAddPrevSibling(sibling, CreateNode(name));
		}
		return n;
	}

	void AddContent(xmlNode* node, const char* content)
	{
		if (node != NULL)
		{
			// Look for non-UTF8 characters that will need to be converted to %XXXX.
			FUSStringBuilder xmlSBuilder;
			const char* str = content;
			char c;
			for (; (c = *str) != 0; ++str)
			{
				// For performance reason, use a look-up table.
				if (xmlValidityTable[(uint8) c])
				{
					// Valid characters
					xmlSBuilder.append(c);
				}
				else
				{
					// Remove any invalid character(s) in the filename using the %X guideline
					xmlSBuilder.append((char) '%');
					xmlSBuilder.appendHex((uint8) c);
				}
			}

			xmlNodeAddContent(node, xcT(xmlSBuilder.ToCharPtr()));
		}
	}

#ifdef UNICODE
	void AddContent(xmlNode* node, const fstring& content)
	{
		fm::string s = FUStringConversion::ToString(content);
		AddContent(node, s.c_str());
	}
#endif

	// Converts all the spaces into %20.
	void ConvertFilename(fstring& str)
	{
		FUStringBuilder xmlBuilder;
		const fchar* s = str.c_str();
		fchar c;
		for (; (c = *s) != 0; ++s)
		{
			if (filenameValidityTable[(uint8) c]) xmlBuilder.append(c);
			else
			{
				xmlBuilder.append((fchar) '%');
				xmlBuilder.appendHex((uint8) c);
			}
		}
		str = xmlBuilder.ToString();
	}
	
	void AddContentUnprocessed(xmlNode* node, const char* content)
	{
		if (node != NULL) xmlNodeAddContent(node, xcT(content));
	}

	void AddAttribute(xmlNode* node, const char* attributeName, const char* value)
	{
		if (node != NULL)
		{
			xmlNewProp(node, xcT(attributeName), xcT(value));
		}
	}

#ifdef UNICODE
	void AddAttribute(xmlNode* node, const char* attributeName, const fstring& attributeValue)
	{
		fm::string s = FUStringConversion::ToString(attributeValue);
		AddAttribute(node, attributeName, s.c_str());
	}
#endif


	void RemoveAttribute(xmlNode* node, const char* attributeName)
	{
		xmlAttrPtr ptr = xmlHasProp(node, xcT(attributeName));
		xmlRemoveProp(ptr);
	}


	// Insert a child, respecting lexical ordering
	void AddChildSorted(xmlNode* parent, xmlNode* child)
	{
		// Do an insertion sort in alphabetical order of the element names. 
		// Walk backward from the last child, to make sure that
		// the chronological ordering of elements of the same type is respected.
		//
		for (xmlNode* p = xmlGetLastChild(parent); p != NULL; p = p->prev)
		{
			if (p->type != XML_ELEMENT_NODE) continue;
			if (strcmp((const char*) p->name, (const char*) child->name) <= 0)
			{
				xmlAddNextSibling(p, child);
				return;
			}
		}

		// Add to the top of the list.
		if (parent->children && parent->children->type == XML_ELEMENT_NODE)
		{
			xmlAddPrevSibling(parent->children, child);
		}
		else
		{
			AddChild(parent, child);
		}
	}

	xmlNode* AddChildSorted(xmlNode* parent, const char* name, const char* content)
	{
		xmlNode* node = CreateNode(name);
		if (content != NULL && *content != 0) AddContent(node, content);
		AddChildSorted(parent, node);
		return node;
	}

	void ReParentNode(xmlNode* node, xmlNode* newParent)
	{
		xmlUnlinkNode(node);
		AddChild(newParent, node);
	}
};
