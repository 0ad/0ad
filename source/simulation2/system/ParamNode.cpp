/* Copyright (C) 2017 Wildfire Games.
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

#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>	// this isn't in string.hpp in old Boosts

static CParamNode g_NullNode(false);

CParamNode::CParamNode(bool isOk) :
	m_IsOk(isOk)
{
}

void CParamNode::LoadXML(CParamNode& ret, const XMBFile& xmb, const wchar_t* sourceIdentifier /*= NULL*/)
{
	ret.ApplyLayer(xmb, xmb.GetRoot(), sourceIdentifier);
}

void CParamNode::LoadXML(CParamNode& ret, const VfsPath& path, const std::string& validatorName)
{
	CXeromyces xero;
	PSRETURN ok = xero.Load(g_VFS, path, validatorName);
	if (ok != PSRETURN_OK)
		return; // (Xeromyces already logged an error)

	LoadXML(ret, xero, path.string().c_str());
}

PSRETURN CParamNode::LoadXMLString(CParamNode& ret, const char* xml, const wchar_t* sourceIdentifier /*=NULL*/)
{
	CXeromyces xero;
	PSRETURN ok = xero.LoadString(xml);
	if (ok != PSRETURN_OK)
		return ok;

	ret.ApplyLayer(xero, xero.GetRoot(), sourceIdentifier);

	return PSRETURN_OK;
}

