/* Copyright (C) 2009 Wildfire Games.
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

#include "Util.h"
#include "Stream/Stream.h"

using namespace DatafileIO;

utf16string DatafileIO::ReadUString(InputStream& stream)
{
	uint32_t length;
	stream.Read(&length, 4);

	utf16string ret;
	ret.resize(length);
	stream.Read(&ret[0], length*2);

	return ret;
}

void DatafileIO::WriteUString(OutputStream& stream, const utf16string& string)
{
	uint32_t length = (uint32_t)string.length();
	stream.Write(&length, 4);
	stream.Write((utf16_t*)&string[0], length*2);
}

#ifndef _WIN32
// TODO In reality, these two should be able to de/encode UTF-16 to/from UCS-4
// instead of just treating UTF-16 as UCS-2

std::wstring DatafileIO::utf16tow(const utf16string &str)
{
	return std::wstring(str.begin(), str.end());
}

utf16string DatafileIO::wtoutf16(const std::wstring &str)
{
	return utf16string(str.begin(), str.end());
}
#endif
