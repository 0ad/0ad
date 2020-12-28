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

#include "BinarySerializer.h"

#include "lib/alignment.h"
#include "ps/CLogger.h"

#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/ScriptExtraHeaders.h"
#include "SerializedScriptTypes.h"

static u8 GetArrayType(js::Scalar::Type arrayType)
{
	switch(arrayType)
	{
	case js::Scalar::Int8:
		return SCRIPT_TYPED_ARRAY_INT8;
	case js::Scalar::Uint8:
		return SCRIPT_TYPED_ARRAY_UINT8;
	case js::Scalar::Int16:
		return SCRIPT_TYPED_ARRAY_INT16;
	case js::Scalar::Uint16:
		return SCRIPT_TYPED_ARRAY_UINT16;
	case js::Scalar::Int32:
		return SCRIPT_TYPED_ARRAY_INT32;
	case js::Scalar::Uint32:
		return SCRIPT_TYPED_ARRAY_UINT32;
	case js::Scalar::Float32:
		return SCRIPT_TYPED_ARRAY_FLOAT32;
	case js::Scalar::Float64:
		return SCRIPT_TYPED_ARRAY_FLOAT64;
	case js::Scalar::Uint8Clamped:
		return SCRIPT_TYPED_ARRAY_UINT8_CLAMPED;
	default:
		LOGERROR("Cannot serialize unrecognized typed array view: %d", arrayType);
		throw PSERROR_Serialize_InvalidScriptValue();
	}
}

CBinarySerializerScriptImpl::CBinarySerializerScriptImpl(const ScriptInterface& scriptInterface, ISerializer& serializer) :
	m_ScriptInterface(scriptInterface), m_Serializer(serializer), m_ScriptBackrefsNext(0)
{
	ScriptRequest rq(m_ScriptInterface);

	m_ScriptBackrefSymbol.init(rq.cx, JS::NewSymbol(rq.cx, nullptr));
}

