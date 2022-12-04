/* Copyright (C) 2022 Wildfire Games.
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
#include "ps/CStrIntern.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptRequest.h"

#include <sstream>
#include <string_view>

#include <boost/algorithm/string.hpp>

static CParamNode g_NullNode(false);

CParamNode::CParamNode(bool isOk) :
	m_IsOk(isOk)
{
}

void CParamNode::LoadXML(CParamNode& ret, const XMBData& xmb, const wchar_t* sourceIdentifier /*= NULL*/)
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

void CParamNode::ApplyLayer(const XMBData& xmb, const XMBElement& element, const wchar_t* sourceIdentifier /*= NULL*/)
{
	ResetScriptVal();

	std::string name = xmb.GetElementString(element.GetNodeName());
	CStr value = element.GetText();

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
				if (attr.Value == "add")
					op = ADD;
				else if (attr.Value == "mul")
					op = MUL;
				else if (attr.Value == "mul_round")
					op = MUL_ROUND;
				else
					LOGWARNING("Invalid op '%ls'", attr.Value);
			}
		}
	}
	{
		XERO_ITER_ATTR(element, attr)
		{
			if (attr.Name == at_datatype && attr.Value == "tokens")
			{
				CParamNode& node = m_Childs[name];

				// Split into tokens
				std::vector<std::string> oldTokens;
				std::vector<std::string> newTokens;
				if (!replacing && !node.m_Value.empty()) // ignore the old tokens if replace="" was given
					boost::algorithm::split(oldTokens, node.m_Value, boost::algorithm::is_space(), boost::algorithm::token_compress_on);
				if (!value.empty())
					boost::algorithm::split(newTokens, value, boost::algorithm::is_space(), boost::algorithm::token_compress_on);

				// Merge the two lists
				std::vector<std::string> tokens = oldTokens;
				for (const std::string& newToken : newTokens)
				{
					if (newToken[0] == '-')
					{
						std::vector<std::string>::iterator tokenIt =
							std::find(tokens.begin(), tokens.end(),
								std::string_view{newToken}.substr(1));
						if (tokenIt != tokens.end())
							tokens.erase(tokenIt);
						else
						{
							const std::string identifier{
								sourceIdentifier ? (" in '" +
									utf8_from_wstring(sourceIdentifier) + "'") : ""};
							LOGWARNING("[ParamNode] Could not remove token "
								"'%s' from node '%s'%s; not present in "
								"list nor inherited (possible typo?)",
								std::string_view{newToken}.substr(1), name,
								identifier);
						}
					}
					else
					{
						if (std::find(oldTokens.begin(), oldTokens.end(), newToken) == oldTokens.end())
							tokens.push_back(newToken);
					}
				}

				node.m_Value = boost::algorithm::join(tokens, " ");
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
		fixed mod = fixed::FromString(value);

		switch (op)
		{
		case ADD:
			node.m_Value = (oldval + mod).ToString();
			break;
		case MUL:
			node.m_Value = oldval.Multiply(mod).ToString();
			break;
		case MUL_ROUND:
			node.m_Value = fixed::FromInt(oldval.Multiply(mod).ToInt_RoundToNearest()).ToString();
			break;
		default:
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
		const char* attrName(xmb.GetAttributeString(attr.Name));
		node.m_Childs[CStr("@") + attrName].m_Value = attr.Value;
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

const std::wstring CParamNode::ToWString() const
{
	return wstring_from_utf8(m_Value);
}

const std::string& CParamNode::ToString() const
{
	return m_Value;
}

const CStrIntern CParamNode::ToUTF8Intern() const
{
	return CStrIntern(m_Value);
}

int CParamNode::ToInt() const
{
	return std::strtol(m_Value.c_str(), nullptr, 10);
}

fixed CParamNode::ToFixed() const
{
	return fixed::FromString(m_Value);
}

float CParamNode::ToFloat() const
{
	return std::strtof(m_Value.c_str(), nullptr);
}

bool CParamNode::ToBool() const
{
	if (m_Value == "true")
		return true;
	else
		return false;
}

const CParamNode::ChildrenMap& CParamNode::GetChildren() const
{
	return m_Childs;
}

std::string CParamNode::EscapeXMLString(const std::string& str)
{
	std::string ret;
	ret.reserve(str.size());
	// TODO: would be nice to check actual v1.0 XML codepoints,
	// but our UTF8 validation routines are lacking.
	for (size_t i = 0; i < str.size(); ++i)
	{
		char c = str[i];
		switch (c)
		{
		case '<': ret += "&lt;"; break;
		case '>': ret += "&gt;"; break;
		case '&': ret += "&amp;"; break;
		case '"': ret += "&quot;"; break;
		case '\t': ret += "&#9;"; break;
		case '\n': ret += "&#10;"; break;
		case '\r': ret += "&#13;"; break;
		default:
			ret += c;
		}
	}
	return ret;
}

std::string CParamNode::ToXMLString() const
{
	std::stringstream strm;
	ToXMLString(strm);
	return strm.str();
}

void CParamNode::ToXMLString(std::ostream& strm) const
{
	strm << m_Value;

	ChildrenMap::const_iterator it = m_Childs.begin();
	for (; it != m_Childs.end(); ++it)
	{
		// Skip attributes here (they were handled when the caller output the tag)
		if (it->first.length() && it->first[0] == '@')
			continue;

		strm << "<" << it->first;

		// Output the child's attributes first
		ChildrenMap::const_iterator cit = it->second.m_Childs.begin();
		for (; cit != it->second.m_Childs.end(); ++cit)
		{
			if (cit->first.length() && cit->first[0] == '@')
			{
				std::string attrname (cit->first.begin()+1, cit->first.end());
				strm << " " << attrname << "=\"" << EscapeXMLString(cit->second.m_Value) << "\"";
			}
		}

		strm << ">";

		it->second.ToXMLString(strm);

		strm << "</" << it->first << ">";
	}
}

void CParamNode::ToJSVal(const ScriptRequest& rq, bool cacheValue, JS::MutableHandleValue ret) const
{
	if (cacheValue && m_ScriptVal != NULL)
	{
		ret.set(*m_ScriptVal);
		return;
	}

	ConstructJSVal(rq, ret);

	if (cacheValue)
		m_ScriptVal.reset(new JS::PersistentRootedValue(rq.cx, ret));
}

void CParamNode::ConstructJSVal(const ScriptRequest& rq, JS::MutableHandleValue ret) const
{
	if (m_Childs.empty())
	{
		// Empty node - map to undefined
		if (m_Value.empty())
		{
			ret.setUndefined();
			return;
		}

		// Just a string
		JS::RootedString str(rq.cx, JS_NewStringCopyUTF8Z(rq.cx, JS::ConstUTF8CharsZ(m_Value.data(), m_Value.size())));
		str.set(JS_AtomizeAndPinJSString(rq.cx, str));
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

	JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));
	if (!obj)
	{
		ret.setUndefined();
		return; // TODO: report error
	}

	JS::RootedValue childVal(rq.cx);
	for (std::map<std::string, CParamNode>::const_iterator it = m_Childs.begin(); it != m_Childs.end(); ++it)
	{
		it->second.ConstructJSVal(rq, &childVal);
		if (!JS_SetProperty(rq.cx, obj, it->first.c_str(), childVal))
		{
			ret.setUndefined();
			return; // TODO: report error
		}
	}

	// If the node has a string too, add that as an extra property
	if (!m_Value.empty())
	{
		std::u16string text(m_Value.begin(), m_Value.end());
		JS::RootedString str(rq.cx, JS_AtomizeAndPinUCStringN(rq.cx, text.c_str(), text.length()));
		if (!str)
		{
			ret.setUndefined();
			return; // TODO: report error
		}

		JS::RootedValue subChildVal(rq.cx, JS::StringValue(str));
		if (!JS_SetProperty(rq.cx, obj, "_string", subChildVal))
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
