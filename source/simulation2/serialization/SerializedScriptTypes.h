/* Copyright (C) 2015 Wildfire Games.
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
	SCRIPT_TYPE_OBJECT_PROTOTYPE = 11,	// user-defined prototype
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

#endif // INCLUDED_SERIALIZEDSCRIPTTYPES
