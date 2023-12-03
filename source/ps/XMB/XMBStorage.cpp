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

#include "XMBStorage.h"

#include "lib/file/io/write_buffer.h"
#include "lib/file/vfs/vfs.h"
#include "ps/CLogger.h"
#include "scriptinterface/Object.h"
#include "scriptinterface/ScriptConversions.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include "scriptinterface/ScriptInterface.h"

#include <libxml/parser.h>
#include <string_view>
#include <unordered_map>

const char* XMBStorage::HeaderMagicStr = "XMB0";
const char* XMBStorage::UnfinishedHeaderMagicStr = "XMBu";
// Arbitrary version number - change this if we update the code and
// need to invalidate old users' caches
const u32 XMBStorage::XMBVersion = 4;

namespace
{
class XMBStorageWriter
{
public:
	template<typename ...Args>
	bool Load(WriteBuffer& writeBuffer, Args&&... args);

	int GetElementName(const std::string& name) { return GetName(m_ElementSize, m_ElementIDs, name); }
	int GetAttributeName(const std::string& name) { return GetName(m_AttributeSize, m_AttributeIDs, name); }

protected:
	int GetName(int& totalSize, std::unordered_map<std::string, int>& names, const std::string& name)
	{
		int nameIdx = totalSize;
		auto [iterator, inserted] = names.try_emplace(name, nameIdx);
		if (inserted)
			totalSize += name.size() + 5; // Add 1 for the null terminator & 4 for the size int.
		return iterator->second;
	}

	void OutputNames(WriteBuffer& writeBuffer, const std::unordered_map<std::string, int>& names) const;

	template<typename ...Args>
	bool OutputElements(WriteBuffer&, Args...)
	{
		static_assert(sizeof...(Args) != sizeof...(Args), "OutputElements must be specialized.");
		return false;
	}

	int m_ElementSize = 0;
	int m_AttributeSize = 0;
	std::unordered_map<std::string, int> m_ElementIDs;
	std::unordered_map<std::string, int> m_AttributeIDs;
};

// Output text, prefixed by length in bytes (including null-terminator)
void WriteStringAndLineNumber(WriteBuffer& writeBuffer, const std::string& text, int lineNumber)
{
	if (text.empty())
	{
		// No text; don't write much
		writeBuffer.Append("\0\0\0\0", 4);
	}
	else
	{
		// Write length and line number and null-terminated text
		u32 nodeLen = u32(4 + text.length() + 1);
		writeBuffer.Append(&nodeLen, 4);
		writeBuffer.Append(&lineNumber, 4);
		writeBuffer.Append((void*)text.c_str(), nodeLen-4);
	}
}

template<typename ...Args>
bool XMBStorageWriter::Load(WriteBuffer& writeBuffer, Args&&... args)
{
	// Header
	writeBuffer.Append(XMBStorage::UnfinishedHeaderMagicStr, 4);
	// Version
	writeBuffer.Append(&XMBStorage::XMBVersion, 4);

	// Filled in below.
	size_t elementPtr = writeBuffer.Size();
	writeBuffer.Append("????????", 8);
	// Likewise with attributes.
	size_t attributePtr = writeBuffer.Size();
	writeBuffer.Append("????????", 8);

	if (!OutputElements<Args&&...>(writeBuffer, std::forward<Args>(args)...))
		return false;

	u32 data = writeBuffer.Size();
	writeBuffer.Overwrite(&data, 4, elementPtr);
	data = m_ElementIDs.size();
	writeBuffer.Overwrite(&data, 4, elementPtr + 4);
	OutputNames(writeBuffer, m_ElementIDs);

	data = writeBuffer.Size();
	writeBuffer.Overwrite(&data, 4, attributePtr);
	data = m_AttributeIDs.size();
	writeBuffer.Overwrite(&data, 4, attributePtr + 4);
	OutputNames(writeBuffer, m_AttributeIDs);

	// File is now valid, so insert correct magic string.
	writeBuffer.Overwrite(XMBStorage::HeaderMagicStr, 4, 0);

	return true;
}

void XMBStorageWriter::OutputNames(WriteBuffer& writeBuffer, const std::unordered_map<std::string, int>& names) const
{
	std::vector<std::pair<std::string, int>> orderedElements;
	for (const std::pair<const std::string, int>& n : names)
		orderedElements.emplace_back(n);
	std::sort(orderedElements.begin(), orderedElements.end(), [](const auto& a, const auto&b) { return a.second < b.second; });
	for (const std::pair<std::string, int>& n : orderedElements)
	{
		u32 textLen = (u32)n.first.length() + 1;
		writeBuffer.Append(&textLen, 4);
		writeBuffer.Append((void*)n.first.c_str(), textLen);
	}
}

class JSNodeData
{
public:
	JSNodeData(const ScriptInterface& s) : scriptInterface(s), rq(s) {}

	bool Setup(XMBStorageWriter& xmb, JS::HandleValue value);
	bool Output(WriteBuffer& writeBuffer, JS::HandleValue value) const;

	std::vector<std::pair<u32, std::string>> m_Attributes;
	std::vector<std::pair<u32, JS::Heap<JS::Value>>> m_Children;

