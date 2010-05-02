/* Copyright (C) 2010 Wildfire Games.
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
#include "ps/utf16string.h"
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

	virtual void NumberU8(uint8_t& out, uint8_t lower, uint8_t upper);
	virtual void NumberI32(int32_t& out, int32_t lower, int32_t upper);
	virtual void NumberU32(uint32_t& out, uint32_t lower, uint32_t upper);
	virtual void NumberU8_Unbounded(uint8_t& out);
	virtual void NumberI32_Unbounded(int32_t& out);
	virtual void NumberU32_Unbounded(uint32_t& out);
	virtual void NumberFloat_Unbounded(float& out);
	virtual void NumberDouble_Unbounded(double& out);
	virtual void NumberFixed_Unbounded(fixed& out);
	virtual void Bool(bool& out);
	virtual void StringASCII(std::string& out, uint32_t minlength, uint32_t maxlength);
	virtual void String(std::wstring& out, uint32_t minlength, uint32_t maxlength);
	virtual void StringUTF16(utf16string& out);

	/// Deserialize a jsval, replacing 'out'
	virtual void ScriptVal(jsval& out) = 0;

	/// Deserialize an object jsval, appending properties to object 'obj'
	virtual void ScriptObjectAppend(jsval& obj) = 0;

	/// Deserialize a JSString
	virtual void ScriptString(JSString*& out) = 0;

	virtual void RawBytes(u8* data, size_t len);

	// Features for simulation-state serialisation:
	virtual int GetVersion() const;

protected:
	virtual void ReadString(std::string& out);

	virtual void Get(u8* data, size_t len) = 0;

private:
	// Helper function for bounded number types
	template<typename T> T Number_(T lower, T upper);
};

#endif // INCLUDED_IDESERIALIZER
