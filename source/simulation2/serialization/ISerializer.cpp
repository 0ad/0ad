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

#include "ISerializer.h"

#include "lib/utf8.h"

ISerializer::~ISerializer()
{
}

void ISerializer::NumberU8(const char* name, uint8_t value, uint8_t lower, uint8_t upper)
{
	if (!(lower <= value && value <= upper))
		throw PSERROR_Serialize_OutOfBounds();
	PutNumber(name, value);
}

void ISerializer::NumberI8(const char* name, int8_t value, int8_t lower, int8_t upper)
{
	if (!(lower <= value && value <= upper))
		throw PSERROR_Serialize_OutOfBounds();
	PutNumber(name, value);
}

void ISerializer::NumberU16(const char* name, uint16_t value, uint16_t lower, uint16_t upper)
{
	if (!(lower <= value && value <= upper))
		throw PSERROR_Serialize_OutOfBounds();
	PutNumber(name, value);
}

void ISerializer::NumberI16(const char* name, int16_t value, int16_t lower, int16_t upper)
{
	if (!(lower <= value && value <= upper))
		throw PSERROR_Serialize_OutOfBounds();
	PutNumber(name, value);
}

void ISerializer::NumberU32(const char* name, uint32_t value, uint32_t lower, uint32_t upper)
{
	if (!(lower <= value && value <= upper))
		throw PSERROR_Serialize_OutOfBounds();
	PutNumber(name, value);
}

void ISerializer::NumberI32(const char* name, int32_t value, int32_t lower, int32_t upper)
{
	if (!(lower <= value && value <= upper))
		throw PSERROR_Serialize_OutOfBounds();
	PutNumber(name, value);
}

void ISerializer::StringASCII(const char* name, const std::string& value, uint32_t minlength, uint32_t maxlength)
{
	if (!(minlength <= value.length() && value.length() <= maxlength))
		throw PSERROR_Serialize_OutOfBounds();

	for (size_t i = 0; i < value.length(); ++i)
		if (value[i] == 0 || (unsigned char)value[i] >= 128)
			throw PSERROR_Serialize_InvalidCharInString();

	PutString(name, value);
}

void ISerializer::String(const char* name, const std::wstring& value, uint32_t minlength, uint32_t maxlength)
{
	if (!(minlength <= value.length() && value.length() <= maxlength))
		throw PSERROR_Serialize_OutOfBounds();

	Status err;
	std::string str = utf8_from_wstring(value, &err);
	if (err)
		throw PSERROR_Serialize_InvalidCharInString();

	PutString(name, str);
}

void ISerializer::ScriptVal(const char* name, JS::HandleValue value)
{
	PutScriptVal(name, value);
}

void ISerializer::RawBytes(const char* name, const u8* data, size_t len)
{
	PutRaw(name, data, len);
}

bool ISerializer::IsDebug() const
{
	return false;
}
