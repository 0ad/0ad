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

#include "StdDeserializer.h"

#include "SerializedScriptTypes.h"

#include "ps/CLogger.h"
#include "ps/CStr.h"

#include "scriptinterface/ScriptInterface.h"
#include "js/jsapi.h"

CStdDeserializer::CStdDeserializer(ScriptInterface& scriptInterface, std::istream& stream) :
	m_ScriptInterface(scriptInterface), m_Stream(stream)
{
}

CStdDeserializer::~CStdDeserializer()
{
	FreeScriptBackrefs();
}

void CStdDeserializer::Get(u8* data, size_t len)
{
	m_Stream.read((char*)data, len);
	if (!m_Stream.good()) // hit eof before len, or other errors
		throw PSERROR_Deserialize_ReadFailed();
}

void CStdDeserializer::AddScriptBackref(JSObject* obj)
{
	std::pair<std::map<u32, JSObject*>::iterator, bool> it = m_ScriptBackrefs.insert(std::make_pair((u32)m_ScriptBackrefs.size()+1, obj));
	debug_assert(it.second);
	if (!JS_AddRoot(m_ScriptInterface.GetContext(), (void*)&it.first->second))
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
		if (!JS_RemoveRoot(m_ScriptInterface.GetContext(), (void*)&it->second))
			throw PSERROR_Deserialize_ScriptError("JS_RemoveRoot failed");
	}
	m_ScriptBackrefs.clear();
}

////////////////////////////////////////////////////////////////

jsval CStdDeserializer::ReadScriptVal(JSObject* appendParent)
{
	JSContext* cx = m_ScriptInterface.GetContext();

	uint8_t type;
	NumberU8_Unbounded(type);
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
			obj = appendParent;
		else if (type == SCRIPT_TYPE_ARRAY)
			obj = JS_NewArrayObject(cx, 0, NULL);
		else
			obj = JS_NewObject(cx, NULL, NULL, NULL);

		if (!obj)
			throw PSERROR_Deserialize_ScriptError();
		CScriptValRooted objRoot(cx, OBJECT_TO_JSVAL(obj));

		AddScriptBackref(obj);

		uint32_t numProps;
		NumberU32_Unbounded(numProps);

		for (uint32_t i = 0; i < numProps; ++i)
		{
			utf16string propname;
			StringUTF16(propname);

			jsval propval = ReadScriptVal(NULL);
			CScriptValRooted propvalRoot(cx, propval);

			if (!JS_SetUCProperty(cx, obj, (const jschar*)propname.data(), propname.length(), &propval))
				throw PSERROR_Deserialize_ScriptError();
		}

		return OBJECT_TO_JSVAL(obj);
	}
	case SCRIPT_TYPE_STRING:
	{
		JSString* str;
		ScriptString(str);
		return STRING_TO_JSVAL(str);
	}
	case SCRIPT_TYPE_INT:
	{
		int32_t value;
		NumberI32(value, JSVAL_INT_MIN, JSVAL_INT_MAX);
		return INT_TO_JSVAL(value);
	}
	case SCRIPT_TYPE_DOUBLE:
	{
		double value;
		NumberDouble_Unbounded(value);
		jsval rval;
		if (!JS_NewNumberValue(cx, value, &rval))
			throw PSERROR_Deserialize_ScriptError("JS_NewNumberValue failed");
		return rval;
	}
	case SCRIPT_TYPE_BOOLEAN:
	{
		uint8_t value;
		NumberU8(value, 0, 1);
		return BOOLEAN_TO_JSVAL(value ? JS_TRUE : JS_FALSE);
	}
	case SCRIPT_TYPE_BACKREF:
	{
		u32 tag;
		NumberU32_Unbounded(tag);
		JSObject* obj = GetScriptBackref(tag);
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Invalid backref tag");
		return OBJECT_TO_JSVAL(obj);
	}
	default:
		throw PSERROR_Deserialize_OutOfBounds();
	}
}

void CStdDeserializer::ScriptString(JSString*& out)
{
	utf16string str;
	StringUTF16(str);

	out = JS_NewUCStringCopyN(m_ScriptInterface.GetContext(), (const jschar*)str.data(), str.length());
	if (!out)
	{
		LOGERROR(L"JS_NewUCStringCopyN failed");
		throw PSERROR_Deserialize_ScriptError();
	}
}

void CStdDeserializer::ScriptVal(jsval& out)
{
	out = ReadScriptVal(NULL);
}

void CStdDeserializer::ScriptVal(CScriptVal& out)
{
	out = ReadScriptVal(NULL);
}

void CStdDeserializer::ScriptObjectAppend(jsval& obj)
{
	if (!JSVAL_IS_OBJECT(obj))
		throw PSERROR_Deserialize_ScriptError();

	ReadScriptVal(JSVAL_TO_OBJECT(obj));
}
