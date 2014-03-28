/* Copyright (C) 2013 Wildfire Games.
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
#include "scriptinterface/ScriptExtraHeaders.h" // for typed arrays

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

u32 CStdDeserializer::ReserveScriptBackref()
{
	AddScriptBackref(NULL);
	return m_ScriptBackrefs.size();
}

void CStdDeserializer::SetReservedScriptBackref(u32 tag, JSObject* obj)
{
	std::pair<std::map<u32, JSObject*>::iterator, bool> it = m_ScriptBackrefs.insert(std::make_pair(tag, obj));
	ENSURE(!it.second);
}

void CStdDeserializer::FreeScriptBackrefs()
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);
	
	std::map<u32, JSObject*>::iterator it = m_ScriptBackrefs.begin();
	for (; it != m_ScriptBackrefs.end(); ++it)
	{
		JS_RemoveObjectRoot(m_ScriptInterface.GetContext(), &it->second);
	}
	m_ScriptBackrefs.clear();
}

////////////////////////////////////////////////////////////////

jsval CStdDeserializer::ReadScriptVal(const char* UNUSED(name), JSObject* appendParent)
{
	JSContext* cx = m_ScriptInterface.GetContext();

	JSAutoRequest rq(cx);

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
	case SCRIPT_TYPE_OBJECT_PROTOTYPE:
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
		else if (type == SCRIPT_TYPE_OBJECT)
		{
			obj = JS_NewObject(cx, NULL, NULL, NULL);
		}
		else // SCRIPT_TYPE_OBJECT_PROTOTYPE
		{
			std::wstring prototypeName;
			String("proto name", prototypeName, 0, 256);

			// Get constructor object
			JSObject* proto = GetSerializablePrototype(prototypeName);
			if (!proto)
				throw PSERROR_Deserialize_ScriptError("Failed to find serializable prototype for object");

			JSObject* parent = JS_GetParent(proto);
			if (!proto || !parent)
				throw PSERROR_Deserialize_ScriptError();

			obj = JS_NewObject(cx, NULL, proto, parent);
			if (!obj)
				throw PSERROR_Deserialize_ScriptError("JS_NewObject failed");
			CScriptValRooted objRoot(cx, JS::ObjectValue(*obj));

			// Does it have custom Deserialize function?
			// if so, we let it handle the deserialized data, rather than adding properties directly
			JSBool hasCustomDeserialize, hasCustomSerialize;
			if (!JS_HasProperty(cx, obj, "Serialize", &hasCustomSerialize) || !JS_HasProperty(cx, obj, "Deserialize", &hasCustomDeserialize))
				throw PSERROR_Serialize_ScriptError("JS_HasProperty failed");

			if (hasCustomDeserialize)
			{
				JS::RootedValue serialize(cx);
				if (!JS_LookupProperty(cx, obj, "Serialize", serialize.address()))
					throw PSERROR_Serialize_ScriptError("JS_LookupProperty failed");
				bool hasNullSerialize = hasCustomSerialize && JSVAL_IS_NULL(serialize);

				// If Serialize is null, we'll still call Deserialize but with undefined argument
				CScriptValRooted data;
				if (!hasNullSerialize)
					ScriptVal("data", data);

				m_ScriptInterface.CallFunctionVoid(JS::ObjectValue(*obj), "Deserialize", data);
				
				AddScriptBackref(obj);
				
				return JS::ObjectValue(*obj);
			}
		}

		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Deserializer failed to create new object");
		CScriptValRooted objRoot(cx, JS::ObjectValue(*obj));

		AddScriptBackref(obj);

		uint32_t numProps;
		NumberU32_Unbounded("num props", numProps);

		for (uint32_t i = 0; i < numProps; ++i)
		{
			utf16string propname;
			ReadStringUTF16("prop name", propname);

			JS::RootedValue propval(cx, ReadScriptVal("prop value", NULL));
			CScriptValRooted propvalRoot(cx, propval);

			if (!JS_SetUCProperty(cx, obj, (const jschar*)propname.data(), propname.length(), propval.address()))
				throw PSERROR_Deserialize_ScriptError();
		}

		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_STRING:
	{
		JSString* str;
		ScriptString("string", str);
		return JS::StringValue(str);
	}
	case SCRIPT_TYPE_INT:
	{
		int32_t value;
		NumberI32("value", value, JSVAL_INT_MIN, JSVAL_INT_MAX);
		return JS::NumberValue(value);
	}
	case SCRIPT_TYPE_DOUBLE:
	{
		double value;
		NumberDouble_Unbounded("value", value);
		jsval rval = JS::NumberValue(value);
		if (JSVAL_IS_NULL(rval))
			throw PSERROR_Deserialize_ScriptError("JS_NewNumberValue failed");
		return rval;
	}
	case SCRIPT_TYPE_BOOLEAN:
	{
		uint8_t value;
		NumberU8("value", value, 0, 1);
		return JS::BooleanValue(value ? true : false);
	}
	case SCRIPT_TYPE_BACKREF:
	{
		u32 tag;
		NumberU32_Unbounded("tag", tag);
		JSObject* obj = GetScriptBackref(tag);
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Invalid backref tag");
		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_OBJECT_NUMBER:
	{
		double value;
		NumberDouble_Unbounded("value", value);
		JS::RootedValue val(cx, JS::NumberValue(value));
		CScriptValRooted objRoot(cx, val);

		JSObject* ctorobj;
		if (!JS_GetClassObject(cx, JS_GetGlobalForScopeChain(cx), JSProto_Number, &ctorobj))
			throw PSERROR_Deserialize_ScriptError("JS_GetClassObject failed");

		JSObject* obj = JS_New(cx, ctorobj, 1, val.address());
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("JS_New failed");
		AddScriptBackref(obj);
		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_OBJECT_STRING:
	{
		JSString* str;
		ScriptString("value", str);
		if (!str)
			throw PSERROR_Deserialize_ScriptError();
		JS::RootedValue val(cx, JS::StringValue(str));
		CScriptValRooted valRoot(cx, val);

		JSObject* ctorobj;
		if (!JS_GetClassObject(cx, JS_GetGlobalForScopeChain(cx), JSProto_String, &ctorobj))
			throw PSERROR_Deserialize_ScriptError("JS_GetClassObject failed");

		JSObject* obj = JS_New(cx, ctorobj, 1, val.address());
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("JS_New failed");
		AddScriptBackref(obj);
		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_OBJECT_BOOLEAN:
	{
		bool value;
		Bool("value", value);
		JS::RootedValue val(cx, JS::BooleanValue(value));

		JSObject* ctorobj;
		if (!JS_GetClassObject(cx, JS_GetGlobalForScopeChain(cx), JSProto_Boolean, &ctorobj))
			throw PSERROR_Deserialize_ScriptError("JS_GetClassObject failed");

		JSObject* obj = JS_New(cx, ctorobj, 1, val.address());
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("JS_New failed");
		AddScriptBackref(obj);
		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_TYPED_ARRAY:
	{
		u8 arrayType;
		u32 byteOffset, length;
		NumberU8_Unbounded("array type", arrayType);
		NumberU32_Unbounded("byte offset", byteOffset);
		NumberU32_Unbounded("length", length);

		// To match the serializer order, we reserve the typed array's backref tag here
		u32 arrayTag = ReserveScriptBackref();

		// Get buffer object
		jsval bufferVal = ReadScriptVal("buffer", NULL);

		if (!bufferVal.isObject())
			throw PSERROR_Deserialize_ScriptError();

		JSObject* bufferObj = &bufferVal.toObject();
		if (!JS_IsArrayBufferObject(bufferObj))
			throw PSERROR_Deserialize_ScriptError("js_IsArrayBuffer failed");

		JSObject* arrayObj;
		switch(arrayType)
		{
		case SCRIPT_TYPED_ARRAY_INT8:
			arrayObj = JS_NewInt8ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_UINT8:
			arrayObj = JS_NewUint8ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_INT16:
			arrayObj = JS_NewInt16ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_UINT16:
			arrayObj = JS_NewUint16ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_INT32:
			arrayObj = JS_NewInt32ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_UINT32:
			arrayObj = JS_NewUint32ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_FLOAT32:
			arrayObj = JS_NewFloat32ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_FLOAT64:
			arrayObj = JS_NewFloat64ArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		case SCRIPT_TYPED_ARRAY_UINT8_CLAMPED:
			arrayObj = JS_NewUint8ClampedArrayWithBuffer(cx, bufferObj, byteOffset, length);
			break;
		default:
			throw PSERROR_Deserialize_ScriptError("Failed to deserialize unrecognized typed array view");
		}
		if (!arrayObj)
			throw PSERROR_Deserialize_ScriptError("js_CreateTypedArrayWithBuffer failed");

		SetReservedScriptBackref(arrayTag, arrayObj);

		return JS::ObjectValue(*arrayObj);
	}
	case SCRIPT_TYPE_ARRAY_BUFFER:
	{
		u32 length;
		NumberU32_Unbounded("buffer length", length);
		u8* bufferData = NULL;
		

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: need to convert JS ArrayBuffer data from little-endian
#endif
		void* contents = NULL;
		JS_AllocateArrayBufferContents(cx, length, &contents, &bufferData);
		RawBytes("buffer data", bufferData, length);
		JSObject* bufferObj = JS_NewArrayBufferWithContents(cx, contents);
		AddScriptBackref(bufferObj);

		return JS::ObjectValue(*bufferObj);
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
	if (!obj.isObject())
		throw PSERROR_Deserialize_ScriptError();

	ReadScriptVal(name, JSVAL_TO_OBJECT(obj));
}

void CStdDeserializer::SetSerializablePrototypes(std::map<std::wstring, JSObject*>& prototypes)
{
	m_SerializablePrototypes = prototypes;
}

bool CStdDeserializer::IsSerializablePrototype(const std::wstring& name)
{
	return m_SerializablePrototypes.find(name) != m_SerializablePrototypes.end();
}

JSObject* CStdDeserializer::GetSerializablePrototype(const std::wstring& name)
{
	std::map<std::wstring, JSObject*>::iterator it = m_SerializablePrototypes.find(name);
	if (it != m_SerializablePrototypes.end())
		return it->second;
	else
		return NULL;
}
