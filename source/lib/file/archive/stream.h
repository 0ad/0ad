/**
 * =========================================================================
 * File        : stream.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_STREAM
#define INCLUDED_STREAM

struct ICodec;

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

	boost::shared_array<u8> m_mem;
	// size of m_mem. allows reusing previously allocated buffers
	// (user-specified buffers can't be reused because we have no control
	// over their lifetime)
	size_t m_capacity;
};


class Stream
{
public:
	Stream(boost::shared_ptr<ICodec> codec);

	void SetOutputBuffer(u8* out, size_t outSize);

	void AllocateOutputBuffer(size_t outSizeMax);

	/**
	 * 'feed' the codec with a data block.
	 *
	 * @param bytesProcessed receives the number of output bytes produced.
	 * it can legitimately be 0 - this happens if the input buffer is small
	 * and the codec hasn't produced any output.
	 **/
	LibError Feed(const u8* in, size_t inSize, size_t& bytesProcessed);

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
	boost::shared_ptr<ICodec> m_codec;
	OutputBufferManager m_outputBufferManager;

	size_t m_inConsumed;
	size_t m_outProduced;
	u32 m_checksum;
};

extern LibError FeedStream(uintptr_t cbData, const u8* in, size_t inSize, size_t& bytesProcessed);

#endif	// #ifndef INCLUDED_STREAM