void CParamNode::ApplyLayer(const XMBFile& xmb, const XMBElement& element, const wchar_t* sourceIdentifier /*= NULL*/)
{
	ResetScriptVal();

	std::string name = xmb.GetElementString(element.GetNodeName()); // TODO: is GetElementString inefficient?
	CStrW value = element.GetText().FromUTF8();

	bool hasSetValue = false;

	// Look for special attributes
	int at_disable = xmb.GetAttributeID("disable");
	int at_replace = xmb.GetAttributeID("replace");
	int at_filtered = xmb.GetAttributeID("filtered");
	int at_merge = xmb.GetAttributeID("merge");
	int at_op = xmb.GetAttributeID("op");
	int at_datatype = xmb.GetAttributeID("datatype");
	enum op {
		INVALID,
		ADD,
		MUL,
		MUL_ROUND
	} op = INVALID;
	bool replacing = false;
	bool filtering = false;
	bool merging = false;
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
				replacing = true;
			}
			else if (attr.Name == at_filtered)
			{
				filtering = true;
			}
			else if (attr.Name == at_merge)
			{
				if (m_Childs.find(name) == m_Childs.end())
					return;
				merging = true;
			}
			else if (attr.Name == at_op)
			{
				if (std::wstring(attr.Value.begin(), attr.Value.end()) == L"add")
					op = ADD;
				else if (std::wstring(attr.Value.begin(), attr.Value.end()) == L"mul")
					op = MUL;
				else if (std::wstring(attr.Value.begin(), attr.Value.end()) == L"mul_round")
					op = MUL_ROUND;
				else
					LOGWARNING("Invalid op '%ls'", attr.Value);
			}
		}
	}
	{
		XERO_ITER_ATTR(element, attr)
		{
			if (attr.Name == at_datatype && std::wstring(attr.Value.begin(), attr.Value.end()) == L"tokens")
			{
				CParamNode& node = m_Childs[name];

				// Split into tokens
				std::vector<std::wstring> oldTokens;
				std::vector<std::wstring> newTokens;
				if (!replacing && !node.m_Value.empty()) // ignore the old tokens if replace="" was given
					boost::algorithm::split(oldTokens, node.m_Value, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
				if (!value.empty())
					boost::algorithm::split(newTokens, value, boost::algorithm::is_space(), boost::algorithm::token_compress_on);

				// Merge the two lists
				std::vector<std::wstring> tokens = oldTokens;
				for (size_t i = 0; i < newTokens.size(); ++i)
				{
					if (newTokens[i][0] == L'-')
					{
						std::vector<std::wstring>::iterator tokenIt = std::find(tokens.begin(), tokens.end(), newTokens[i].substr(1));
						if (tokenIt != tokens.end())
							tokens.erase(tokenIt);
						else
							LOGWARNING("[ParamNode] Could not remove token '%s' from node '%s'%s; not present in list nor inherited (possible typo?)",
								utf8_from_wstring(newTokens[i].substr(1)), name, sourceIdentifier ? (" in '" + utf8_from_wstring(sourceIdentifier) + "'").c_str() : "");
					}
					else
					{
						if (std::find(oldTokens.begin(), oldTokens.end(), newTokens[i]) == oldTokens.end())
							tokens.push_back(newTokens[i]);
					}
				}

				node.m_Value = boost::algorithm::join(tokens, L" ");
				hasSetValue = true;
				break;
			}
		}
	}

	// Add this element as a child node
	CParamNode& node = m_Childs[name];
	if (op != INVALID)
	{
		// TODO: Support parsing of data types other than fixed; log warnings in other cases
		fixed oldval = node.ToFixed();
		fixed mod = fixed::FromString(CStrW(value));

		switch (op)
		{
		case ADD:
			node.m_Value = (oldval + mod).ToString().FromUTF8();
			break;
		case MUL:
			node.m_Value = oldval.Multiply(mod).ToString().FromUTF8();
			break;
		case MUL_ROUND:
			node.m_Value = fixed::FromInt(oldval.Multiply(mod).ToInt_RoundToNearest()).ToString().FromUTF8();
			break;
		}
		hasSetValue = true;
	}

	if (!hasSetValue && !merging)
		node.m_Value = value;

	// We also need to reset node's script val, even if it has no children
	// or if the attributes change.
	node.ResetScriptVal();

	// For the filtered case
	ChildrenMap childs;

	// Recurse through the element's children
	XERO_ITER_EL(element, child)
	{
		node.ApplyLayer(xmb, child, sourceIdentifier);
		if (filtering)
		{
			std::string childname = xmb.GetElementString(child.GetNodeName());
			if (node.m_Childs.find(childname) != node.m_Childs.end())
				childs[childname] = std::move(node.m_Childs[childname]);
		}
	}

	if (filtering)
		node.m_Childs.swap(childs);

	// Add the element's attributes, prefixing names with "@"
	XERO_ITER_ATTR(element, attr)
	{
		// Skip special attributes
		if (attr.Name == at_replace || attr.Name == at_op || attr.Name == at_merge || attr.Name == at_filtered)
			continue;
		// Add any others
		std::string attrName = xmb.GetAttributeString(attr.Name);
		node.m_Childs["@" + attrName].m_Value = attr.Value.FromUTF8();
	}
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

const std::string CParamNode::ToUTF8() const
{
	return utf8_from_wstring(m_Value);
}

const CStrIntern CParamNode::ToUTF8Intern() const
{
	return CStrIntern(utf8_from_wstring(m_Value));
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

float CParamNode::ToFloat() const
{
	float ret = 0;
	std::wstringstream strm;
	strm << m_Value;
	strm >> ret;
	return ret;
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

void CParamNode::ToJSVal(JSContext* cx, bool cacheValue, JS::MutableHandleValue ret) const
{
	if (cacheValue && m_ScriptVal != NULL)
	{
		ret.set(*m_ScriptVal);
		return;
	}

	ConstructJSVal(cx, ret);

	if (cacheValue)
		m_ScriptVal.reset(new JS::PersistentRootedValue(cx, ret));
}

void CParamNode::ConstructJSVal(JSContext* cx, JS::MutableHandleValue ret) const
{
	JSAutoRequest rq(cx);
	if (m_Childs.empty())
	{
		// Empty node - map to undefined
		if (m_Value.empty())
		{
			ret.setUndefined();
			return;
		}

		// Just a string
		utf16string text(m_Value.begin(), m_Value.end());
		JS::RootedString str(cx, JS_InternUCStringN(cx, reinterpret_cast<const char16_t*>(text.data()), text.length()));
		if (str)
		{
			ret.setString(str);
			return;
		}
		// TODO: report error
		ret.setUndefined();
		return;
	}

	// Got child nodes - convert this node into a hash-table-style object:

	JS::RootedObject obj(cx, JS_NewPlainObject(cx));
	if (!obj)
	{
		ret.setUndefined();
		return; // TODO: report error
	}

	JS::RootedValue childVal(cx);
	for (std::map<std::string, CParamNode>::const_iterator it = m_Childs.begin(); it != m_Childs.end(); ++it)
	{
		it->second.ConstructJSVal(cx, &childVal);
		if (!JS_SetProperty(cx, obj, it->first.c_str(), childVal))
		{
			ret.setUndefined();
			return; // TODO: report error
		}
	}

	// If the node has a string too, add that as an extra property
	if (!m_Value.empty())
	{
		utf16string text(m_Value.begin(), m_Value.end());
		JS::RootedString str(cx, JS_InternUCStringN(cx, reinterpret_cast<const char16_t*>(text.data()), text.length()));
		if (!str)
		{
			ret.setUndefined();
			return; // TODO: report error
		}

		JS::RootedValue childVal(cx, JS::StringValue(str));
		if (!JS_SetProperty(cx, obj, "_string", childVal))
		{
			ret.setUndefined();
			return; // TODO: report error
		}
	}

	ret.setObject(*obj);
}

void CParamNode::ResetScriptVal()
{
	m_ScriptVal = NULL;
}
