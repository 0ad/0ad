#ifndef _Serialization_H
#define _Serialization_H

#include "types.h"
#include "misc.h"

#define Serialize_int_1(_pos, _val) \
	STMT( *((_pos)++) = _val&0xff; )

#define Serialize_int_2(_pos, _val) STMT(\
	Serialize_int_1(_pos, _val>>8); \
	Serialize_int_1(_pos, _val); \
)

#define Serialize_int_3(_pos, _val) STMT(\
	Serialize_int_1(_pos, _val>>16); \
	Serialize_int_2(_pos, _val); \
)

#define Serialize_int_4(_pos, _val) STMT(\
	Serialize_int_1(_pos, _val>>24); \
	Serialize_int_3(_pos, _val); \
)

#define Serialize_int_8(_pos, _val) STMT(\
	Serialize_int_4(_pos, _val>>32); \
	Serialize_int_4(_pos, _val); \
)

#define Deserialize_int_1(_pos, _val) STMT(\
	(_val) -= (_val) & 0xff; \
	(_val) += *((_pos)++); )

#define Deserialize_int_2(_pos, _val) STMT(\
	Deserialize_int_1(_pos, _val); \
	_val <<= 8; \
	Deserialize_int_1(_pos, _val); )

#define Deserialize_int_3(_pos, _val) STMT(\
	Deserialize_int_2(_pos, _val); \
	_val <<= 8; \
	Deserialize_int_1(_pos, _val); )

#define Deserialize_int_4(_pos, _val) STMT(\
	Deserialize_int_3(_pos, _val); \
	_val <<= 8; \
	Deserialize_int_1(_pos, _val); )

#define Deserialize_int_8(_pos, _val) STMT(\
	Deserialize_int_4(_pos, _val); \
	_val <<= 8; \
	Deserialize_int_4(_pos, _val); )

/*#define Serialize_CStr(_pos, _str) STMT( \
	uint len=_str.Length(); \
	Serialize_int_4(_pos, len); \
	memcpy(_pos, _str, len+1); _pos += len+1; )

#define Deserialize_CStr(_pos, _str) STMT( \
	uint len; Deserialize_int_4(_pos, len); \
	_str=CStr((char *)_pos); _pos += len+1; )*/

/**
 * An interface for serializable objects. For a serializable object to be usable
 * as a network message field, it must have a constructor without arguments.
 */
class ISerializable
{
public:
	/**
	 * Return the length of the serialized form of this object
	 */
	virtual uint GetSerializedLength() const = 0;
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
