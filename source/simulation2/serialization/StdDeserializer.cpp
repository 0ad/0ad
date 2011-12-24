/* Copyright (C) 2011 Wildfire Games.
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

#include "StdDeserializer.h"

#include "SerializedScriptTypes.h"
#include "StdSerializer.h" // for DEBUG_SERIALIZER_ANNOTATE

#include "scriptinterface/ScriptInterface.h"

#include "js/jsapi.h"

#include "lib/byte_order.h"


CStdDeserializer::CStdDeserializer(ScriptInterface& scriptInterface, std::istream& stream) :
	m_ScriptInterface(scriptInterface), m_Stream(stream)
{
}

CStdDeserializer::~CStdDeserializer()
{
	FreeScriptBackrefs();
}

void CStdDeserializer::Get(const char* name, u8* data, size_t len)
{
#if DEBUG_SERIALIZER_ANNOTATE
	std::string strName;
	char c = m_Stream.get();
	ENSURE(c == '<');
	while (1)
	{
		c = m_Stream.get();
		if (c == '>')
			break;
		else
			strName += c;
	}
	ENSURE(strName == name);
#else
	UNUSED2(name);
#endif
	m_Stream.read((char*)data, (std::streamsize)len);
	if (!m_Stream.good()) // hit eof before len, or other errors
		throw PSERROR_Deserialize_ReadFailed();
}

std::istream& CStdDeserializer::GetStream()
{
	return m_Stream;
}

void CStdDeserializer::RequireBytesInStream(size_t numBytes)
{
	// It would be nice to do:
// 	if (numBytes > (size_t)m_Stream.rdbuf()->in_avail())
// 		throw PSERROR_Deserialize_OutOfBounds("RequireBytesInStream");
	// but that doesn't work (at least on MSVC) since in_avail isn't
	// guaranteed to return the actual number of bytes available; see e.g.
	// http://social.msdn.microsoft.com/Forums/en/vclanguage/thread/13009a88-933f-4be7-bf3d-150e425e66a6#70ea562d-8605-4742-8851-1bae431ce6ce
	
	// Instead we'll just verify that it's not an extremely large number:
	if (numBytes > 64*MiB)
		throw PSERROR_Deserialize_OutOfBounds("RequireBytesInStream");
}

void CStdDeserializer::AddScriptBackref(JSObject* obj)
{
	std::pair<std::map<u32, JSObject*>::iterator, bool> it = m_ScriptBackrefs.insert(std::make_pair((u32)m_ScriptBackrefs.size()+1, obj));
	ENSURE(it.second);
	if (!JS_AddObjectRoot(m_ScriptInterface.GetContext(), &it.first->second))
		throw PSERROR_Deserialize_ScriptError("JS_AddRoot failed");
}

JSObject* CStdDeserializer::GetScriptBackref(u32 tag)
{
	std::map<u32, JSObject*>::const_iterator it = m_ScriptBackrefs.find(tag);
	if (it == m_ScriptBackrefs.end())
		return NULL;
	return it->second;
}

void CStdDeserializer::FreeScriptBackrefs()
{
	std::map<u32, JSObject*>::iterator it = m_ScriptBackrefs.begin();
	for (; it != m_ScriptBackrefs.end(); ++it)
	{
		if (!JS_RemoveObjectRoot(m_ScriptInterface.GetContext(), &it->second))
			throw PSERROR_Deserialize_ScriptError("JS_RemoveRoot failed");
	}
	m_ScriptBackrefs.clear();
}

////////////////////////////////////////////////////////////////

jsval CStdDeserializer::ReadScriptVal(const char* UNUSED(name), JSObject* appendParent)
{
	JSContext* cx = m_ScriptInterface.GetContext();

	uint8_t type;
	NumberU8_Unbounded("type", type);
	switch (type)
	{
	case SCRIPT_TYPE_VOID:
		return JSVAL_VOID;

	case SCRIPT_TYPE_NULL:
		return JSVAL_NULL;

	case SCRIPT_TYPE_ARRAY:
	case SCRIPT_TYPE_OBJECT:
	{
		JSObject* obj;
		if (appendParent)
		{
			obj = appendParent;
		}
		else if (type == SCRIPT_TYPE_ARRAY)
		{
			u32 length;
			NumberU32_Unbounded("array length", length);
			obj = JS_NewArrayObject(cx, length, NULL);
		}
		else
		{
			obj = JS_NewObject(cx, NULL, NULL, NULL);
		}

		if (!obj)
			throw PSERROR_Deserialize_ScriptError();
		CScriptValRooted objRoot(cx, OBJECT_TO_JSVAL(obj));

		AddScriptBackref(obj);

		uint32_t numProps;
		NumberU32_Unbounded("num props", numProps);

		for (uint32_t i = 0; i < numProps; ++i)
		{
			utf16string propname;
			ReadStringUTF16("prop name", propname);

			jsval propval = ReadScriptVal("prop value", NULL);
			CScriptValRooted propvalRoot(cx, propval);

			if (!JS_SetUCProperty(cx, obj, (const jschar*)propname.data(), propname.length(), &propval))
				throw PSERROR_Deserialize_ScriptError();
		}

		return OBJECT_TO_JSVAL(obj);
	}
	case SCRIPT_TYPE_STRING:
	{
		JSString* str;
		ScriptString("string", str);
		return STRING_TO_JSVAL(str);
	}
	case SCRIPT_TYPE_INT:
	{
		int32_t value;
		NumberI32("value", value, JSVAL_INT_MIN, JSVAL_INT_MAX);
		return INT_TO_JSVAL(value);
	}
	case SCRIPT_TYPE_DOUBLE:
	{
		double value;
		NumberDouble_Unbounded("value", value);
		jsval rval;
		if (!JS_NewNumberValue(cx, value, &rval))
			throw PSERROR_Deserialize_ScriptError("JS_NewNumberValue failed");
		return rval;
	}
	case SCRIPT_TYPE_BOOLEAN:
	{
		uint8_t value;
		NumberU8("value", value, 0, 1);
		return BOOLEAN_TO_JSVAL(value ? JS_TRUE : JS_FALSE);
	}
	case SCRIPT_TYPE_BACKREF:
	{
		u32 tag;
		NumberU32_Unbounded("tag", tag);
		JSObject* obj = GetScriptBackref(tag);
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Invalid backref tag");
		return OBJECT_TO_JSVAL(obj);
	}
	default:
		throw PSERROR_Deserialize_OutOfBounds();
	}
}

void CStdDeserializer::ReadStringUTF16(const char* name, utf16string& str)
{
	uint32_t len;
	NumberU32_Unbounded("string length", len);
	RequireBytesInStream(len*2);
	str.resize(len);
	Get(name, (u8*)str.data(), len*2);
}

void CStdDeserializer::ScriptString(const char* name, JSString*& out)
{
	utf16string str;
	ReadStringUTF16(name, str);

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: probably need to convert JS strings from little-endian
#endif

	out = JS_NewUCStringCopyN(m_ScriptInterface.GetContext(), (const jschar*)str.data(), str.length());
	if (!out)
		throw PSERROR_Deserialize_ScriptError("JS_NewUCStringCopyN failed");
}

void CStdDeserializer::ScriptVal(const char* name, jsval& out)
{
	out = ReadScriptVal(name, NULL);
}

void CStdDeserializer::ScriptVal(const char* name, CScriptVal& out)
{
	out = ReadScriptVal(name, NULL);
}

void CStdDeserializer::ScriptVal(const char* name, CScriptValRooted& out)
{
	out = CScriptValRooted(m_ScriptInterface.GetContext(), ReadScriptVal(name, NULL));
}

void CStdDeserializer::ScriptObjectAppend(const char* name, jsval& obj)
{
	if (!JSVAL_IS_OBJECT(obj))
		throw PSERROR_Deserialize_ScriptError();

	ReadScriptVal(name, JSVAL_TO_OBJECT(obj));
}
