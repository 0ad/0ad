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

#include "ps/CStr.h"
#include "ps/XML/Xeromyces.h"

#include "js/jsapi.h"

#include <sstream>

// Disable "'boost::algorithm::detail::is_classifiedF' : assignment operator could not be generated"
#if MSC_VERSION
#pragma warning(disable:4512)
#endif

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>	// this isn't in string.hpp in old Boosts

static CParamNode g_NullNode(false);

CParamNode::CParamNode(bool isOk) :
	m_IsOk(isOk)
{
}

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

	bool hasSetValue = false;

	// Look for special attributes
	int at_disable = xmb.GetAttributeID("disable");
	int at_replace = xmb.GetAttributeID("replace");
	int at_datatype = xmb.GetAttributeID("datatype");
	{
		XERO_ITER_ATTR(element, attr)
		{
			if (attr.Name == at_disable)
			{
				m_Childs.erase(name);
				return;
			}
			else if (attr.Name == at_replace)
			{
				m_Childs.erase(name);
				break;
			}
			else if (attr.Name == at_datatype && std::wstring(attr.Value.begin(), attr.Value.end()) == L"tokens")
			{
				CParamNode& node = m_Childs[name];
				std::wstring newValue(value.begin(), value.end());
				std::vector<std::wstring> oldTokens;
				std::vector<std::wstring> newTokens;
				if (!node.m_Value.empty())
					boost::algorithm::split(oldTokens, node.m_Value, boost::algorithm::is_space());
				if (!newValue.empty())
					boost::algorithm::split(newTokens, newValue, boost::algorithm::is_space());

				std::vector<std::wstring> tokens = oldTokens;
				for (size_t i = 0; i < newTokens.size(); ++i)
				{
					if (newTokens[i][0] == L'-')
						tokens.erase(std::find(tokens.begin(), tokens.end(), newTokens[i].substr(1)));
					else
						if (std::find(oldTokens.begin(), oldTokens.end(), newTokens[i]) == oldTokens.end())
							tokens.push_back(newTokens[i]);
				}

				node.m_Value = boost::algorithm::join(tokens, L" ");
				hasSetValue = true;
				break;
			}
		}
	}

	// Add this element as a child node
	CParamNode& node = m_Childs[name];
	if (!hasSetValue)
		node.m_Value = std::wstring(value.begin(), value.end());

	// Recurse through the element's children
	XERO_ITER_EL(element, child)
	{
		node.ApplyLayer(xmb, child);
	}

	// Add the element's attributes, prefixing names with "@"
	XERO_ITER_ATTR(element, attr)
	{
		// Skip special attributes
		if (attr.Name == at_replace) continue;
		// Add any others
		std::string attrName = xmb.GetAttributeString(attr.Name);
		std::wstring attrValue(attr.Value.begin(), attr.Value.end());
		node.m_Childs["@" + attrName].m_Value = attrValue;
	}
}

void CParamNode::CopyFilteredChildrenOfChild(const CParamNode& src, const char* name, const std::set<std::string>& permitted)
{
	ChildrenMap::iterator dstChild = m_Childs.find(name);
	ChildrenMap::const_iterator srcChild = src.m_Childs.find(name);
	if (dstChild == m_Childs.end() || srcChild == src.m_Childs.end())
		return; // error

	ChildrenMap::const_iterator it = srcChild->second.m_Childs.begin();
	for (; it != srcChild->second.m_Childs.end(); ++it)
		if (permitted.count(it->first))
			dstChild->second.m_Childs[it->first] = it->second;
}

const CParamNode& CParamNode::GetChild(const char* name) const
{
	ChildrenMap::const_iterator it = m_Childs.find(name);
	if (it == m_Childs.end())
		return g_NullNode;
	return it->second;
}

bool CParamNode::IsOk() const
{
	return m_IsOk;
}

const std::wstring& CParamNode::ToString() const
{
	return m_Value;
}

const std::string CParamNode::ToASCIIString() const
{
	return CStr8(m_Value);
}

int CParamNode::ToInt() const
{
	int ret = 0;
	std::wstringstream strm;
	strm << m_Value;
	strm >> ret;
	return ret;
}

fixed CParamNode::ToFixed() const
{
	return fixed::FromString(CStrW(m_Value));
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
		case '\t': ret += L"&#9;"; break;
		case '\n': ret += L"&#10;"; break;
		case '\r': ret += L"&#13;"; break;
		default:
			if ((0x20 <= c && c <= 0xD7FF) || (0xE000 <= c && c <= 0xFFFD))
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

void CParamNode::SetScriptVal(CScriptValRooted val) const
{
	debug_assert(JSVAL_IS_VOID(m_ScriptVal.get()));
	m_ScriptVal = val;
}

CScriptValRooted CParamNode::GetScriptVal() const
{
	return m_ScriptVal;
}
