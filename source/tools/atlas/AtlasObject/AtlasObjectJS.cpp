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

#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include "JSONSpiritInclude.h"

#if defined(_MSC_VER)
# pragma warning(disable:4996)	// deprecated CRT
#endif

#include "wx/log.h"

#include <sstream>

static AtSmartPtr<AtNode> ConvertNode(json_spirit::Value node);

AtObj AtlasObject::LoadFromJSON(const std::string& json)
{
	json_spirit::Value rootnode;
	json_spirit::read_string(json, rootnode);
	
	AtObj obj;
	obj.p = ConvertNode(rootnode);
	return obj;
}

// Convert from a JSON to an AtNode
static AtSmartPtr<AtNode> ConvertNode(json_spirit::Value node)
{
	AtSmartPtr<AtNode> obj (new AtNode());
	
	if (node.type() == json_spirit::str_type)
	{
		obj->value = std::wstring(node.get_str().begin(),node.get_str().end());
	}
	else if (node.type() == json_spirit::int_type || node.type() == json_spirit::real_type)
	{
		std::wstringstream stream;
		if (node.type() == json_spirit::int_type)
			stream << node.get_int();
		if (node.type() == json_spirit::real_type)
			stream << node.get_real();
		
		obj->value = stream.str().c_str();
		obj->children.insert(AtNode::child_pairtype(
			"@number", AtSmartPtr<AtNode>(new AtNode())
		));
	}
	else if (node.type() == json_spirit::bool_type)
	{
		if (node.get_bool())
			obj->value = L"true";
		else
			obj->value = L"false";
		
		obj->children.insert(AtNode::child_pairtype(
			"@boolean", AtSmartPtr<AtNode>(new AtNode())
		));
	}
	else if (node.type() == json_spirit::array_type)
	{
		obj->children.insert(AtNode::child_pairtype(
			"@array", AtSmartPtr<AtNode>(new AtNode())
		));

		json_spirit::Array nodeChildren = node.get_array();
		json_spirit::Array::iterator itr = nodeChildren.begin();
		
		for (; itr != nodeChildren.end(); itr++)
		{
			obj->children.insert(AtNode::child_pairtype(
				"item", ConvertNode(*itr)
			));
		}
	}
	else if (node.type() == json_spirit::obj_type)
	{
		json_spirit::Object objectProperties = node.get_obj();
		json_spirit::Object::iterator itr = objectProperties.begin();
		for (; itr != objectProperties.end(); itr++)
		{
			obj->children.insert(AtNode::child_pairtype(
				itr->name_, ConvertNode(itr->value_)
			));
		}
	}
	else
	{
		assert(! "Unimplemented type found when parsing JSON!");
	}
	
	return obj;
}


json_spirit::Value BuildJSONNode(AtNode::Ptr p)
{
	if (!p)
	{
		json_spirit::Value rval;
		return rval;
	}

	// Special case for numbers/booleans to allow round-tripping
	if (p->children.count("@number"))
	{
		// Convert to double
		std::wstringstream str;
		str << p->value;
		double val = 0;
		str >> val;

		json_spirit::Value rval(val);
		return rval;
	}
	else if (p->children.count("@boolean"))
	{
		bool val = false;
		if (p->value == L"true")
			val = true;
		
		json_spirit::Value rval(val);
		return rval;
	}

	// If no children, then use the value string instead
	if (p->children.empty())
	{
		json_spirit::Value rval(std::string(p->value.begin(), p->value.end()));
		return rval;
	}

	if (p->children.find("@array") != p->children.end())
	{
		json_spirit::Array rval;

		// Find the <item> children
		AtNode::child_maptype::const_iterator lower = p->children.lower_bound("item");
		AtNode::child_maptype::const_iterator upper = p->children.upper_bound("item");

		unsigned int idx = 0;
		for (AtNode::child_maptype::const_iterator it = lower; it != upper; ++it)
		{
			json_spirit::Value child = BuildJSONNode(it->second);
			rval.push_back(child);

			++idx;
		}

		return rval;
	}
	else
	{
		json_spirit::Object rval;
		
		for (AtNode::child_maptype::const_iterator it = p->children.begin(); it != p->children.end(); ++it)
		{
			json_spirit::Value child = BuildJSONNode(it->second);
			rval.push_back(json_spirit::Pair(it->first.c_str(), child));
		}

		return rval;
	}
}

std::string AtlasObject::SaveToJSON(AtObj& obj)
{
	json_spirit::Value root = BuildJSONNode(obj.p);
	std::string ret = json_spirit::write_string(root, 0);
	return ret;
}
