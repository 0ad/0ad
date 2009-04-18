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

/**
 * =========================================================================
 * File        : stream.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

#ifndef INCLUDED_STREAM
#define INCLUDED_STREAM

#include "codec.h"

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
	LibError Feed(const u8* in, size_t inSize);

	LibError Finish();

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

extern LibError FeedStream(uintptr_t cbData, const u8* in, size_t inSize);

#endif	// #ifndef INCLUDED_STREAM