	const ScriptInterface& scriptInterface;
	const ScriptRequest rq;
};

template<>
bool XMBStorageWriter::OutputElements<JSNodeData&, const u32&, JS::HandleValue&&>(WriteBuffer& writeBuffer, JSNodeData& data, const u32& nodeName, JS::HandleValue&& value)
{
	// Set up variables.
	if (!data.Setup(*this, value))
		return false;

	size_t posLength = writeBuffer.Size();
	// Filled in later with the length of the element
	writeBuffer.Append("????", 4);

	writeBuffer.Append(&nodeName, 4);

	u32 attrCount = static_cast<u32>(data.m_Attributes.size());
	writeBuffer.Append(&attrCount, 4);

	u32 childCount = data.m_Children.size();
	writeBuffer.Append(&childCount, 4);

	// Filled in later with the offset to the list of child elements
	size_t posChildrenOffset = writeBuffer.Size();
	writeBuffer.Append("????", 4);

	data.Output(writeBuffer, value);

	// Output attributes
	for (const std::pair<const u32, std::string> attr : data.m_Attributes)
	{
		writeBuffer.Append(&attr.first, 4);
		u32 attrLen = u32(attr.second.size())+1;
		writeBuffer.Append(&attrLen, 4);
		writeBuffer.Append((void*)attr.second.c_str(), attrLen);
	}

	// Go back and fill in the child-element offset
	u32 childrenOffset = (u32)(writeBuffer.Size() - (posChildrenOffset+4));
	writeBuffer.Overwrite(&childrenOffset, 4, posChildrenOffset);

	// Output all child elements, making a copy since data will be overwritten.
	std::vector<std::pair<u32, JS::Heap<JS::Value>>> children = data.m_Children;
	for (const std::pair<u32, JS::Heap<JS::Value>>& child : children)
	{
		JS::RootedValue val(data.rq.cx, child.second);
		if (!OutputElements<JSNodeData&, const u32&, JS::HandleValue&&>(writeBuffer, data, child.first, val))
			return false;
	}

	// Go back and fill in the length
	u32 length = (u32)(writeBuffer.Size() - posLength);
	writeBuffer.Overwrite(&length, 4, posLength);

	return true;
}

bool JSNodeData::Setup(XMBStorageWriter& xmb, JS::HandleValue value)
{
	m_Attributes.clear();
	m_Children.clear();
	JSType valType = JS_TypeOfValue(rq.cx, value);
	if (valType != JSTYPE_OBJECT)
		return true;

	std::vector<std::string> props;
	if (!Script::EnumeratePropertyNames(rq, value, true, props))
	{
		LOGERROR("Failed to enumerate component properties.");
		return false;
	}

	for (const std::string& prop : props)
	{
		// Special 'value' key.
		if (prop == "_string")
			continue;

		bool attrib = !prop.empty() && prop.front() == '@';

		std::string_view name = prop;
		if (!attrib && !prop.empty() && prop.back() == '@')
		{
			const size_t idx = std::string_view{prop}.substr(0, prop.size() - 1)
				.find_last_of('@');
			if (idx == std::string::npos)
			{
				LOGERROR("Object key name cannot end with an '@' unless it is an index specifier.");
				return false;
			}
			name = std::string_view(prop.c_str(), idx);
		}
		else if (attrib)
			name = std::string_view(prop.c_str()+1, prop.length()-1);

		JS::RootedValue child(rq.cx);
		if (!Script::GetProperty(rq, value, prop.c_str(), &child))
			return false;

		if (attrib)
		{
			std::string attrVal;
			if (!Script::FromJSVal(rq, child, attrVal))
			{
				LOGERROR("Attributes must be convertible to string");
				return false;
			}
			m_Attributes.emplace_back(xmb.GetAttributeName(std::string(name)), attrVal);
			continue;
		}

		bool isArray = false;
		if (!JS::IsArrayObject(rq.cx, child, &isArray))
			return false;
		if (!isArray)
		{
			m_Children.emplace_back(xmb.GetElementName(std::string(name)), child);
			continue;
		}

		// Parse each array object as a child.
		JS::RootedObject obj(rq.cx);
		JS_ValueToObject(rq.cx, child, &obj);
		u32 length;
		JS::GetArrayLength(rq.cx, obj, &length);
		for (size_t i = 0; i < length; ++i)
		{
			JS::RootedValue arrayChild(rq.cx);
			Script::GetPropertyInt(rq, child, i, &arrayChild);
			m_Children.emplace_back(xmb.GetElementName(std::string(name)), arrayChild);
		}
	}
	return true;
}

bool JSNodeData::Output(WriteBuffer& writeBuffer, JS::HandleValue value) const
{
	switch (JS_TypeOfValue(rq.cx, value))
	{
		case JSTYPE_UNDEFINED:
		case JSTYPE_NULL:
		{
			writeBuffer.Append("\0\0\0\0", 4);
			break;
		}
		case JSTYPE_OBJECT:
		{
			if (!Script::HasProperty(rq, value, "_string"))
			{
				writeBuffer.Append("\0\0\0\0", 4);
				break;
			}
			JS::RootedValue actualValue(rq.cx);
			if (!Script::GetProperty(rq, value, "_string", &actualValue))
				return false;
			std::string strVal;
			if (!Script::FromJSVal(rq, actualValue, strVal))
			{
				LOGERROR("'_string' value must be convertible to string");
				return false;
			}
			WriteStringAndLineNumber(writeBuffer, strVal, 0);
			break;
		}
		case JSTYPE_STRING:
		case JSTYPE_NUMBER:
		{
			std::string strVal;
			if (!Script::FromJSVal(rq, value, strVal))
				return false;

			WriteStringAndLineNumber(writeBuffer, strVal, 0);
			break;
		}
		default:
		{
			LOGERROR("Unsupported JS construct when parsing ParamNode");
			return false;
		}
	}
	return true;
}

template<>
bool XMBStorageWriter::OutputElements<xmlNodePtr&&>(WriteBuffer& writeBuffer, xmlNodePtr&& node)
{
	// Filled in later with the length of the element
	size_t posLength = writeBuffer.Size();
	writeBuffer.Append("????", 4);

	u32 name = GetElementName((const char*)node->name);
	writeBuffer.Append(&name, 4);

	u32 attrCount = 0;
	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
		++attrCount;
	writeBuffer.Append(&attrCount, 4);

	u32 childCount = 0;
	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			++childCount;
	writeBuffer.Append(&childCount, 4);

	// Filled in later with the offset to the list of child elements
	size_t posChildrenOffset = writeBuffer.Size();
	writeBuffer.Append("????", 4);


	// Trim excess whitespace in the entity's text, while counting
	// the number of newlines trimmed (so that JS error reporting
	// can give the correct line number within the script)

	std::string whitespace = " \t\r\n";
	std::string text;
	for (xmlNodePtr child = node->children; child; child = child->next)
	{
		if (child->type == XML_TEXT_NODE)
		{
			xmlChar* content = xmlNodeGetContent(child);
			text += std::string((const char*)content);
			xmlFree(content);
		}
	}

	u32 linenum = xmlGetLineNo(node);

	// Find the start of the non-whitespace section
	size_t first = text.find_first_not_of(whitespace);

	if (first == text.npos)
		// Entirely whitespace - easy to handle
		text = "";

	else
	{
		// Count the number of \n being cut off,
		// and add them to the line number
		std::string trimmed (text.begin(), text.begin()+first);
		linenum += std::count(trimmed.begin(), trimmed.end(), '\n');

		// Find the end of the non-whitespace section,
		// and trim off everything else
		size_t last = text.find_last_not_of(whitespace);
		text = text.substr(first, 1+last-first);
	}


	// Output text, prefixed by length in bytes
	WriteStringAndLineNumber(writeBuffer, text, linenum);

	// Output attributes
	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
	{
		u32 attrName = GetAttributeName((const char*)attr->name);
		writeBuffer.Append(&attrName, 4);

		xmlChar* value = xmlNodeGetContent(attr->children);
		u32 attrLen = u32(xmlStrlen(value)+1);
		writeBuffer.Append(&attrLen, 4);
		writeBuffer.Append((void*)value, attrLen);
		xmlFree(value);
	}

	// Go back and fill in the child-element offset
	u32 childrenOffset = (u32)(writeBuffer.Size() - (posChildrenOffset+4));
	writeBuffer.Overwrite(&childrenOffset, 4, posChildrenOffset);

	// Output all child elements
	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			OutputElements<xmlNodePtr&&>(writeBuffer, std::move(child));

	// Go back and fill in the length
	u32 length = (u32)(writeBuffer.Size() - posLength);
	writeBuffer.Overwrite(&length, 4, posLength);

	return true;
}
} // anonymous namespace

