/* Copyright (C) 2014 Wildfire Games.
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
#include "scriptinterface/ScriptExtraHeaders.h" // for JSDOUBLE_IS_INT32, typed arrays
#include "SerializedScriptTypes.h"

static u8 GetArrayType(JSArrayBufferViewType arrayType)
{
	switch(arrayType)
	{
	case js::ArrayBufferView::TYPE_INT8:
		return SCRIPT_TYPED_ARRAY_INT8;
	case js::ArrayBufferView::TYPE_UINT8:
		return SCRIPT_TYPED_ARRAY_UINT8;
	case js::ArrayBufferView::TYPE_INT16:
		return SCRIPT_TYPED_ARRAY_INT16;
	case js::ArrayBufferView::TYPE_UINT16:
		return SCRIPT_TYPED_ARRAY_UINT16;
	case js::ArrayBufferView::TYPE_INT32:
		return SCRIPT_TYPED_ARRAY_INT32;
	case js::ArrayBufferView::TYPE_UINT32:
		return SCRIPT_TYPED_ARRAY_UINT32;
	case js::ArrayBufferView::TYPE_FLOAT32:
		return SCRIPT_TYPED_ARRAY_FLOAT32;
	case js::ArrayBufferView::TYPE_FLOAT64:
		return SCRIPT_TYPED_ARRAY_FLOAT64;
	case js::ArrayBufferView::TYPE_UINT8_CLAMPED:
		return SCRIPT_TYPED_ARRAY_UINT8_CLAMPED;
	default:
		LOGERROR(L"Cannot serialize unrecognized typed array view: %d", arrayType);
		throw PSERROR_Serialize_InvalidScriptValue();
	}
}

CBinarySerializerScriptImpl::CBinarySerializerScriptImpl(ScriptInterface& scriptInterface, ISerializer& serializer) :
	m_ScriptInterface(scriptInterface), m_Serializer(serializer), m_Rooter(m_ScriptInterface),
	m_ScriptBackrefsArena(1 * MiB), m_ScriptBackrefs(backrefs_t::key_compare(), ScriptBackrefsAlloc(m_ScriptBackrefsArena)), m_ScriptBackrefsNext(1)
{
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
			JS::RootedValue bufferVal(cx, JS::ObjectValue(*JS_GetArrayBufferViewBuffer(obj)));
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
			m_Serializer.RawBytes("buffer data", (const u8*)JS_GetArrayBufferData(obj), length);
			break;
		}
		else
		{
			// Find type of object
			JSClass* jsclass = JS_GetClass(obj);
			if (!jsclass)
				throw PSERROR_Serialize_ScriptError("JS_GetClass failed");
			JSProtoKey protokey = JSCLASS_CACHED_PROTO_KEY(jsclass);

			if (protokey == JSProto_Object)
			{
				// Object class - check for user-defined prototype
				JS::RootedObject proto(cx);
				JS_GetPrototype(cx, obj, proto.address());
				if (!proto)
					throw PSERROR_Serialize_ScriptError("JS_GetPrototype failed");

				if (m_SerializablePrototypes.empty() || !IsSerializablePrototype(proto))
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

					const std::wstring& prototypeName = GetPrototypeName(proto);
					m_Serializer.String("proto name", prototypeName, 0, 256);

					// Does it have custom Serialize function?
					// if so, we serialize the data it returns, rather than the object's properties directly
					JSBool hasCustomSerialize;
					if (!JS_HasProperty(cx, obj, "Serialize", &hasCustomSerialize))
						throw PSERROR_Serialize_ScriptError("JS_HasProperty failed");

					if (hasCustomSerialize)
					{
						JS::RootedValue serialize(cx);
						if (!JS_LookupProperty(cx, obj, "Serialize", serialize.address()))
							throw PSERROR_Serialize_ScriptError("JS_LookupProperty failed");

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
				JSString* str = JS_ValueToString(cx, val);
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
				LOGERROR(L"Cannot serialise JS objects with unrecognized class '%hs'", jsclass->name);
				throw PSERROR_Serialize_InvalidScriptValue();
			}
		}

		// Find all properties (ordered by insertion time)

		// (Note that we don't do any rooting, because we assume nothing is going to trigger GC.
		// I'm not absolute certain that's necessarily a valid assumption.)

		JS::AutoIdArray ida (cx, JS_Enumerate(cx, obj));
		if (!ida)
			throw PSERROR_Serialize_ScriptError("JS_Enumerate failed");

		m_Serializer.NumberU32_Unbounded("num props", (u32)ida.length());

		for (size_t i = 0; i < ida.length(); ++i)
		{
			jsid id = ida[i];

			JS::RootedValue idval(cx);
			JS::RootedValue propval(cx);
			
			// Get the property name as a string
			if (!JS_IdToValue(cx, id, idval.address()))
				throw PSERROR_Serialize_ScriptError("JS_IdToValue failed");
			JSString* idstr = JS_ValueToString(cx, idval.get());
			if (!idstr)
				throw PSERROR_Serialize_ScriptError("JS_ValueToString failed");

			ScriptString("prop name", idstr);

			// Use LookupProperty instead of GetProperty to avoid the danger of getters
			// (they might delete values and trigger GC)
			if (!JS_LookupPropertyById(cx, obj, id, propval.address()))
				throw PSERROR_Serialize_ScriptError("JS_LookupPropertyById failed");

			HandleScriptVal(propval);
		}

		break;
	}
	case JSTYPE_FUNCTION:
	{
		// We can't serialise functions, but we can at least name the offender (hopefully)
		std::wstring funcname(L"(unnamed)");
		JSFunction* func = JS_ValueToFunction(cx, val);
		if (func)
		{
			JSString* string = JS_GetFunctionId(func);
			if (string)
			{
				size_t length;
				const jschar* ch = JS_GetStringCharsAndLength(cx, string, &length);
				if (ch && length > 0)
					funcname = std::wstring(ch, ch + length);
			}
		}

		LOGERROR(L"Cannot serialise JS objects of type 'function': %ls", funcname.c_str());
		throw PSERROR_Serialize_InvalidScriptValue();
	}
	case JSTYPE_STRING:
	{
		m_Serializer.NumberU8_Unbounded("type", SCRIPT_TYPE_STRING);
		ScriptString("string", val.toString());
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
		bool b = JSVAL_TO_BOOLEAN(val);
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

void CBinarySerializerScriptImpl::ScriptString(const char* name, JSString* string)
{
	JSContext* cx = m_ScriptInterface.GetContext();
	JSAutoRequest rq(cx);

	size_t length;
	const jschar* chars = JS_GetStringCharsAndLength(cx, string, &length);

	if (!chars)
		throw PSERROR_Serialize_ScriptError("JS_GetStringCharsAndLength failed");

#if BYTE_ORDER != LITTLE_ENDIAN
#error TODO: probably need to convert JS strings to little-endian
#endif

	// Serialize strings directly as UTF-16, to avoid expensive encoding conversions
	m_Serializer.NumberU32_Unbounded("string length", (u32)length);
	m_Serializer.RawBytes(name, (const u8*)chars, length*2);
}

u32 CBinarySerializerScriptImpl::GetScriptBackrefTag(JSObject* obj)
{
	// To support non-tree structures (e.g. "var x = []; var y = [x, x];"), we need a way
	// to indicate multiple references to one object(/array). So every time we serialize a
	// new object, we give it a new non-zero tag; when we serialize it a second time we just
	// refer to that tag.
	//
	// The tags are stored in a map. Maybe it'd be more efficient to store it inline in the object
	// somehow? but this works okay for now

	std::pair<backrefs_t::iterator, bool> it = m_ScriptBackrefs.insert(std::make_pair(obj, m_ScriptBackrefsNext));

	// If it was already there, return the tag
	if (!it.second)
		return it.first->second;

	// If it was newly inserted, we need to make sure it gets rooted
	// for the duration that it's in m_ScriptBackrefs
	m_Rooter.Push(it.first->first);
	m_ScriptBackrefsNext++;
	// Return a non-tag number so callers know they need to serialize the object
	return 0;
}

bool CBinarySerializerScriptImpl::IsSerializablePrototype(JSObject* prototype)
{
	return m_SerializablePrototypes.find(prototype) != m_SerializablePrototypes.end();
}

std::wstring CBinarySerializerScriptImpl::GetPrototypeName(JSObject* prototype)
{
	std::map<JSObject*, std::wstring>::iterator it = m_SerializablePrototypes.find(prototype);
	ENSURE(it != m_SerializablePrototypes.end());
	return it->second;
}

void CBinarySerializerScriptImpl::SetSerializablePrototypes(std::map<JSObject*, std::wstring>& prototypes)
{
	m_SerializablePrototypes = prototypes;
}
