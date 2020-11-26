/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUXmlParser.h"

#define xmlT(a) (const xmlChar*) (a)

namespace FUXmlParser
{
	// Convert a XML string to a text string: handles the '%' character
	fm::string XmlToString(const char* s)
	{
		// Replace any '%' character string into the wanted characters: %20 is common.
		FUSStringBuilder xmlSBuilder;
		char c;
		while ((c = *s) != 0)
		{
			if (c != '%')
			{
				xmlSBuilder.append(c);
				++s;
			}
			else
			{
				++s; // skip the '%' character
				uint32 value = FUStringConversion::HexToUInt32(&s, 2);
				xmlSBuilder.append((char) value);
			}
		}
		return xmlSBuilder.ToString();
	}

#ifdef UNICODE
	// Convert a XML string to a text string: handles the '%' character
	fstring XmlToString(const fchar* s)
	{
		// Replace any '%' character string into the wanted characters: %20 is common.
		FUStringBuilder xmlBuilder;
		fchar c;
		while ((c = *s) != 0)
		{
			if (c != (fchar) '%')
			{
				xmlBuilder.append(c);
				++s;
			}
			else
			{
				++s; // skip the '%' character
				uint32 value = FUStringConversion::HexToUInt32(&s, 2);
				xmlBuilder.append((fchar) value);
			}
		}
		return xmlBuilder.ToString();
	}
#endif // UNICODE

	// Returns the first child node of a given type
	xmlNode* FindChildByType(xmlNode* parent, const char* type)
	{
		if (parent != NULL)
		{
			for (xmlNode* child = parent->children; child != NULL; child = child->next)
			{
				if (child->type == XML_ELEMENT_NODE)
				{
					if (IsEquivalent(child->name, type)) return child;
				}
			}
		}
		return NULL;
	}

	// return the first child node of a given name
	xmlNode* FindChildByName(xmlNode* parent, const char* name)
	{
		if (parent != NULL)
		{
			for (xmlNode* child = parent->children; child != NULL; child = child->next)
			{
				if (child->type == XML_ELEMENT_NODE)
				{
					fm::string str_name = ReadNodeProperty(child, "name");
					if (str_name == name) return child;
				}
			}
		}
		return NULL;
	}
	
	
	// return the first child node of a given property value
	xmlNode* FindChildByProperty(xmlNode* parent, const char* prop, const char* val)
	{
		if (parent != NULL)
		{
			for (xmlNode* child = parent->children; child != NULL; child = child->next)
			{
				fm::string str_pop = ReadNodeProperty(child, prop);
				if (str_pop == val) return child;
				
			}
		}
		return NULL;
	}

	// return the first node in list of a given property
	xmlNode* FindNodeInListByProperty(xmlNodeList list, const char* property, const char* prop)
	{
		for (xmlNodeList::iterator it = list.begin(); it != list.end(); ++it)
		{
			xmlNode* element = *it;
			fm::string str_prop = ReadNodeProperty(element, property);
			if (str_prop == prop) return element;
		
		}
		return NULL;
	}

	// Retrieves all the child nodes of a given type
	void FindChildrenByType(xmlNode* parent, const char* type, xmlNodeList& nodes)
	{
		if (parent != NULL)
		{
			for (xmlNode* child = parent->children; child != NULL; child = child->next)
			{
				if (child->type == XML_ELEMENT_NODE)
				{
					if (IsEquivalent(child->name, type)) nodes.push_back(child);
				} 
			}
		}
	}

	// Returns whether the given node has the given property
	bool HasNodeProperty(xmlNode* node, const char* property)
	{
		xmlAttr* attribute = xmlHasProp(node, xmlT(property));
		return attribute != NULL;
	}

	// Returns the string value of a node's property
	fm::string ReadNodeProperty(xmlNode* node, const char* property)
	{
		if (node != NULL && property != NULL)
		{
			xmlChar* data = xmlGetProp(node, xmlT(property));
			if (data != NULL)
			{
				// Process the string for special characters
				fm::string ret = XmlToString((const char*) data);
				xmlFree(data);
				return ret;
			}
		}

		return emptyString;
	}

	// Returns the CRC value of a node's property
	FUCrc32::crc32 ReadNodePropertyCRC(xmlNode* node, const char* property)
	{
		FUCrc32::crc32 ret = 0;
		if (node != NULL && property != NULL)
		{
			xmlChar* data = xmlGetProp(node, xmlT(property));
			if (data != NULL)
			{
				ret = FUCrc32::CRC32((const char*) data);
				xmlFree(data);
			}
		}
		return ret;
	}

	// Returns the text content directly attached to a node
	const char* ReadNodeContentDirect(xmlNode* node)
	{
		if (node == NULL || node->children == NULL
			|| node->children->type != XML_TEXT_NODE || node->children->content == NULL) return emptyCharString;
		return (const char*) node->children->content;
	}

	// Returns the inner text content of a node
	fm::string ReadNodeContentFull(xmlNode* node)
	{
		if (node != NULL)
		{
			xmlChar* content = xmlNodeGetContent(node);
			if (content != NULL)
			{
				// Process the string for special characters
				fm::string ret = XmlToString((const char*) content);
				xmlFree(content);
				return ret;
			}
		}
		return emptyString;
	}
};
