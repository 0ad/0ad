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

#ifndef INCLUDED_SERIALIZEDSCRIPTTYPES
#define INCLUDED_SERIALIZEDSCRIPTTYPES

enum
{
	SCRIPT_TYPE_VOID = 0,
	SCRIPT_TYPE_NULL = 1,
	SCRIPT_TYPE_ARRAY = 2,
	SCRIPT_TYPE_OBJECT = 3,				// standard Object prototype
	SCRIPT_TYPE_STRING = 4,
	SCRIPT_TYPE_INT = 5,
	SCRIPT_TYPE_DOUBLE = 6,
	SCRIPT_TYPE_BOOLEAN = 7,
	SCRIPT_TYPE_BACKREF = 8,
	SCRIPT_TYPE_TYPED_ARRAY = 9,		// ArrayBufferView subclasses - see below
	SCRIPT_TYPE_ARRAY_BUFFER = 10,		// ArrayBuffer containing actual typed array data (may be shared by multiple views)
	SCRIPT_TYPE_OBJECT_PROTOTYPE = 11,	// User-defined prototype - see GetPrototypeInfo
	SCRIPT_TYPE_OBJECT_NUMBER = 12,		// standard Number class
	SCRIPT_TYPE_OBJECT_STRING = 13,		// standard String class
	SCRIPT_TYPE_OBJECT_BOOLEAN = 14,	// standard Boolean class
	SCRIPT_TYPE_OBJECT_MAP = 15,		// Map class
	SCRIPT_TYPE_OBJECT_SET = 16			// Set class
};

// ArrayBufferView subclasses (to avoid relying directly on the JSAPI enums)
enum
{
	SCRIPT_TYPED_ARRAY_INT8 = 0,
	SCRIPT_TYPED_ARRAY_UINT8 = 1,
	SCRIPT_TYPED_ARRAY_INT16 = 2,
	SCRIPT_TYPED_ARRAY_UINT16 = 3,
	SCRIPT_TYPED_ARRAY_INT32 = 4,
	SCRIPT_TYPED_ARRAY_UINT32 = 5,
	SCRIPT_TYPED_ARRAY_FLOAT32 = 6,
	SCRIPT_TYPED_ARRAY_FLOAT64 = 7,
	SCRIPT_TYPED_ARRAY_UINT8_CLAMPED = 8
};

struct SPrototypeSerialization
{
	std::string name = "";
	bool hasCustomSerialize = false;
	bool hasCustomDeserialize = false;
	bool hasNullSerialize = false;
};

inline SPrototypeSerialization GetPrototypeInfo(const ScriptRequest& rq, JS::HandleObject prototype)
{
	SPrototypeSerialization ret;

	JS::RootedValue constructor(rq.cx, JS::ObjectOrNullValue(JS_GetConstructor(rq.cx, prototype)));
	if (!ScriptInterface::GetProperty(rq, constructor, "name", ret.name))
		throw PSERROR_Serialize_ScriptError("Could not get constructor name.");

	// Nothing to do for basic Object objects.
	if (ret.name == "Object")
		return ret;

	if (!JS_HasProperty(rq.cx, prototype, "Serialize", &ret.hasCustomSerialize) ||
	     !JS_HasProperty(rq.cx, prototype, "Deserialize", &ret.hasCustomDeserialize))
		throw PSERROR_Serialize_ScriptError("JS_HasProperty failed");

	if (ret.hasCustomSerialize)
	{
		JS::RootedValue serialize(rq.cx);
		if (!JS_GetProperty(rq.cx, prototype, "Serialize", &serialize))
			throw PSERROR_Serialize_ScriptError("JS_GetProperty failed");

		if (serialize.isNull())
			ret.hasNullSerialize = true;
		else if (!ret.hasCustomDeserialize)
			throw PSERROR_Serialize_ScriptError("Cannot serialize script with non-null Serialize but no Deserialize.");
	}
	return ret;
}

#endif // INCLUDED_SERIALIZEDSCRIPTTYPES
