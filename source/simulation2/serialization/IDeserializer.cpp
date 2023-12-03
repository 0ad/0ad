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

#include "precompiled.h"

#include "IDeserializer.h"

#include "lib/byte_order.h"
#include "lib/utf8.h"
#include "ps/CStr.h"

IDeserializer::~IDeserializer()
{
}

void IDeserializer::NumberU8(const char* name, uint8_t& out, uint8_t lower, uint8_t upper)
{
	uint8_t value;
	Get(name, (u8*)&value, sizeof(uint8_t));

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds(name);

	out = value;
}

void IDeserializer::NumberI8(const char* name, int8_t& out, int8_t lower, int8_t upper)
{
	int8_t value;
	Get(name, (u8*)&value, sizeof(uint8_t));

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds(name);

	out = value;
}

void IDeserializer::NumberU16(const char* name, uint16_t& out, uint16_t lower, uint16_t upper)
{
	uint16_t value;
	Get(name, (u8*)&value, sizeof(uint16_t));
	value = to_le16(value);

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds(name);

	out = value;
}

void IDeserializer::NumberI16(const char* name, int16_t& out, int16_t lower, int16_t upper)
{
	int16_t value;
	Get(name, (u8*)&value, sizeof(uint16_t));
	value = (i16)to_le16((u16)value);

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds(name);

	out = value;
}

void IDeserializer::NumberU32(const char* name, uint32_t& out, uint32_t lower, uint32_t upper)
{
	uint32_t value;
	Get(name, (u8*)&value, sizeof(uint32_t));
	value = to_le32(value);

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds(name);

	out = value;
}

void IDeserializer::NumberI32(const char* name, int32_t& out, int32_t lower, int32_t upper)
{
	int32_t value;
	Get(name, (u8*)&value, sizeof(uint32_t));
	value = (i32)to_le32((u32)value);

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds(name);

	out = value;
}

void IDeserializer::NumberU8_Unbounded(const char* name, uint8_t& out)
{
	Get(name, (u8*)&out, sizeof(uint8_t));
}

void IDeserializer::NumberI8_Unbounded(const char* name, int8_t& out)
{
	Get(name, (u8*)&out, sizeof(int8_t));
}

void IDeserializer::NumberU16_Unbounded(const char* name, uint16_t& out)
{
	uint16_t value;
	Get(name, (u8*)&value, sizeof(uint16_t));
	out = to_le16(value);
}

void IDeserializer::NumberI16_Unbounded(const char* name, int16_t& out)
{
	int16_t value;
	Get(name, (u8*)&value, sizeof(int16_t));
	out = (i16)to_le16((u16)value);
}

void IDeserializer::NumberU32_Unbounded(const char* name, uint32_t& out)
{
	uint32_t value;
	Get(name, (u8*)&value, sizeof(uint32_t));
	out = to_le32(value);
}

void IDeserializer::NumberI32_Unbounded(const char* name, int32_t& out)
{
	int32_t value;
	Get(name, (u8*)&value, sizeof(int32_t));
	out = (i32)to_le32((u32)value);
}

void IDeserializer::NumberFloat_Unbounded(const char* name, float& out)
{
	Get(name, (u8*)&out, sizeof(float));
}

void IDeserializer::NumberDouble_Unbounded(const char* name, double& out)
{
	Get(name, (u8*)&out, sizeof(double));
}

void IDeserializer::NumberFixed_Unbounded(const char* name, fixed& out)
{
	int32_t n;
	NumberI32_Unbounded(name, n);
	out.SetInternalValue(n);
}

void IDeserializer::Bool(const char* name, bool& out)
{
	uint8_t i;
	NumberU8(name, i, 0, 1);
	out = (i != 0);
}

void IDeserializer::StringASCII(const char* name, std::string& out, uint32_t minlength, uint32_t maxlength)
{
	uint32_t len;
	NumberU32("string length", len, minlength, maxlength);

	RequireBytesInStream(len);
	out.resize(len);
	Get(name, (u8*)out.data(), len);

	for (size_t i = 0; i < out.length(); ++i)
		if (out[i] == 0 || (unsigned char)out[i] >= 128)
			throw PSERROR_Deserialize_InvalidCharInString();
}

void IDeserializer::String(const char* name, std::wstring& out, uint32_t minlength, uint32_t maxlength)
{
	std::string str;
	uint32_t len;
	NumberU32_Unbounded("string length", len);

	RequireBytesInStream(len);
	str.resize(len);
	Get(name, (u8*)str.data(), len);

	Status err;
	out = wstring_from_utf8(str, &err);
	if (err)
		throw PSERROR_Deserialize_InvalidCharInString();

	if (!(minlength <= out.length() && out.length() <= maxlength))
		throw PSERROR_Deserialize_OutOfBounds(name);
}

void IDeserializer::RawBytes(const char* name, u8* data, size_t len)
{
	Get(name, data, len);
}

int IDeserializer::GetVersion() const
{
	debug_warn(L"GetVersion() not implemented in this subclass");
	return 0;
}
