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

/*
 * output buffer and 'stream' layered on top of a compression codec
 */

#ifndef INCLUDED_STREAM
#define INCLUDED_STREAM

#include "lib/file/archive/codec.h"

// note: this is similar in function to std::vector, but we don't need
// iterators etc. and would prefer to avoid initializing each byte.
class OutputBufferManager
{
public:
	OutputBufferManager();

	void Reset();
	void SetBuffer(u8* buffer, size_t size);

	/**
	 * allocate a new output buffer.
	 *
	 * @param size [bytes] to allocate.
	 *
	 * notes:
	 * - if a buffer had previously been allocated and is large enough,
	 *   it is reused (this reduces the number of allocations).
	 * - this class manages the lifetime of the buffer.
	 **/
	void AllocateBuffer(size_t size);

	u8* Buffer() const
	{
		return m_buffer;
	}

	size_t Size() const
	{
		return m_size;
	}

private:
	bool IsAllowableBuffer(u8* buffer, size_t size);

	u8* m_buffer;
	size_t m_size;

	shared_ptr<u8> m_mem;
	// size of m_mem. allows reusing previously allocated buffers
	// (user-specified buffers can't be reused because we have no control
	// over their lifetime)
	size_t m_capacity;
};


class Stream
{
public:
	Stream(const PICodec& codec);

	void SetOutputBuffer(u8* out, size_t outSize);

	void AllocateOutputBuffer(size_t outSizeMax);

	/**
	 * 'feed' the codec with a data block.
	 **/
	Status Feed(const u8* in, size_t inSize);

	Status Finish();

	size_t OutSize() const
	{
		return m_outProduced;
	}

	u32 Checksum() const
	{
		return m_checksum;
	}

private:
	PICodec m_codec;
	OutputBufferManager m_outputBufferManager;

	size_t m_inConsumed;
	size_t m_outProduced;
	u32 m_checksum;
};

// avoids the need for std::bind (not supported on all compilers) and boost::bind (can't be
// used at work)
struct StreamFeeder
{
	NONCOPYABLE(StreamFeeder);
public:
	StreamFeeder(Stream& stream)
		: stream(stream)
	{
	}

	Status operator()(const u8* data, size_t size) const
	{
		return stream.Feed(data, size);
	}

private:
	Stream& stream;
};

#endif	// #ifndef INCLUDED_STREAM
