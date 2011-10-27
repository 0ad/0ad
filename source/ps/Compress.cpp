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

#include "Compress.h"

#include "lib/byte_order.h"
#include "lib/external_libraries/zlib.h"

void CompressZLib(const std::string& data, std::string& out, bool includeLengthHeader)
{
	uLongf maxCompressedSize = compressBound(data.size());
	uLongf destLen = maxCompressedSize;

	out.clear();

	if (includeLengthHeader)
	{
		// Add a 4-byte uncompressed length header to the output
		out.resize(maxCompressedSize + 4);
		write_le32((void*)out.c_str(), data.size());
		int zok = compress((Bytef*)out.c_str() + 4, &destLen, (const Bytef*)data.c_str(), data.size());
		ENSURE(zok == Z_OK);
		out.resize(destLen + 4);
	}
	else
	{
		out.resize(maxCompressedSize);
		int zok = compress((Bytef*)out.c_str(), &destLen, (const Bytef*)data.c_str(), data.size());
		ENSURE(zok == Z_OK);
		out.resize(destLen);
	}
}

void DecompressZLib(const std::string& data, std::string& out, bool includeLengthHeader)
{
	ENSURE(includeLengthHeader); // otherwise we don't know how much to allocate

	out.clear();
	out.resize(read_le32(data.c_str()));

	uLongf destLen = out.size();
	int zok = uncompress((Bytef*)out.c_str(), &destLen, (const Bytef*)data.c_str() + 4, data.size() - 4);
	ENSURE(zok == Z_OK);
	ENSURE(destLen == out.size());

	// TODO: better error reporting might be nice
}