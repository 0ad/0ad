/* Copyright (C) 2016 Wildfire Games.
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
	m_ScriptInterface(scriptInterface), m_Stream(stream), 
	m_dummyObject(scriptInterface.GetJSRuntime())
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	JS_AddExtraGCRootsTracer(m_ScriptInterface.GetJSRuntime(), CStdDeserializer::Trace, this);

	// Add a dummy tag because the serializer uses the tag 0 to indicate that a value
	// needs to be serialized and then tagged
	m_dummyObject = JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr());
	m_ScriptBackrefs.push_back(JS::Heap<JSObject*>(m_dummyObject));
}

CStdDeserializer::~CStdDeserializer()
{
	FreeScriptBackrefs();
	JS_RemoveExtraGCRootsTracer(m_ScriptInterface.GetJSRuntime(), CStdDeserializer::Trace, this);
}

void CStdDeserializer::Trace(JSTracer *trc, void *data)
{
	reinterpret_cast<CStdDeserializer*>(data)->TraceMember(trc);
}

void CStdDeserializer::TraceMember(JSTracer *trc)
{
	for (size_t i=0; i<m_ScriptBackrefs.size(); ++i)
		JS_CallHeapObjectTracer(trc, &m_ScriptBackrefs[i], "StdDeserializer::m_ScriptBackrefs");

	for (std::pair<const std::wstring, JS::Heap<JSObject*>>& proto : m_SerializablePrototypes)
		JS_CallHeapObjectTracer(trc, &proto.second, "StdDeserializer::m_SerializablePrototypes");
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

void CStdDeserializer::GetScriptBackref(u32 tag, JS::MutableHandleObject ret)
{
	ENSURE(m_ScriptBackrefs.size() > tag);
	ret.set(m_ScriptBackrefs[tag]);
}

u32 CStdDeserializer::ReserveScriptBackref()
{
	m_ScriptBackrefs.push_back(JS::Heap<JSObject*>(m_dummyObject));
	return m_ScriptBackrefs.size()-1;
}

void CStdDeserializer::SetReservedScriptBackref(u32 tag, JS::HandleObject obj)
{
	ENSURE(m_ScriptBackrefs[tag] == m_dummyObject);
	m_ScriptBackrefs[tag] = JS::Heap<JSObject*>(obj);
}

void CStdDeserializer::FreeScriptBackrefs()
{
	m_ScriptBackrefs.clear();
}

////////////////////////////////////////////////////////////////

jsval CStdDeserializer::ReadScriptVal(const char* UNUSED(name), JS::HandleObject appendParent)
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
	case SCRIPT_TYPE_OBJECT_PROTOTYPE:
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
		else if (type == SCRIPT_TYPE_OBJECT)
		{
			obj.set(JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
		}
		else // SCRIPT_TYPE_OBJECT_PROTOTYPE
		{
			std::wstring prototypeName;
			String("proto name", prototypeName, 0, 256);

			// Get constructor object
			JS::RootedObject proto(cx);
			GetSerializablePrototype(prototypeName, &proto);
			if (!proto)
				throw PSERROR_Deserialize_ScriptError("Failed to find serializable prototype for object");

			JS::RootedObject parent(cx, JS_GetParent(proto));
			if (!proto || !parent)
				throw PSERROR_Deserialize_ScriptError();

			obj.set(JS_NewObject(cx, nullptr, proto, parent));
			if (!obj)
				throw PSERROR_Deserialize_ScriptError("JS_NewObject failed");

			// Does it have custom Deserialize function?
			// if so, we let it handle the deserialized data, rather than adding properties directly
			bool hasCustomDeserialize, hasCustomSerialize;
			if (!JS_HasProperty(cx, obj, "Serialize", &hasCustomSerialize) || !JS_HasProperty(cx, obj, "Deserialize", &hasCustomDeserialize))
				throw PSERROR_Serialize_ScriptError("JS_HasProperty failed");

			if (hasCustomDeserialize)
			{
				JS::RootedValue serialize(cx);
				if (!JS_GetProperty(cx, obj, "Serialize", &serialize))
					throw PSERROR_Serialize_ScriptError("JS_GetProperty failed");
				bool hasNullSerialize = hasCustomSerialize && serialize.isNull();

				// If Serialize is null, we'll still call Deserialize but with undefined argument
				JS::RootedValue data(cx);
				if (!hasNullSerialize)
					ScriptVal("data", &data);

				JS::RootedValue objVal(cx, JS::ObjectValue(*obj));
				m_ScriptInterface.CallFunctionVoid(objVal, "Deserialize", data);
				
				AddScriptBackref(obj);
				
				return JS::ObjectValue(*obj);
			}
		}

		if (!obj)
			throw PSERROR_Deserialize_ScriptError("Deserializer failed to create new object");

		AddScriptBackref(obj);

		uint32_t numProps;
		NumberU32_Unbounded("num props", numProps);

		for (uint32_t i = 0; i < numProps; ++i)
		{
			utf16string propname;
			ReadStringUTF16("prop name", propname);
			JS::RootedValue propval(cx, ReadScriptVal("prop value", JS::NullPtr()));

			if (!JS_SetUCProperty(cx, obj, (const char16_t*)propname.data(), propname.length(), propval))
				throw PSERROR_Deserialize_ScriptError();
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
		u32 tag;
		NumberU32_Unbounded("tag", tag);
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
		u32 arrayTag = ReserveScriptBackref();

		// Get buffer object
		JS::RootedValue bufferVal(cx, ReadScriptVal("buffer", JS::NullPtr()));

		if (!bufferVal.isObject())
			throw PSERROR_Deserialize_ScriptError();

		JS::RootedObject bufferObj(cx, &bufferVal.toObject());
		if (!JS_IsArrayBufferObject(bufferObj))
			throw PSERROR_Deserialize_ScriptError("js_IsArrayBuffer failed");

		JS::RootedObject arrayObj(cx);
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
		u32 mapSize;
		NumberU32_Unbounded("map size", mapSize);
		JS::RootedValue mapVal(cx);
		m_ScriptInterface.Eval("(new Map())", &mapVal);

		// To match the serializer order, we reserve the map's backref tag here
		u32 mapTag = ReserveScriptBackref();
		
		for (u32 i=0; i<mapSize; ++i)
		{
			JS::RootedValue key(cx, ReadScriptVal("map key", JS::NullPtr()));
			JS::RootedValue value(cx, ReadScriptVal("map value", JS::NullPtr()));
			m_ScriptInterface.CallFunctionVoid(mapVal, "set", key, value);
		}
		JS::RootedObject mapObj(cx, &mapVal.toObject());
		SetReservedScriptBackref(mapTag, mapObj);
		return mapVal;
	}
	case SCRIPT_TYPE_OBJECT_SET:
	{
		u32 setSize;
		NumberU32_Unbounded("set size", setSize);
		JS::RootedValue setVal(cx);
		m_ScriptInterface.Eval("(new Set())", &setVal);

		// To match the serializer order, we reserve the set's backref tag here
		u32 setTag = ReserveScriptBackref();

		for (u32 i=0; i<setSize; ++i)
		{
			JS::RootedValue value(cx, ReadScriptVal("set value", JS::NullPtr()));
			m_ScriptInterface.CallFunctionVoid(setVal, "add", value);
		}
		JS::RootedObject setObj(cx, &setVal.toObject());
		SetReservedScriptBackref(setTag, setObj);
		return setVal;
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

void CStdDeserializer::ScriptString(const char* name, JS::MutableHandleString out)
{
	utf16string str;
	ReadStringUTF16(name, str);

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: probably need to convert JS strings from little-endian
#endif

	out.set(JS_NewUCStringCopyN(m_ScriptInterface.GetContext(), (const char16_t*)str.data(), str.length()));
	if (!out)
		throw PSERROR_Deserialize_ScriptError("JS_NewUCStringCopyN failed");
}

void CStdDeserializer::ScriptVal(const char* name, JS::MutableHandleValue out)
{
	out.set(ReadScriptVal(name, JS::NullPtr()));
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

void CStdDeserializer::SetSerializablePrototypes(std::map<std::wstring, JS::Heap<JSObject*> >& prototypes)
{
	m_SerializablePrototypes = prototypes;
}

bool CStdDeserializer::IsSerializablePrototype(const std::wstring& name)
{
	return m_SerializablePrototypes.find(name) != m_SerializablePrototypes.end();
}

void CStdDeserializer::GetSerializablePrototype(const std::wstring& name, JS::MutableHandleObject ret)
{
	std::map<std::wstring, JS::Heap<JSObject*> >::iterator it = m_SerializablePrototypes.find(name);
	if (it != m_SerializablePrototypes.end())
		ret.set(it->second);
	else
		ret.set(NULL);
}
