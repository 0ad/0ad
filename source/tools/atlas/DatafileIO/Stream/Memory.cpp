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

#include "Memory.h"

#include "../Util.h"

#include "zlib.h"

#include <algorithm>
#include <cassert>

using namespace DatafileIO;

SeekableInputStream_mem::SeekableInputStream_mem(char* data, size_t size, Releaser* releaser)
: m_Data(data), m_Size(size), m_Cursor(0), m_Releaser(releaser)
{
}

SeekableInputStream_mem::~SeekableInputStream_mem()
{
	m_Releaser->release(m_Data);
	delete m_Releaser;
}

off_t SeekableInputStream_mem::Tell() const
{
	return m_Cursor;
}

bool SeekableInputStream_mem::IsOk() const
{
	return true;
}

void SeekableInputStream_mem::Seek(off_t pos, Stream::whence mode)
{
	if (mode == FROM_START)
		m_Cursor = pos;
	else if (mode == FROM_CURRENT)
		m_Cursor += pos;
	else if (mode == FROM_END)
		m_Cursor = (off_t)m_Size - pos;
}

size_t SeekableInputStream_mem::Read(void* buffer, size_t size)
{
	if (m_Cursor >= (off_t)m_Size)
		return 0;

	size_t amount = size;
	if (m_Cursor + amount > m_Size)
		amount = m_Size - m_Cursor;

	std::copy(m_Data+m_Cursor, m_Data+m_Cursor+amount, (char*)buffer);
	m_Cursor += (off_t)amount;

	return amount;
}

bool SeekableInputStream_mem::AcquireBuffer(void*& buffer, size_t& size, size_t max_size)
{
	buffer = m_Data + m_Cursor;
	size = m_Size - m_Cursor;
	if (size > max_size) size = max_size;
	return true;
}
void SeekableInputStream_mem::ReleaseBuffer(void* /*buffer*/)
{
	/* do nothing */
}

//////////////////////////////////////////////////////////////////////////

Maybel33tInputStream::Maybel33tInputStream(SeekableInputStream* stream)
: m_Stream(NULL)
{
	char head[8];
	size_t bytes = stream->Read(head, 8);
	if (bytes == 8 && strncmp(head, "l33t", 4) == 0)
	{
		size_t uncompressedSize = *(uint32_t*) &head[4];

		void* buffer;
		size_t size;
		if (! stream->AcquireBuffer(buffer, size))
		{
			assert(!"Buffer acquisition unsuccessful");
		}
		else
		{
			char* uncompressedBuffer = new char[uncompressedSize];
			uLongf newUncomprSize = (uLongf)uncompressedSize;
			int err = uncompress((Bytef*)uncompressedBuffer, &newUncomprSize, (Bytef*)buffer, (uLong)size);
			if (err != Z_OK)
			{
				assert(!"Decompression failed");
				delete[] uncompressedBuffer;
			}
			else
			{
				assert(newUncomprSize == uncompressedSize);
				m_Stream = new SeekableInputStream_mem(uncompressedBuffer, uncompressedSize, new SeekableInputStream_mem::Releaser_NewArray);
			}
			stream->ReleaseBuffer(buffer);
		}
		delete stream;
	}
	else
	{
		stream->Seek(-(off_t)bytes, FROM_CURRENT);
		m_Stream = stream;
	}

}
Maybel33tInputStream::~Maybel33tInputStream()
{
	delete m_Stream;
}

bool Maybel33tInputStream::IsOk() const
{
	return (m_Stream != NULL);
}

off_t Maybel33tInputStream::Tell() const { return m_Stream->Tell(); }
void Maybel33tInputStream::Seek(off_t pos, Stream::whence mode) { m_Stream->Seek(pos, mode); }
size_t Maybel33tInputStream::Read(void* buffer, size_t size) { return m_Stream->Read(buffer, size); }
bool Maybel33tInputStream::AcquireBuffer(void*& buffer, size_t& size, size_t max_size) { return m_Stream->AcquireBuffer(buffer, size, max_size); }
void Maybel33tInputStream::ReleaseBuffer(void* buffer) { m_Stream->ReleaseBuffer(buffer); }
