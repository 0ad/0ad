/* Copyright (C) 2021 Wildfire Games.
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
#include <libxml/parser.h>
#include <string>

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
	obj.m_Node = ConvertNode(root);

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
		name += reinterpret_cast<const char*>(cur_attr->name);
		xmlChar* content = xmlNodeGetContent(cur_attr->children);
		std::string value = reinterpret_cast<char*>(content);
		xmlFree(content);

		AtNode* newNode = new AtNode(value.c_str());
		obj->m_Children.insert(AtNode::child_pairtype(
			name.c_str(), AtNode::Ptr(newNode)
		));
	}

	// Loop through all child elements
	for (xmlNodePtr cur_node = node->children; cur_node; cur_node = cur_node->next)
	{
		if (cur_node->type == XML_ELEMENT_NODE)
		{
			obj->m_Children.insert(AtNode::child_pairtype(
				reinterpret_cast<const char*>(cur_node->name), ConvertNode(cur_node)
			));
		}
		else if (cur_node->type == XML_TEXT_NODE)
		{
			xmlChar* content = xmlNodeGetContent(cur_node);
			std::string value = reinterpret_cast<char*>(content);
			xmlFree(content);
			obj->m_Value += value;
		}
	}

	// Trim whitespace surrounding the string value
	const std::string whitespace = " \t\r\n";
	size_t first = obj->m_Value.find_first_not_of(whitespace);
	if (first == std::string::npos)
		obj->m_Value = "";
	else
	{
		size_t last = obj->m_Value.find_last_not_of(whitespace);
		obj->m_Value = obj->m_Value.substr(first, 1+last-first);
	}

	return obj;
}

// Build a DOM node from a given AtNode
static void BuildDOMNode(xmlDocPtr doc, xmlNodePtr node, AtNode::Ptr p)
{
	if (p)
	{
		if (p->m_Value.length())
			xmlNodeAddContent(node, reinterpret_cast<const xmlChar*>(p->m_Value.c_str()));

		for (const AtNode::child_maptype::value_type& child : p->m_Children)
		{
			const xmlChar* first_child = reinterpret_cast<const xmlChar*>(child.first.c_str());

			// Test for attribute nodes (whose names start with @)
			if (child.first.length() && child.first[0] == '@')
			{
				assert(child.second);
				assert(child.second->m_Children.empty());
				xmlNewProp(node, first_child + 1, reinterpret_cast<const xmlChar*>(child.second->m_Value.c_str()));
			}
			else
			{
				// First node in the document - needs to be made the root node
				if (node == nullptr)
				{
					xmlNodePtr root = xmlNewNode(nullptr, first_child);
					xmlDocSetRootElement(doc, root);
					BuildDOMNode(doc, root, child.second);
				}
				else
				{
					xmlNodePtr newChild = xmlNewChild(node, nullptr, first_child, nullptr);
					BuildDOMNode(doc, newChild, child.second);
				}
			}
		}
	}
}

std::string AtlasObject::SaveToXML(AtObj& obj)
{
	if (!obj.m_Node || obj.m_Node->m_Children.size() != 1)
	{
		assert(! "SaveToXML: root must only have one child");
		return "";
	}

	AtNode::Ptr firstChild (obj.m_Node->m_Children.begin()->second);

	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
	BuildDOMNode(doc, nullptr, obj.m_Node);

	xmlChar* buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &buf, &size, "utf-8", 1);

	std::string ret((const char*)buf, size);

	xmlFree(buf);
	xmlFreeDoc(doc);

	// TODO: handle errors better

	return ret;
}
