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

#ifndef INCLUDED_NETWORK_SERIALIZATION
#define INCLUDED_NETWORK_SERIALIZATION

#define Serialize_int_1(_pos, _val) \
	STMT( *((_pos)++) = (u8)((_val)&0xff); )

#define Serialize_int_2(_pos, _val) STMT(\
	Serialize_int_1(_pos, (_val)>>8); \
	Serialize_int_1(_pos, (_val)); \
)

#define Serialize_int_3(_pos, _val) STMT(\
	Serialize_int_1(_pos, (_val)>>16); \
	Serialize_int_2(_pos, (_val)); \
)

#define Serialize_int_4(_pos, _val) STMT(\
	Serialize_int_1(_pos, (_val)>>24); \
	Serialize_int_3(_pos, (_val)); \
)

#define Serialize_int_8(_pos, _val) STMT(\
	Serialize_int_4(_pos, (_val)>>32); \
	Serialize_int_4(_pos, (_val)); \
)

#define __shift_de(_pos, _val) STMT( \
	(_val) <<= 8; \
	(_val) += *((_pos)++); )

#define Deserialize_int_1(_pos, _val) STMT(\
	(_val) = *((_pos)++); )

#define Deserialize_int_2(_pos, _val) STMT(\
	Deserialize_int_1(_pos, _val); \
	__shift_de(_pos, _val); )

#define Deserialize_int_3(_pos, _val) STMT(\
	Deserialize_int_2(_pos, _val); \
	__shift_de(_pos, _val); )

#define Deserialize_int_4(_pos, _val) STMT(\
	Deserialize_int_3(_pos, _val); \
	__shift_de(_pos, _val); )

#define Deserialize_int_8(_pos, _val) STMT(\
	uint32 _v1; uint32 _v2; \
	Deserialize_int_4(_pos, _v1); \
	Deserialize_int_4(_pos, _v2); \
	_val = _v1; \
	_val <<= 32;	/* janwas: careful! (uint32 << 32) = 0 */ \
	_val |= _v2; )

/**
 * An interface for serializable objects. For a serializable object to be usable
 * as a network message field, it must have a constructor without arguments.
 */
class ISerializable
{
public:
	virtual ~ISerializable() {}

	/**
	 * Return the length of the serialized form of this object
	 */
	virtual size_t GetSerializedLength() const = 0;
	/**
	 * Serialize the object into the passed buffer.
	 *
	 * @return a pointer to the location in the buffer right after the
	 * serialized object
	 */
	virtual u8 *Serialize(u8 *buffer) const = 0;
	/**
	 * Deserialize the object (i.e. read in data from the buffer and initialize
	 * the object's fields). Note that it is up to the deserializer to detect
	 * errors in data format.
	 *
	 * @param buffer A pointer pointing to the start of the serialized data.
	 * @param end A pointer to the end of the message.
	 *
	 * @returns a pointer to the location in the buffer right after the
	 * serialized object, or NULL if there was a data format error
	 */
	virtual const u8 *Deserialize(const u8 *buffer, const u8 *end) = 0;
};

#endif