bool XMBStorage::ReadFromFile(const PIVFS& vfs, const VfsPath& filename)
{
	if(vfs->LoadFile(filename, m_Buffer, m_Size) < 0)
		return false;
	// if the game crashes during loading, (e.g. due to driver bugs),
	// it sometimes leaves empty XMB files in the cache.
	// reporting failure will cause our caller to re-generate the XMB.
	if (m_Size == 0)
		return false;
	ENSURE(m_Size >= 4); // make sure it's at least got the initial header
	return true;
}

bool XMBStorage::LoadXMLDoc(const xmlDocPtr doc)
{
	WriteBuffer writeBuffer;

	XMBStorageWriter writer;
	if (!writer.Load(writeBuffer, std::move(xmlDocGetRootElement(doc))))
		return false;

	m_Buffer = writeBuffer.Data(); // add a reference
	m_Size = writeBuffer.Size();
	return true;
}

bool XMBStorage::LoadJSValue(const ScriptInterface& scriptInterface, JS::HandleValue value, const std::string& rootName)
{
	WriteBuffer writeBuffer;

	XMBStorageWriter writer;
	const u32 name = writer.GetElementName(rootName);
	JSNodeData data(scriptInterface);
	if (!writer.Load(writeBuffer, data, name, std::move(value)))
		return false;

	m_Buffer = writeBuffer.Data(); // add a reference
	m_Size = writeBuffer.Size();
	return true;
}
