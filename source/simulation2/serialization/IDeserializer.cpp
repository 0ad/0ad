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

#include "precompiled.h"

#include "IDeserializer.h"

#include "lib/byte_order.h"
#include "lib/utf8.h"
#include "ps/CStr.h"

IDeserializer::~IDeserializer()
{
}

template<typename T>
T IDeserializer::Number_(T lower, T upper)
{
	T value;
	Get((u8*)&value, sizeof(T));

	if (!(lower <= value && value <= upper))
		throw PSERROR_Deserialize_OutOfBounds();

	return value;
}

void IDeserializer::NumberU8(uint8_t& out, uint8_t lower, uint8_t upper)
{
	out = Number_(lower, upper);
}

void IDeserializer::NumberI32(int32_t& out, int32_t lower, int32_t upper)
{
	out = (i32)to_le32((u32)Number_(lower, upper));
}

void IDeserializer::NumberU32(uint32_t& out, uint32_t lower, uint32_t upper)
{
	out = to_le32(Number_(lower, upper));
}

void IDeserializer::NumberU8_Unbounded(uint8_t& out)
{
	Get((u8*)&out, sizeof(uint8_t));
}

void IDeserializer::NumberI32_Unbounded(int32_t& out)
{
	int32_t value;
	Get((u8*)&value, sizeof(int32_t));
	out = (i32)to_le32((u32)value);
}

void IDeserializer::NumberU32_Unbounded(uint32_t& out)
{
	uint32_t value;
	Get((u8*)&value, sizeof(uint32_t));
	out = to_le32(value);
}

void IDeserializer::NumberFloat_Unbounded(float& out)
{
	Get((u8*)&out, sizeof(float));
}

void IDeserializer::NumberDouble_Unbounded(double& out)
{
	Get((u8*)&out, sizeof(double));
}

void IDeserializer::NumberFixed_Unbounded(CFixed_23_8& out)
{
	int32_t n;
	NumberI32_Unbounded(n);
	out.SetInternalValue(n);
}

void IDeserializer::Bool(bool& out)
{
	uint8_t i;
	NumberU8(i, 0, 1);
	out = (i != 0);
}

void IDeserializer::StringASCII(std::string& out, uint32_t minlength, uint32_t maxlength)
{
	uint32_t len;
	NumberU32(len, minlength, maxlength);
	out.resize(len); // TODO: should check len <= bytes remaining in stream
	Get((u8*)out.data(), len);

	for (size_t i = 0; i < out.length(); ++i)
		if (out[i] == 0 || (unsigned char)out[i] >= 128)
			throw PSERROR_Deserialize_InvalidCharInString();
}

void IDeserializer::String(std::wstring& out, uint32_t minlength, uint32_t maxlength)
{
	std::string str;
	uint32_t len;
	NumberU32_Unbounded(len);
	str.resize(len); // TODO: should check len <= bytes remaining in stream
	Get((u8*)str.data(), len);

	LibError err;
	out = wstring_from_utf8(str, &err);
	if (err)
		throw PSERROR_Deserialize_InvalidCharInString();

	if (!(minlength <= out.length() && out.length() <= maxlength))
		throw PSERROR_Deserialize_OutOfBounds();
}

void IDeserializer::StringUTF16(utf16string& out)
{
	std::string str;
	uint32_t len;
	NumberU32_Unbounded(len);
	str.resize(len); // TODO: should check len <= bytes remaining in stream
	Get((u8*)str.data(), len);

	// TODO: Maybe we should have a utf16string_from_utf8
	LibError err;
	std::wstring strw = wstring_from_utf8(str, &err);
	if (err != INFO::OK)
		throw PSERROR_Deserialize_InvalidCharInString();
	out = utf16string(strw.begin(), strw.end());
}

void IDeserializer::RawBytes(u8* data, size_t len)
{
	Get(data, len);
}

int IDeserializer::GetVersion() const
{
	debug_warn(L"GetVersion() not implemented in this subclass");
	return 0;
}

void IDeserializer::ReadString(std::string& out)
{
	uint32_t len;
	NumberU32_Unbounded(len);
	out.resize(len); // TODO: should check len <= bytes remaining in stream
	Get((u8*)out.data(), len);
}
