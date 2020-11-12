/* Copyright (C) 2020 Wildfire Games.
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

CStdDeserializer::CStdDeserializer(const ScriptInterface& scriptInterface, std::istream& stream) :
	m_ScriptInterface(scriptInterface), m_Stream(stream)
{
	JS_AddExtraGCRootsTracer(m_ScriptInterface.GetJSRuntime(), CStdDeserializer::Trace, this);
}

CStdDeserializer::~CStdDeserializer()
{
	JS_RemoveExtraGCRootsTracer(m_ScriptInterface.GetJSRuntime(), CStdDeserializer::Trace, this);
}

void CStdDeserializer::Trace(JSTracer *trc, void *data)
{
	reinterpret_cast<CStdDeserializer*>(data)->TraceMember(trc);
}

void CStdDeserializer::TraceMember(JSTracer *trc)
{
	for (size_t i=0; i<m_ScriptBackrefs.size(); ++i)
		JS_CallObjectTracer(trc, &m_ScriptBackrefs[i], "StdDeserializer::m_ScriptBackrefs");
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
	if (!m_Stream.good())
	{
		// hit eof before len, or other errors
		// NOTE: older libc++ versions incorrectly set eofbit on the last char; test gcount as a workaround
		// see https://llvm.org/bugs/show_bug.cgi?id=9335
		if (m_Stream.bad() || m_Stream.fail() || (m_Stream.eof() && m_Stream.gcount() != (std::streamsize)len))
			throw PSERROR_Deserialize_ReadFailed();
	}
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

void CStdDeserializer::AddScriptBackref(JS::HandleObject obj)
{
	m_ScriptBackrefs.push_back(JS::Heap<JSObject*>(obj));
}

void CStdDeserializer::GetScriptBackref(size_t tag, JS::MutableHandleObject ret)
{
	ENSURE(m_ScriptBackrefs.size() > tag);
	ret.set(m_ScriptBackrefs[tag]);
}

////////////////////////////////////////////////////////////////

JS::Value CStdDeserializer::ReadScriptVal(const char* UNUSED(name), JS::HandleObject appendParent)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	uint8_t type;
	NumberU8_Unbounded("type", type);
	switch (type)
	{
	case SCRIPT_TYPE_VOID:
		return JS::UndefinedValue();

	case SCRIPT_TYPE_NULL:
		return JS::NullValue();

	case SCRIPT_TYPE_ARRAY:
	case SCRIPT_TYPE_OBJECT:
	{
		JS::RootedObject obj(cx);
		if (appendParent)
		{
			obj.set(appendParent);
		}
		else if (type == SCRIPT_TYPE_ARRAY)
		{
			u32 length;
			NumberU32_Unbounded("array length", length);
			obj.set(JS_NewArrayObject(cx, length));
		}
		else // SCRIPT_TYPE_OBJECT
		{
			obj.set(JS_NewPlainObject(cx));
		}

		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Deserializer failed to create new object");

		AddScriptBackref(obj);

		uint32_t numProps;
		NumberU32_Unbounded("num props", numProps);
		bool isLatin1;
		for (uint32_t i = 0; i < numProps; ++i)
		{
			Bool("isLatin1", isLatin1);
			if (isLatin1)
			{
				std::vector<JS::Latin1Char> propname;
				ReadStringLatin1("prop name", propname);
				JS::RootedValue propval(cx, ReadScriptVal("prop value", nullptr));

				utf16string prp(propname.begin(), propname.end());;
// TODO: Should ask upstream about getting a variant of JS_SetProperty with a length param.
				if (!JS_SetUCProperty(cx, obj, (const char16_t*)prp.data(), prp.length(), propval))
					throw PSERROR_Deserialize_ScriptError();
			}
			else
			{
				utf16string propname;
				ReadStringUTF16("prop name", propname);
				JS::RootedValue propval(cx, ReadScriptVal("prop value", nullptr));

				if (!JS_SetUCProperty(cx, obj, (const char16_t*)propname.data(), propname.length(), propval))
					throw PSERROR_Deserialize_ScriptError();
			}
		}

		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_STRING:
	{
		JS::RootedString str(cx);
		ScriptString("string", &str);
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
		JS::RootedValue rval(cx, JS::NumberValue(value));
		if (rval.isNull())
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
		i32 tag;
		NumberI32("tag", tag, 0, JSVAL_INT_MAX);
		JS::RootedObject obj(cx);
		GetScriptBackref(tag, &obj);
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Invalid backref tag");
		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_OBJECT_NUMBER:
	{
		double value;
		NumberDouble_Unbounded("value", value);
		JS::RootedValue val(cx, JS::NumberValue(value));

		JS::RootedObject ctorobj(cx);
		if (!JS_GetClassObject(cx, JSProto_Number, &ctorobj))
			throw PSERROR_Deserialize_ScriptError("JS_GetClassObject failed");

		JS::RootedObject obj(cx, JS_New(cx, ctorobj, JS::HandleValueArray(val)));
		if (!obj)
			throw PSERROR_Deserialize_ScriptError("JS_New failed");
		AddScriptBackref(obj);
		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_OBJECT_STRING:
	{
		JS::RootedString str(cx);
		ScriptString("value", &str);
		if (!str)
			throw PSERROR_Deserialize_ScriptError();
		JS::RootedValue val(cx, JS::StringValue(str));

		JS::RootedObject ctorobj(cx);
		if (!JS_GetClassObject(cx, JSProto_String, &ctorobj))
			throw PSERROR_Deserialize_ScriptError("JS_GetClassObject failed");

		JS::RootedObject obj(cx, JS_New(cx, ctorobj, JS::HandleValueArray(val)));
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

		JS::RootedObject ctorobj(cx);
		if (!JS_GetClassObject(cx, JSProto_Boolean, &ctorobj))
			throw PSERROR_Deserialize_ScriptError("JS_GetClassObject failed");

		JS::RootedObject obj(cx, JS_New(cx, ctorobj, JS::HandleValueArray(val)));
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
		JS::RootedObject arrayObj(cx);
		AddScriptBackref(arrayObj);

		// Get buffer object
		JS::RootedValue bufferVal(cx, ReadScriptVal("buffer", nullptr));

		if (!bufferVal.isObject())
			throw PSERROR_Deserialize_ScriptError();

		JS::RootedObject bufferObj(cx, &bufferVal.toObject());
		if (!JS_IsArrayBufferObject(bufferObj))
			throw PSERROR_Deserialize_ScriptError("js_IsArrayBuffer failed");

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

		return JS::ObjectValue(*arrayObj);
	}
	case SCRIPT_TYPE_ARRAY_BUFFER:
	{
		u32 length;
		NumberU32_Unbounded("buffer length", length);

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: need to convert JS ArrayBuffer data from little-endian
#endif
		void* contents = malloc(length);
		ENSURE(contents);
		RawBytes("buffer data", (u8*)contents, length);
		JS::RootedObject bufferObj(cx, JS_NewArrayBufferWithContents(cx, length, contents));
		AddScriptBackref(bufferObj);

		return JS::ObjectValue(*bufferObj);
	}
	case SCRIPT_TYPE_OBJECT_MAP:
	{
		JS::RootedObject obj(cx, JS::NewMapObject(cx));
		AddScriptBackref(obj);

		u32 mapSize;
		NumberU32_Unbounded("map size", mapSize);

		for (u32 i=0; i<mapSize; ++i)
		{
			JS::RootedValue key(cx, ReadScriptVal("map key", nullptr));
			JS::RootedValue value(cx, ReadScriptVal("map value", nullptr));
			JS::MapSet(cx, obj, key, value);
		}

		return JS::ObjectValue(*obj);
	}
	case SCRIPT_TYPE_OBJECT_SET:
	{
		JS::RootedValue setVal(cx);
		m_ScriptInterface.Eval("(new Set())", &setVal);

		JS::RootedObject setObj(cx, &setVal.toObject());
		AddScriptBackref(setObj);

		u32 setSize;
		NumberU32_Unbounded("set size", setSize);

		for (u32 i=0; i<setSize; ++i)
		{
			JS::RootedValue value(cx, ReadScriptVal("set value", nullptr));
			m_ScriptInterface.CallFunctionVoid(setVal, "add", value);
		}

		return setVal;
	}
	default:
		throw PSERROR_Deserialize_OutOfBounds();
	}
}

void CStdDeserializer::ReadStringLatin1(const char* name, std::vector<JS::Latin1Char>& str)
{
	uint32_t len;
	NumberU32_Unbounded("string length", len);
	RequireBytesInStream(len);
	str.resize(len);
	Get(name, (u8*)str.data(), len);
}

void CStdDeserializer::ReadStringUTF16(const char* name, utf16string& str)
{
	uint32_t len;
	NumberU32_Unbounded("string length", len);
	RequireBytesInStream(len*2);
	str.resize(len);
	Get(name, (u8*)str.data(), len*2);
}

void CStdDeserializer::ScriptString(const char* name, JS::MutableHandleString out)
{
#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: probably need to convert JS strings from little-endian
#endif

	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	bool isLatin1;
	Bool("isLatin1", isLatin1);
	if (isLatin1)
	{
		std::vector<JS::Latin1Char> str;
		ReadStringLatin1(name, str);

		out.set(JS_NewStringCopyN(cx, (const char*)str.data(), str.size()));
		if (!out)
			throw PSERROR_Deserialize_ScriptError("JS_NewStringCopyN failed");
	}
	else
	{
		utf16string str;
		ReadStringUTF16(name, str);

		out.set(JS_NewUCStringCopyN(cx, (const char16_t*)str.data(), str.length()));
		if (!out)
			throw PSERROR_Deserialize_ScriptError("JS_NewUCStringCopyN failed");
	}
}

void CStdDeserializer::ScriptVal(const char* name, JS::MutableHandleValue out)
{
	out.set(ReadScriptVal(name, nullptr));
}

void CStdDeserializer::ScriptObjectAppend(const char* name, JS::HandleValue objVal)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	if (!objVal.isObject())
		throw PSERROR_Deserialize_ScriptError();

	JS::RootedObject obj(cx, &objVal.toObject());
	ReadScriptVal(name, obj);
}
