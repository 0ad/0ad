/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_IDESERIALIZER
#define INCLUDED_IDESERIALIZER

#include "maths/Fixed.h"
#include "ps/Errors.h"
#include "scriptinterface/ScriptTypes.h"

ERROR_GROUP(Deserialize);
ERROR_TYPE(Deserialize, OutOfBounds);
ERROR_TYPE(Deserialize, InvalidCharInString);
ERROR_TYPE(Deserialize, ReadFailed);
ERROR_TYPE(Deserialize, ScriptError);

/**
 * Deserialization interface; see \ref serialization "serialization overview".
 */
class IDeserializer
{
public:
	virtual ~IDeserializer();

	virtual void NumberU8(const char* name, uint8_t& out, uint8_t lower, uint8_t upper);
	virtual void NumberI8(const char* name, int8_t& out, int8_t lower, int8_t upper);
	virtual void NumberU16(const char* name, uint16_t& out, uint16_t lower, uint16_t upper);
	virtual void NumberI16(const char* name, int16_t& out, int16_t lower, int16_t upper);
	virtual void NumberU32(const char* name, uint32_t& out, uint32_t lower, uint32_t upper);
	virtual void NumberI32(const char* name, int32_t& out, int32_t lower, int32_t upper);
	virtual void NumberU8_Unbounded(const char* name, uint8_t& out);
	virtual void NumberI8_Unbounded(const char* name, int8_t& out);
	virtual void NumberU16_Unbounded(const char* name, uint16_t& out);
	virtual void NumberI16_Unbounded(const char* name, int16_t& out);
	virtual void NumberU32_Unbounded(const char* name, uint32_t& out);
	virtual void NumberI32_Unbounded(const char* name, int32_t& out);
	virtual void NumberFloat_Unbounded(const char* name, float& out);
	virtual void NumberDouble_Unbounded(const char* name, double& out);
	virtual void NumberFixed_Unbounded(const char* name, fixed& out);
	virtual void Bool(const char* name, bool& out);
	virtual void StringASCII(const char* name, std::string& out, uint32_t minlength, uint32_t maxlength);
	virtual void String(const char* name, std::wstring& out, uint32_t minlength, uint32_t maxlength);

	/// Deserialize a jsval, replacing 'out'
	virtual void ScriptVal(const char* name, JS::MutableHandleValue out) = 0;

	/// Deserialize an object value, appending properties to object 'objVal'
	virtual void ScriptObjectAppend(const char* name, JS::HandleValue objVal) = 0;

	/// Deserialize a JSString
	virtual void ScriptString(const char* name, JSString*& out) = 0;

	virtual void RawBytes(const char* name, u8* data, size_t len);

	// Features for simulation-state serialisation:
	virtual int GetVersion() const;

	/**
	 * Returns a stream which can be used to deserialize data directly.
	 * (This is particularly useful for chaining multiple deserializers
	 * together.)
	 */
	virtual std::istream& GetStream() = 0;

	/**
	 * Throws an exception if the stream definitely cannot provide the required
	 * number of bytes.
	 * (It might be conservative and *not* throw an exception in some cases where
	 * the stream actually can't provide the required bytes.)
	 * (This should be used when allocating memory based on data in the
	 * stream, e.g. reading strings, to avoid dangerously large allocations
	 * when the data is invalid.)
	 */
	virtual void RequireBytesInStream(size_t numBytes) = 0;

protected:
	virtual void Get(const char* name, u8* data, size_t len) = 0;
};

#endif // INCLUDED_IDESERIALIZER