void CBinarySerializerScriptImpl::HandleScriptVal(JS::HandleValue val)
{
	ScriptRequest rq(m_ScriptInterface);

	switch (JS_TypeOfValue(rq.cx, val))
	{
	case JSTYPE_UNDEFINED:
	{
		m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_VOID);
		break;
	}
	case JSTYPE_NULL: // This type is never actually returned (it's a JS2 feature)
	{
		m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_NULL);
		break;
	}
	case JSTYPE_OBJECT:
	{
		if (val.isNull())
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_NULL);
			break;
		}

		JS::RootedObject obj(rq.cx, &val.toObject());

		// If we've already serialized this object, just output a reference to it
		i32 tag = GetScriptBackrefTag(obj);
		if (tag != -1)
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_BACKREF);
			m_Serializer.NumberI32("tag", tag, 0, JSVAL_INT_MAX);
			break;
		}

		// Arrays, Maps and Sets are special cases of Objects
		bool isArray;
		bool isMap;
		bool isSet;

		if (JS::IsArrayObject(rq.cx, obj, &isArray) && isArray)
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_ARRAY);
			// TODO: probably should have a more efficient storage format

			// Arrays like [1, 2, ] have an 'undefined' at the end which is part of the
			// length but seemingly isn't enumerated, so store the length explicitly
			uint length = 0;
			if (!JS::GetArrayLength(rq.cx, obj, &length))
				throw PSERROR_Serialize_ScriptError("JS::GetArrayLength failed");
			m_Serializer.NumberU32_Unbounded("array length", length);
		}
		else if (JS_IsTypedArrayObject(obj))
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_TYPED_ARRAY);

			m_Serializer.NumberU8_Unbounded("array type", GetArrayType(JS_GetArrayBufferViewType(obj)));
			m_Serializer.NumberU32_Unbounded("byte offset", JS_GetTypedArrayByteOffset(obj));
			m_Serializer.NumberU32_Unbounded("length", JS_GetTypedArrayLength(obj));

			bool sharedMemory;
			// Now handle its array buffer
			// this may be a backref, since ArrayBuffers can be shared by multiple views
			JS::RootedValue bufferVal(rq.cx, JS::ObjectValue(*JS_GetArrayBufferViewBuffer(rq.cx, obj, &sharedMemory)));
			HandleScriptVal(bufferVal);
			break;
		}
		else if (JS::IsArrayBufferObject(obj))
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_ARRAY_BUFFER);

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: need to convert JS ArrayBuffer data to little-endian
#endif

			u32 length = JS::GetArrayBufferByteLength(obj);
			m_Serializer.NumberU32_Unbounded("buffer length", length);
			JS::AutoCheckCannotGC nogc;
			bool sharedMemory;
			m_Serializer.RawBytes("buffer data", (const u8*)JS::GetArrayBufferData(obj, &sharedMemory, nogc), length);
			break;
		}

		else if (JS::IsMapObject(rq.cx, obj, &isMap) && isMap)
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_MAP);
			m_Serializer.NumberU32_Unbounded("map size", JS::MapSize(rq.cx, obj));

			JS::RootedValue keyValueIterator(rq.cx);
			if (!JS::MapEntries(rq.cx, obj, &keyValueIterator))
				throw PSERROR_Serialize_ScriptError("JS::MapEntries failed");

			JS::ForOfIterator it(rq.cx);
			if (!it.init(keyValueIterator))
				throw PSERROR_Serialize_ScriptError("JS::ForOfIterator::init failed");

			JS::RootedValue keyValuePair(rq.cx);
			bool done;
			while (true)
			{
				if (!it.next(&keyValuePair, &done))
					throw PSERROR_Serialize_ScriptError("JS::ForOfIterator::next failed");

				if (done)
					break;

				JS::RootedObject keyValuePairObj(rq.cx, &keyValuePair.toObject());
				JS::RootedValue key(rq.cx);
				JS::RootedValue value(rq.cx);
				ENSURE(JS_GetElement(rq.cx, keyValuePairObj, 0, &key));
				ENSURE(JS_GetElement(rq.cx, keyValuePairObj, 1, &value));

				HandleScriptVal(key);
				HandleScriptVal(value);
			}
			break;
		}

		else if (JS::IsSetObject(rq.cx, obj, &isSet) && isSet)
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_SET);
			m_Serializer.NumberU32_Unbounded("set size", JS::SetSize(rq.cx, obj));

			JS::RootedValue valueIterator(rq.cx);
			if (!JS::SetValues(rq.cx, obj, &valueIterator))
				throw PSERROR_Serialize_ScriptError("JS::SetValues failed");

			JS::ForOfIterator it(rq.cx);
			if (!it.init(valueIterator))
				throw PSERROR_Serialize_ScriptError("JS::ForOfIterator::init failed");

			JS::RootedValue value(rq.cx);
			bool done;
			while (true)
			{
				if (!it.next(&value, &done))
					throw PSERROR_Serialize_ScriptError("JS::ForOfIterator::next failed");

				if (done)
					break;

				HandleScriptVal(value);
			}
			break;
		}

		else
		{
			// Find type of object
			const JSClass* jsclass = JS_GetClass(obj);
			if (!jsclass)
				throw PSERROR_Serialize_ScriptError("JS_GetClass failed");

			JSProtoKey protokey = JSCLASS_CACHED_PROTO_KEY(jsclass);

			if (protokey == JSProto_Object)
			{
				// Object class - check for user-defined prototype
				JS::RootedObject proto(rq.cx);
				if (!JS_GetPrototype(rq.cx, obj, &proto))
					throw PSERROR_Serialize_ScriptError("JS_GetPrototype failed");

				SPrototypeSerialization protoInfo = GetPrototypeInfo(rq, proto);

				if (protoInfo.name == "Object")
					m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT);
				else
				{
					m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_PROTOTYPE);
					m_Serializer.String("proto", wstring_from_utf8(protoInfo.name), 0, 256);

					// Does it have custom Serialize function?
					// if so, we serialize the data it returns, rather than the object's properties directly
					if (protoInfo.hasCustomSerialize)
					{
						// If serialize is null, don't serialize anything more
						if (!protoInfo.hasNullSerialize)
						{
							JS::RootedValue data(rq.cx);
							if (!m_ScriptInterface.CallFunction(val, "Serialize", &data))
								throw PSERROR_Serialize_ScriptError("Prototype Serialize function failed");
							m_Serializer.ScriptVal("data", &data);
						}
						// Break here to skip the custom object property serialization logic below.
						break;
					}
				}
			}
			else if (protokey == JSProto_Number)
			{
				// Standard Number object
				m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_NUMBER);
				// Get primitive value
				double d;
				if (!JS::ToNumber(rq.cx, val, &d))
					throw PSERROR_Serialize_ScriptError("JS::ToNumber failed");
				m_Serializer.NumberDouble_Unbounded("value", d);
				break;
			}
			else if (protokey == JSProto_String)
			{
				// Standard String object
				m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_STRING);
				// Get primitive value
				JS::RootedString str(rq.cx, JS::ToString(rq.cx, val));
				if (!str)
					throw PSERROR_Serialize_ScriptError("JS_ValueToString failed");
				ScriptString("value", str);
				break;
			}
			else if (protokey == JSProto_Boolean)
			{
				// Standard Boolean object
				m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_BOOLEAN);
				// Get primitive value
				bool b = JS::ToBoolean(val);
				m_Serializer.Bool("value", b);
				break;
			}
			else
			{
				// Unrecognized class
				LOGERROR("Cannot serialise JS objects with unrecognized class '%s'", jsclass->name);
				throw PSERROR_Serialize_InvalidScriptValue();
			}
		}

		// Find all properties (ordered by insertion time)
		JS::Rooted<JS::IdVector> ida(rq.cx, JS::IdVector(rq.cx));
		if (!JS_Enumerate(rq.cx, obj, &ida))
			throw PSERROR_Serialize_ScriptError("JS_Enumerate failed");

		m_Serializer.NumberU32_Unbounded("num props", (u32)ida.length());

		for (size_t i = 0; i < ida.length(); ++i)
		{
			JS::RootedId id(rq.cx, ida[i]);

			JS::RootedValue idval(rq.cx);
			JS::RootedValue propval(rq.cx);

			// Forbid getters, which might delete values and mess things up.
			JS::Rooted<JS::PropertyDescriptor> desc(rq.cx);
			if (!JS_GetPropertyDescriptorById(rq.cx, obj, id, &desc))
				throw PSERROR_Serialize_ScriptError("JS_GetPropertyDescriptorById failed");
			if (desc.hasGetterObject())
				throw PSERROR_Serialize_ScriptError("Cannot serialize property getters");

			// Get the property name as a string
			if (!JS_IdToValue(rq.cx, id, &idval))
				throw PSERROR_Serialize_ScriptError("JS_IdToValue failed");
			JS::RootedString idstr(rq.cx, JS::ToString(rq.cx, idval));
			if (!idstr)
				throw PSERROR_Serialize_ScriptError("JS_ValueToString failed");

			ScriptString("prop name", idstr);

			if (!JS_GetPropertyById(rq.cx, obj, id, &propval))
				throw PSERROR_Serialize_ScriptError("JS_GetPropertyById failed");

			HandleScriptVal(propval);
		}

		break;
	}
	case JSTYPE_FUNCTION:
	{
		// We can't serialise functions, but we can at least name the offender (hopefully)
		std::wstring funcname(L"(unnamed)");
		JS::RootedFunction func(rq.cx, JS_ValueToFunction(rq.cx, val));
		if (func)
		{
			JS::RootedString string(rq.cx, JS_GetFunctionId(func));
			if (string)
			{
				if (JS_StringHasLatin1Chars(string))
				{
					size_t length;
					JS::AutoCheckCannotGC nogc;
					const JS::Latin1Char* ch = JS_GetLatin1StringCharsAndLength(rq.cx, nogc, string, &length);
					if (ch && length > 0)
						funcname.assign(ch, ch + length);
				}
				else
				{
					size_t length;
					JS::AutoCheckCannotGC nogc;
					const char16_t* ch = JS_GetTwoByteStringCharsAndLength(rq.cx, nogc, string, &length);
					if (ch && length > 0)
						funcname.assign(ch, ch + length);
				}
			}
		}

		LOGERROR("Cannot serialise JS objects of type 'function': %s", utf8_from_wstring(funcname));
		throw PSERROR_Serialize_InvalidScriptValue();
	}
	case JSTYPE_STRING:
	{
		m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_STRING);
		JS::RootedString stringVal(rq.cx, val.toString());
		ScriptString("string", stringVal);
		break;
	}
	case JSTYPE_NUMBER:
	{
		// To reduce the size of the serialized data, we handle integers and doubles separately.
		// We can't check for val.isInt32 and val.isDouble directly, because integer numbers are not guaranteed
		// to be represented as integers. A number like 33 could be stored as integer on the computer of one player
		// and as double on the other player's computer. That would cause out of sync errors in multiplayer games because
		// their binary representation and thus the hash would be different.

		double d;
		d = val.toNumber();
		i32 integer;

		if (JS_DoubleIsInt32(d, &integer))
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_INT);
			m_Serializer.NumberI32_Unbounded("value", integer);
		}
		else
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_DOUBLE);
			m_Serializer.NumberDouble_Unbounded("value", d);
		}
		break;
	}
	case JSTYPE_BOOLEAN:
	{
		m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_BOOLEAN);
		bool b = val.toBoolean();
		m_Serializer.NumberU8_Unbounded("value", b ? 1 : 0);
		break;
	}
	default:
	{
		debug_warn(L"Invalid TypeOfValue");
		throw PSERROR_Serialize_InvalidScriptValue();
	}
	}
}

