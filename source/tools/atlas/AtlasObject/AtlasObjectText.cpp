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

#include "AtlasObjectText.h"
#include "AtlasObjectImpl.h"
#include "AtlasObject.h"

static std::wstring ConvertRecursive(const AtNode::Ptr obj, bool use_brackets = true)
{
	// Convert (1, ()) into "1"
	// Convert (3, (d: (...), e: (...))) into "3 (conv(...), conv(...))"
	// etc
	// resulting in data-loss [because of the key names], and a rather arbitrary
	// [alphabetical by key] ordering of children, but at least it's fairly readable

	if (! obj)
		return L"";

	std::wstring result;

	bool has_value = !obj->m_Value.empty();
	bool has_children = !obj->m_Children.empty();

	if (has_value && has_children)
		result = obj->m_Value + L" ";
	else if (has_value)
		result = obj->m_Value;
	// else no value; result = L""

	if (has_children)
	{
		if (use_brackets)
			result += L"(";

		bool first_child = true; // so we can add ", " in appropriate places

		for (const AtNode::child_maptype::value_type& child : obj->m_Children)
		{
			if (!first_child)
				result += L", ";
			else
				first_child = false;

			result += ConvertRecursive(child.second);
		}

		if (use_brackets)
			result += L")";
	}

	return result;
}

std::wstring AtlasObject::ConvertToString(const AtObj& obj)
{
	return ConvertRecursive(obj.m_Node, false);
}
