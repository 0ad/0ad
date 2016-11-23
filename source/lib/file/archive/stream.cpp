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

#include "precompiled.h"
#include "lib/file/archive/stream.h"

#include "lib/allocators/page_aligned.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/file/archive/codec.h"
//#include "lib/timer.h"

//TIMER_ADD_CLIENT(tc_stream);


OutputBufferManager::OutputBufferManager()
{
	Reset();
}

void OutputBufferManager::Reset()
{
	m_buffer = 0;
	m_size = 0;
	m_capacity = 0;
}

void OutputBufferManager::SetBuffer(u8* buffer, size_t size)
{
	ENSURE(IsAllowableBuffer(buffer, size));

	m_buffer = buffer;
	m_size = size;
}

void OutputBufferManager::AllocateBuffer(size_t size)
{
	// notes:
	// - this implementation allows reusing previous buffers if they
	//   are big enough, which reduces the number of allocations.
	// - no further attempts to reduce allocations (e.g. by doubling
	//   the current size) are made; this strategy is enough.
	// - Pool etc. cannot be used because files may be huge (larger
	//   than the address space of 32-bit systems).

	// no buffer or the previous one wasn't big enough: reallocate
	if(!m_mem || m_capacity < size)
	{
		AllocateAligned(m_mem, size);
		m_capacity = size;
	}

	SetBuffer(m_mem.get(), size);
}

bool OutputBufferManager::IsAllowableBuffer(u8* buffer, size_t size)
{
	// none yet established
	if(m_buffer == 0 && m_size == 0)
		return true;

	// same as last time (happens with temp buffers)
	if(m_buffer == buffer && m_size == size)
		return true;

	// located after the last buffer (note: not necessarily after
	// the entire buffer; a lack of input can cause the output buffer
	// to only partially be used before the next call.)
	if((unsigned)(buffer - m_buffer) <= m_size)
		return true;

	return false;
}


//-----------------------------------------------------------------------------


Stream::Stream(const PICodec& codec)
	: m_codec(codec)
	, m_inConsumed(0), m_outProduced(0)
{
}


void Stream::AllocateOutputBuffer(size_t outSizeMax)
{
	m_outputBufferManager.AllocateBuffer(outSizeMax);
}


void Stream::SetOutputBuffer(u8* out, size_t outSize)
{
	m_outputBufferManager.SetBuffer(out, outSize);
}


Status Stream::Feed(const u8* in, size_t inSize)
{
	if(m_outProduced == m_outputBufferManager.Size())	// output buffer full; must not call Process
		return INFO::ALL_COMPLETE;

	size_t inConsumed, outProduced;
	u8* const out = m_outputBufferManager.Buffer() + m_outProduced;
	const size_t outSize = m_outputBufferManager.Size() - m_outProduced;
	RETURN_STATUS_IF_ERR(m_codec->Process(in, inSize, out, outSize, inConsumed, outProduced));

	m_inConsumed += inConsumed;
	m_outProduced += outProduced;
	return INFO::OK;
}


Status Stream::Finish()
{
	size_t outProduced;
	RETURN_STATUS_IF_ERR(m_codec->Finish(m_checksum, outProduced));
	m_outProduced += outProduced;
	return INFO::OK;
}