void CBinarySerializerScriptImpl::ScriptString(const char* name, JS::HandleString string)
{
	ScriptRequest rq(m_ScriptInterface);

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: probably need to convert JS strings to little-endian
#endif

	size_t length;
	JS::AutoCheckCannotGC nogc;
	// Serialize strings directly as UTF-16 or Latin1, to avoid expensive encoding conversions
	bool isLatin1 = JS_StringHasLatin1Chars(string);
	m_Serializer.Bool("isLatin1", isLatin1);
	if (isLatin1)
	{
		const JS::Latin1Char* chars = JS_GetLatin1StringCharsAndLength(rq.cx, nogc, string, &length);
		if (!chars)
			throw PSERROR_Serialize_ScriptError("JS_GetLatin1StringCharsAndLength failed");
		m_Serializer.NumberU32_Unbounded("string length", (u32)length);
		m_Serializer.RawBytes(name, (const u8*)chars, length);
	}
	else
	{
		const char16_t* chars = JS_GetTwoByteStringCharsAndLength(rq.cx, nogc, string, &length);

		if (!chars)
			throw PSERROR_Serialize_ScriptError("JS_GetTwoByteStringCharsAndLength failed");
		m_Serializer.NumberU32_Unbounded("string length", (u32)length);
		m_Serializer.RawBytes(name, (const u8*)chars, length*2);
	}
}

