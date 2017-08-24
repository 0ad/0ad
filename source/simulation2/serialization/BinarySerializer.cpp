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
	m_ScriptInterface(scriptInterface), m_Serializer(serializer), m_ScriptBackrefs(scriptInterface.GetRuntime()),
	m_SerializablePrototypes(new ObjectIdCache<std::wstring>(scriptInterface.GetRuntime())), m_ScriptBackrefsNext(1)
{
	m_ScriptBackrefs.init();
	m_SerializablePrototypes->init();
}

void CBinarySerializerScriptImpl::HandleScriptVal(JS::HandleValue val)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	switch (JS_TypeOfValue(cx, val))
	{
	case JSTYPE_VOID:
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

		JS::RootedObject obj(cx, &val.toObject());

		// If we've already serialized this object, just output a reference to it
		u32 tag = GetScriptBackrefTag(obj);
		if (tag)
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_BACKREF);
			m_Serializer.NumberU32_Unbounded("tag", tag);
			break;
		}

		// Arrays are special cases of Object
		if (JS_IsArrayObject(cx, obj))
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_ARRAY);
			// TODO: probably should have a more efficient storage format

			// Arrays like [1, 2, ] have an 'undefined' at the end which is part of the
			// length but seemingly isn't enumerated, so store the length explicitly
			uint length = 0;
			if (!JS_GetArrayLength(cx, obj, &length))
				throw PSERROR_Serialize_ScriptError("JS_GetArrayLength failed");
			m_Serializer.NumberU32_Unbounded("array length", length);
		}
		else if (JS_IsTypedArrayObject(obj))
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_TYPED_ARRAY);

			m_Serializer.NumberU8_Unbounded("array type", GetArrayType(JS_GetArrayBufferViewType(obj)));
			m_Serializer.NumberU32_Unbounded("byte offset", JS_GetTypedArrayByteOffset(obj));
			m_Serializer.NumberU32_Unbounded("length", JS_GetTypedArrayLength(obj));

			// Now handle its array buffer
			// this may be a backref, since ArrayBuffers can be shared by multiple views
			JS::RootedValue bufferVal(cx, JS::ObjectValue(*JS_GetArrayBufferViewBuffer(cx, obj)));
			HandleScriptVal(bufferVal);
			break;
		}
		else if (JS_IsArrayBufferObject(obj))
		{
			m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_ARRAY_BUFFER);

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: need to convert JS ArrayBuffer data to little-endian
#endif

			u32 length = JS_GetArrayBufferByteLength(obj);
			m_Serializer.NumberU32_Unbounded("buffer length", length);
			JS::AutoCheckCannotGC nogc;
			m_Serializer.RawBytes("buffer data", (const u8*)JS_GetArrayBufferData(obj, nogc), length);
			break;
		}
		else
		{
			// Find type of object
			const JSClass* jsclass = JS_GetClass(obj);
			if (!jsclass)
				throw PSERROR_Serialize_ScriptError("JS_GetClass failed");
// TODO: Remove this workaround for upstream API breakage when updating SpiderMonkey
// See https://bugzilla.mozilla.org/show_bug.cgi?id=1236373
#define JSCLASS_CACHED_PROTO_WIDTH js::JSCLASS_CACHED_PROTO_WIDTH
			JSProtoKey protokey = JSCLASS_CACHED_PROTO_KEY(jsclass);
#undef JSCLASS_CACHED_PROTO_WIDTH

			if (protokey == JSProto_Object)
			{
				// Object class - check for user-defined prototype
				JS::RootedObject proto(cx);
				JS_GetPrototype(cx, obj, &proto);
				if (!proto)
					throw PSERROR_Serialize_ScriptError("JS_GetPrototype failed");

				if (m_SerializablePrototypes->empty() || !IsSerializablePrototype(proto))
				{
					// Standard Object prototype
					m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT);

					// TODO: maybe we should throw an error for unrecognized non-Object prototypes?
					//	(requires fixing AI serialization first and excluding component scripts)
				}
				else
				{
					// User-defined custom prototype
					m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_PROTOTYPE);

					const std::wstring prototypeName = GetPrototypeName(proto);
					m_Serializer.String("proto name", prototypeName, 0, 256);

					// Does it have custom Serialize function?
					// if so, we serialize the data it returns, rather than the object's properties directly
					bool hasCustomSerialize;
					if (!JS_HasProperty(cx, obj, "Serialize", &hasCustomSerialize))
						throw PSERROR_Serialize_ScriptError("JS_HasProperty failed");

					if (hasCustomSerialize)
					{
						JS::RootedValue serialize(cx);
						if (!JS_GetProperty(cx, obj, "Serialize", &serialize))
							throw PSERROR_Serialize_ScriptError("JS_GetProperty failed");

						// If serialize is null, so don't serialize anything more
						if (!serialize.isNull())
						{
							JS::RootedValue data(cx);
							if (!m_ScriptInterface.CallFunction(val, "Serialize", &data))
								throw PSERROR_Serialize_ScriptError("Prototype Serialize function failed");
							HandleScriptVal(data);
						}
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
				if (!JS::ToNumber(cx, val, &d))
					throw PSERROR_Serialize_ScriptError("JS::ToNumber failed");
				m_Serializer.NumberDouble_Unbounded("value", d);
				break;
			}
			else if (protokey == JSProto_String)
			{
				// Standard String object
				m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_STRING);
				// Get primitive value
				JS::RootedString str(cx, JS::ToString(cx, val));
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
			// TODO: Follow upstream progresses about a JS::IsMapObject
			// https://bugzilla.mozilla.org/show_bug.cgi?id=1285909
			else if (protokey == JSProto_Map)
			{
				m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_MAP);
				m_Serializer.NumberU32_Unbounded("map size", JS::MapSize(cx, obj));

				JS::RootedValue keyValueIterator(cx);
				if (!JS::MapEntries(cx, obj, &keyValueIterator))
					throw PSERROR_Serialize_ScriptError("JS::MapEntries failed");

				JS::ForOfIterator it(cx);
				if (!it.init(keyValueIterator))
					throw PSERROR_Serialize_ScriptError("JS::ForOfIterator::init failed");

				JS::RootedValue keyValuePair(cx);
				bool done;
				while (true)
				{
					if (!it.next(&keyValuePair, &done))
						throw PSERROR_Serialize_ScriptError("JS::ForOfIterator::next failed");

					if (done)
						break;

					JS::RootedObject keyValuePairObj(cx, &keyValuePair.toObject());
					JS::RootedValue key(cx);
					JS::RootedValue value(cx);
					ENSURE(JS_GetElement(cx, keyValuePairObj, 0, &key));
					ENSURE(JS_GetElement(cx, keyValuePairObj, 1, &value));

					HandleScriptVal(key);
					HandleScriptVal(value);
				}
				break;
			}
			// TODO: Follow upstream progresses about a JS::IsSetObject
			// https://bugzilla.mozilla.org/show_bug.cgi?id=1285909
			else if (protokey == JSProto_Set)
			{
				// TODO: When updating SpiderMonkey to a release after 38 use the C++ API for Sets.
				// https://bugzilla.mozilla.org/show_bug.cgi?id=1159469
				u32 setSize;
				m_ScriptInterface.GetProperty(val, "size", setSize);

				m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_OBJECT_SET);
				m_Serializer.NumberU32_Unbounded("set size", setSize);

				JS::RootedValue valueIterator(cx);
				m_ScriptInterface.CallFunction(val, "values", &valueIterator);
				for (u32 i=0; i<setSize; ++i)
				{
					JS::RootedValue currentIterator(cx);
					JS::RootedValue value(cx);
					ENSURE(m_ScriptInterface.CallFunction(valueIterator, "next", &currentIterator));

					m_ScriptInterface.GetProperty(currentIterator, "value", &value);

					HandleScriptVal(value);
				}

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
		JS::AutoIdArray ida (cx, JS_Enumerate(cx, obj));
		if (!ida)
			throw PSERROR_Serialize_ScriptError("JS_Enumerate failed");

		m_Serializer.NumberU32_Unbounded("num props", (u32)ida.length());

		for (size_t i = 0; i < ida.length(); ++i)
		{
			JS::RootedId id(cx, ida[i]);

			JS::RootedValue idval(cx);
			JS::RootedValue propval(cx);

			// Forbid getters, which might delete values and mess things up.
			JS::Rooted<JSPropertyDescriptor> desc(cx);
			if (!JS_GetPropertyDescriptorById(cx, obj, id, &desc))
				throw PSERROR_Serialize_ScriptError("JS_GetPropertyDescriptorById failed");
			if (desc.hasGetterObject())
				throw PSERROR_Serialize_ScriptError("Cannot serialize property getters");

			// Get the property name as a string
			if (!JS_IdToValue(cx, id, &idval))
				throw PSERROR_Serialize_ScriptError("JS_IdToValue failed");
			JS::RootedString idstr(cx, JS::ToString(cx, idval));
			if (!idstr)
				throw PSERROR_Serialize_ScriptError("JS_ValueToString failed");

			ScriptString("prop name", idstr);

			if (!JS_GetPropertyById(cx, obj, id, &propval))
				throw PSERROR_Serialize_ScriptError("JS_GetPropertyById failed");

			HandleScriptVal(propval);
		}

		break;
	}
	case JSTYPE_FUNCTION:
	{
		// We can't serialise functions, but we can at least name the offender (hopefully)
		std::wstring funcname(L"(unnamed)");
		JS::RootedFunction func(cx, JS_ValueToFunction(cx, val));
		if (func)
		{
			JS::RootedString string(cx, JS_GetFunctionId(func));
			if (string)
			{
				if (JS_StringHasLatin1Chars(string))
				{
					size_t length;
					JS::AutoCheckCannotGC nogc;
					const JS::Latin1Char* ch = JS_GetLatin1StringCharsAndLength(cx, nogc, string, &length);
					if (ch && length > 0)
						funcname.assign(ch, ch + length);
				}
				else
				{
					size_t length;
					JS::AutoCheckCannotGC nogc;
					const char16_t* ch = JS_GetTwoByteStringCharsAndLength(cx, nogc, string, &length);
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
		JS::RootedString stringVal(cx, val.toString());
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
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

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
		const JS::Latin1Char* chars = JS_GetLatin1StringCharsAndLength(cx, nogc, string, &length);
		if (!chars)
			throw PSERROR_Serialize_ScriptError("JS_GetLatin1StringCharsAndLength failed");
		m_Serializer.NumberU32_Unbounded("string length", (u32)length);
		m_Serializer.RawBytes(name, (const u8*)chars, length);
	}
	else
	{
		const char16_t* chars = JS_GetTwoByteStringCharsAndLength(cx, nogc, string, &length);

		if (!chars)
			throw PSERROR_Serialize_ScriptError("JS_GetTwoByteStringCharsAndLength failed");
		m_Serializer.NumberU32_Unbounded("string length", (u32)length);
		m_Serializer.RawBytes(name, (const u8*)chars, length*2);
	}
}

u32 CBinarySerializerScriptImpl::GetScriptBackrefTag(JS::HandleObject obj)
{
	// To support non-tree structures (e.g. "var x = []; var y = [x, x];"), we need a way
	// to indicate multiple references to one object(/array). So every time we serialize a
	// new object, we give it a new non-zero tag; when we serialize it a second time we just
	// refer to that tag.
	//
	// The tags are stored in a map. Maybe it'd be more efficient to store it inline in the object
	// somehow? but this works okay for now

	// If it was already there, return the tag
	u32 tag;
	if (m_ScriptBackrefs.find(obj, tag))
		return tag;

	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	m_ScriptBackrefs.add(cx, obj, m_ScriptBackrefsNext);

	m_ScriptBackrefsNext++;
	// Return a non-tag number so callers know they need to serialize the object
	return 0;
}

bool CBinarySerializerScriptImpl::IsSerializablePrototype(JS::HandleObject prototype)
{
	return m_SerializablePrototypes->has(prototype);
}

std::wstring CBinarySerializerScriptImpl::GetPrototypeName(JS::HandleObject prototype)
{
	std::wstring ret;
	bool found = m_SerializablePrototypes->find(prototype, ret);
	ENSURE(found);
	return ret;
}

void CBinarySerializerScriptImpl::SetSerializablePrototypes(shared_ptr<ObjectIdCache<std::wstring> > prototypes)
{
	m_SerializablePrototypes = prototypes;
}
