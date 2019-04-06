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
	obj.m_Node = ConvertNode(rootnode);
	return obj;
}

// Convert from a JSON to an AtNode
static AtSmartPtr<AtNode> ConvertNode(json_spirit::Value node)
{
	AtSmartPtr<AtNode> obj (new AtNode());

	if (node.type() == json_spirit::str_type)
	{
		obj->m_Value = std::wstring(node.get_str().begin(),node.get_str().end());
	}
	else if (node.type() == json_spirit::int_type || node.type() == json_spirit::real_type)
	{
		std::wstringstream stream;
		if (node.type() == json_spirit::int_type)
			stream << node.get_int();
		if (node.type() == json_spirit::real_type)
			stream << node.get_real();

		obj->m_Value = stream.str().c_str();
		obj->m_Children.insert(AtNode::child_pairtype(
			"@number", AtSmartPtr<AtNode>(new AtNode())
		));
	}
	else if (node.type() == json_spirit::bool_type)
	{
		if (node.get_bool())
			obj->m_Value = L"true";
		else
			obj->m_Value = L"false";

		obj->m_Children.insert(AtNode::child_pairtype(
			"@boolean", AtSmartPtr<AtNode>(new AtNode())
		));
	}
	else if (node.type() == json_spirit::array_type)
	{
		obj->m_Children.insert(AtNode::child_pairtype(
			"@array", AtSmartPtr<AtNode>(new AtNode())
		));

		json_spirit::Array nodeChildren = node.get_array();
		json_spirit::Array::iterator itr = nodeChildren.begin();

		for (; itr != nodeChildren.end(); ++itr)
		{
			obj->m_Children.insert(AtNode::child_pairtype(
				"item", ConvertNode(*itr)
			));
		}
	}
	else if (node.type() == json_spirit::obj_type)
	{
		json_spirit::Object objectProperties = node.get_obj();
		json_spirit::Object::iterator itr = objectProperties.begin();
		for (; itr != objectProperties.end(); ++itr)
		{
			obj->m_Children.insert(AtNode::child_pairtype(
				itr->name_, ConvertNode(itr->value_)
			));
		}
	}
	else if (node.type() == json_spirit::null_type)
	{
		return obj;
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
	if (p->m_Children.count("@number"))
	{
		// Convert to double
		std::wstringstream str;
		str << p->m_Value;
		double val = 0;
		str >> val;

		json_spirit::Value rval(val);
		return rval;
	}
	else if (p->m_Children.count("@boolean"))
	{
		bool val = false;
		if (p->m_Value == L"true")
			val = true;

		json_spirit::Value rval(val);
		return rval;
	}

	// If no children, then use the value string instead
	if (p->m_Children.empty())
	{
		json_spirit::Value rval(std::string(p->m_Value.begin(), p->m_Value.end()));
		return rval;
	}

	if (p->m_Children.find("@array") != p->m_Children.end())
	{
		json_spirit::Array rval;

		// Find the <item> children
		AtNode::child_maptype::const_iterator lower = p->m_Children.lower_bound("item");
		AtNode::child_maptype::const_iterator upper = p->m_Children.upper_bound("item");

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

		for (AtNode::child_maptype::const_iterator it = p->m_Children.begin(); it != p->m_Children.end(); ++it)
		{
			json_spirit::Value child = BuildJSONNode(it->second);
			// We don't serialize childs with null value.
			// Instead of something like this we omit the whole property: "StartingCamera": null
			// There's no special reason for that, it's just the same behaviour the previous implementations had.
			if (child.type() != json_spirit::null_type)
				rval.push_back(json_spirit::Pair(it->first.c_str(), child));
		}

		return rval;
	}
}

std::string AtlasObject::SaveToJSON(AtObj& obj)
{
	json_spirit::Value root = BuildJSONNode(obj.m_Node);
	return json_spirit::write_string(root, 0);
}