i32 CBinarySerializerScriptImpl::GetScriptBackrefTag(JS::HandleObject obj)
{
	// To support non-tree structures (e.g. "var x = []; var y = [x, x];"), we need a way
	// to indicate multiple references to one object(/array). So every time we serialize a
	// new object, we give it a new tag; when we serialize it a second time we just refer
	// to that tag.
	//
	// Tags are stored on the object. To avoid overwriting any existing property,
	// they are saved as a uniquely-named, non-enumerable property (the serializer's unique symbol).

	ScriptRequest rq(m_ScriptInterface);

	JS::RootedValue symbolValue(rq.cx, JS::SymbolValue(m_ScriptBackrefSymbol));
	JS::RootedId symbolId(rq.cx);
	ENSURE(JS_ValueToId(rq.cx, symbolValue, &symbolId));

	JS::RootedValue tagValue(rq.cx);

	// If it was already there, return the tag
	bool tagFound;
	ENSURE(JS_HasPropertyById(rq.cx, obj, symbolId, &tagFound));
	if (tagFound)
	{
		ENSURE(JS_GetPropertyById(rq.cx, obj, symbolId, &tagValue));
		ENSURE(tagValue.isInt32());
		return tagValue.toInt32();
	}

	tagValue = JS::Int32Value(m_ScriptBackrefsNext);
	// TODO: this fails if the object cannot be written to.
	// This means we could end up in an infinite loop...
	if (!JS_DefinePropertyById(rq.cx, obj, symbolId, tagValue, JSPROP_READONLY))
	{
		// For now just warn, this should be user-fixable and may not actually error out.
		JS::RootedValue objVal(rq.cx, JS::ObjectValue(*obj.get()));
		LOGWARNING("Serialization symbol cannot be written on object %s", m_ScriptInterface.ToString(&objVal));
	}

	++m_ScriptBackrefsNext;
	// Return a non-tag number so callers know they need to serialize the object
	return -1;
}
