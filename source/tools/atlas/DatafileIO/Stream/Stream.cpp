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

#include "Stream.h"

#include <cassert>
#include <string>
#include <memory.h>

using namespace DatafileIO;

bool InputStream::AcquireBuffer(void*& buffer, size_t& size, size_t max_size)
{
	std::string data;
	const size_t tempBufSize = 65536;
	static char tempBuffer[tempBufSize];
	size = 0;
	size_t bytesLeft = max_size;
	size_t bytesRead;
	while (bytesLeft > 0 && (bytesRead = Read(tempBuffer, std::min(bytesLeft, tempBufSize))) != 0)
	{
		size += bytesRead;
		bytesLeft -= bytesRead;
		data += std::string(tempBuffer, bytesRead);
	}

	buffer = new char[size];
	memcpy(buffer, data.data(), size);

	return true;
}

void InputStream::ReleaseBuffer(void* buffer)
{
	delete[] (char*)buffer;
}
