/* Copyright (C) 2010 Wildfire Games.
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

#include "ParamNode.h"

#include "ps/XML/Xeromyces.h"

#include <sstream>

void CParamNode::LoadXML(CParamNode& ret, const XMBFile& xmb)
{
	ret.ApplyLayer(xmb, xmb.GetRoot());
}

PSRETURN CParamNode::LoadXMLString(CParamNode& ret, const char* xml)
{
	CXeromyces xero;
	PSRETURN ok = xero.LoadString(xml);
	if (ok != PSRETURN_OK)
		return ok;

	ret.ApplyLayer(xero, xero.GetRoot());

	return PSRETURN_OK;
}

void CParamNode::ApplyLayer(const XMBFile& xmb, const XMBElement& element)
{
	std::string name = xmb.GetElementString(element.GetNodeName()); // TODO: is GetElementString inefficient?
	utf16string value = element.GetText();

	// Add this element as a child node
	CParamNode& node = m_Childs[name];
	node.m_Value = std::wstring(value.begin(), value.end());

	// Recurse through the element's children
	XERO_ITER_EL(element, child)
	{
		node.ApplyLayer(xmb, child);
	}

	// Add the element's attributes, prefixing names with "@"
	XERO_ITER_ATTR(element, attr)
	{
		std::string attrName = xmb.GetAttributeString(attr.Name);
		std::wstring attrValue(attr.Value.begin(), attr.Value.end());
		node.m_Childs["@" + attrName].m_Value = attrValue;
	}

	// TODO: support some kind of 'delete' marker, for use with inheritance
}

const CParamNode* CParamNode::GetChild(const char* name) const
{
	ChildrenMap::const_iterator it = m_Childs.find(name);
	if (it == m_Childs.end())
		return NULL;
	return &it->second;
}

const std::wstring& CParamNode::ToString() const
{
	return m_Value;
}

int CParamNode::ToInt() const
{
	int ret;
	std::wstringstream strm;
	strm << m_Value;
	strm >> ret;
	return ret;
}

CFixed_23_8 CParamNode::ToFixed() const
{
	double ret;
	std::wstringstream strm;
	strm << m_Value;
	strm >> ret;
	return CFixed_23_8::FromDouble(ret);
	// TODO: this shouldn't use floating point types
}

bool CParamNode::ToBool() const
{
	if (m_Value == L"true")
		return true;
	else
		return false;
}

const CParamNode::ChildrenMap& CParamNode::GetChildren() const
{
	return m_Childs;
}

std::wstring CParamNode::EscapeXMLString(const std::wstring& str)
{
	std::wstring ret;
	ret.reserve(str.size());
	for (size_t i = 0; i < str.size(); ++i)
	{
		wchar_t c = str[i];
		switch (c)
		{
		case '<': ret += L"&lt;"; break;
		case '>': ret += L"&gt;"; break;
		case '&': ret += L"&amp;"; break;
		case '"': ret += L"&quot;"; break;
		default:
			if (c == 0x09 || c == 0x0A || c == 0x0D || (0x20 <= c && c <= 0xD7FF) || (0xE000 <= c && c <= 0xFFFD))
				ret += c;
			else
				ret += 0xFFFD;
		}
	}
	return ret;
}

std::wstring CParamNode::ToXML() const
{
	std::wstringstream strm;
	ToXML(strm);
	return strm.str();
}

void CParamNode::ToXML(std::wostream& strm) const
{
	strm << m_Value;

	ChildrenMap::const_iterator it = m_Childs.begin();
	for (; it != m_Childs.end(); ++it)
	{
		// Skip attributes here (they were handled when the caller output the tag)
		if (it->first.length() && it->first[0] == '@')
			continue;

		std::wstring name (it->first.begin(), it->first.end());

		strm << L"<" << name;

		// Output the child's attributes first
		ChildrenMap::const_iterator cit = it->second.m_Childs.begin();
		for (; cit != it->second.m_Childs.end(); ++cit)
		{
			if (cit->first.length() && cit->first[0] == '@')
			{
				std::wstring attrname (cit->first.begin()+1, cit->first.end());
				strm << L" " << attrname << L"=\"" << EscapeXMLString(cit->second.m_Value) << L"\"";
			}
		}

		strm << L">";

		it->second.ToXML(strm);

		strm << L"</" << name << ">";
	}
}
