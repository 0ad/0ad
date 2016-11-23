/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef INCLUDED_WRITE_BUFFER
#define INCLUDED_WRITE_BUFFER

#include "lib/file/file.h"

class WriteBuffer
{
public:
	WriteBuffer();

	void Append(const void* data, size_t size);
	void Reserve(size_t size);
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
	void EnsureSufficientCapacity(size_t size);

	size_t m_capacity;	// must come first (init order)

	shared_ptr<u8> m_data;
	size_t m_size;
};


class UnalignedWriter
{
	NONCOPYABLE(UnalignedWriter);
public:
	UnalignedWriter(const PFile& file, off_t ofs);
	~UnalignedWriter();

	/**
	 * add data to the align buffer, writing it out to disk if full.
	 **/
	Status Append(const u8* data, size_t size) const;

	/**
	 * zero-initialize any remaining space in the align buffer and write
	 * it to the file. this is called by the destructor.
	 **/
	void Flush() const;

private:
	Status WriteBlock() const;

	PFile m_file;
	shared_ptr<u8> m_alignedBuf;
	mutable off_t m_alignedOfs;
	mutable size_t m_bytesUsed;
};

typedef shared_ptr<UnalignedWriter> PUnalignedWriter;

#endif	// #ifndef INCLUDED_WRITE_BUFFER
