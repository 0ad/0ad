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

#ifndef INCLUDED_WRITE_BUFFER
#define INCLUDED_WRITE_BUFFER

#include "lib/file/file.h"

class WriteBuffer
{
public:
	WriteBuffer();

	void Append(const void* data, size_t size);
	void Overwrite(const void* data, size_t size, size_t offset);

	shared_ptr<u8> Data() const
	{
		return m_data;
	}

	size_t Size() const
	{
		return m_size;
	}

private:
	size_t m_capacity;	// must come first (init order)

	shared_ptr<u8> m_data;
	size_t m_size;
};


class UnalignedWriter
{
	NONCOPYABLE(UnalignedWriter);
public:
	UnalignedWriter(const PIFile& file, off_t ofs);
	~UnalignedWriter();

	/**
	 * add data to the align buffer, writing it out to disk if full.
	 **/
	LibError Append(const u8* data, size_t size) const;

	/**
	 * zero-initialize any remaining space in the align buffer and write
	 * it to the file. this is called by the destructor.
	 **/
	void Flush() const;

private:
	LibError WriteBlock() const;

	PIFile m_file;
	shared_ptr<u8> m_alignedBuf;
	mutable off_t m_alignedOfs;
	mutable size_t m_bytesUsed;
};

typedef shared_ptr<UnalignedWriter> PUnalignedWriter;

#endif	// #ifndef INCLUDED_WRITE_BUFFER
