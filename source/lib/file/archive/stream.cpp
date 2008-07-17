/**
 * =========================================================================
 * File        : stream.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "stream.h"

#include "lib/allocators/allocators.h"	// page_aligned_alloc
#include "lib/allocators/shared_ptr.h"
#include "codec.h"
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
	debug_assert(IsAllowableBuffer(buffer, size));

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
		m_mem.reset((u8*)page_aligned_alloc(size), PageAlignedDeleter(size));
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


LibError Stream::Feed(const u8* in, size_t inSize)
{
	if(m_outProduced == m_outputBufferManager.Size())	// output buffer full; must not call Process
		return INFO::OK;

	size_t inConsumed, outProduced;
	u8* const out = m_outputBufferManager.Buffer() + m_outProduced;
	const size_t outSize = m_outputBufferManager.Size() - m_outProduced;
	RETURN_ERR(m_codec->Process(in, inSize, out, outSize, inConsumed, outProduced));

	m_inConsumed += inConsumed;
	m_outProduced += outProduced;
	return INFO::CB_CONTINUE;
}


LibError Stream::Finish()
{
	size_t outProduced;
	RETURN_ERR(m_codec->Finish(m_checksum, outProduced));
	m_outProduced += outProduced;
	return INFO::OK;
}


LibError FeedStream(uintptr_t cbData, const u8* in, size_t inSize)
{
//	TIMER_ACCRUE(tc_stream);

	Stream& stream = *(Stream*)cbData;
	return stream.Feed(in, inSize);
}
