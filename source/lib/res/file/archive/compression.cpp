/**
 * =========================================================================
 * File        : compression.cpp
 * Project     : 0 A.D.
 * Description : interface for compressing/decompressing data streams.
 *             : currently implements "deflate" (RFC1951).
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "compression.h"

#include <deque>

#include "lib/res/mem.h"
#include "lib/allocators.h"
#include "lib/timer.h"
#include "../file_io.h"	// IO_EOF

#include <boost/shared_ptr.hpp>

// rationale: this layer allows for other compression methods/libraries
// besides ZLib. it also simplifies the interface for user code and
// does error checking, etc.


ERROR_ASSOCIATE(ERR::COMPRESSION_UNKNOWN_METHOD, "Unknown/unsupported compression method", -1);


// provision for removing all ZLib code (all inflate calls will fail).
// used for checking DLL dependency; might also simulate corrupt Zip files.
//#define NO_ZLIB

#ifndef NO_ZLIB
# include "lib/external_libraries/zlib.h"
#else
// several switch statements are going to have all cases removed.
// squelch the corresponding warning.
# pragma warning(disable: 4065)
#endif


TIMER_ADD_CLIENT(tc_zip_inflate);
TIMER_ADD_CLIENT(tc_zip_memcpy);


//-----------------------------------------------------------------------------

class ICodec
{
public:
	/**
	 * note: the implementation should not check whether any data remains -
	 * codecs are sometimes destroyed without completing a transfer.
	 **/
	virtual ~ICodec()
	{
	}

	/**
	 * @return an upper bound on the output size for the given amount of input.
	 * this is used when allocating a single buffer for the whole operation.
	 **/
	virtual size_t MaxOutputSize(size_t inSize) const = 0;

	/**
	 * clear all previous state and prepare for reuse.
	 *
	 * this is as if the object were destroyed and re-created, but more
	 * efficient since memory buffers can be kept, etc.
	 **/
	virtual LibError Reset() = 0;

	/**
	 * process (i.e. compress or decompress) data.
	 *
	 * @param outSize bytes remaining in the output buffer; shall not be zero.
	 * @param inConsumed, outProduced how many bytes in the input and
	 * output buffers were used. either or both of these can be zero if
	 * the input size is small or there's not enough output space.
	 **/
	virtual LibError Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outProduced) = 0;

	/**
	 * flush buffers and make sure all output has been produced.
	 *
	 * @param out, outSize - the entire output buffer. this assumes the
	 * output pointers passed to Process were contiguous; if not, these
	 * values will not be meaningful.
	 * @param checksum over all input data.
	 * @return error status for the entire operation.
	 **/
	virtual LibError Finish(u8*& out, size_t& outSize, u32& checksum) = 0;

	/**
	 * update a checksum to reflect the contents of a buffer.
	 *
	 * @param checksum the initial value (must be 0 on first call)
	 * @return the new checksum.
	 **/
	virtual u32 UpdateChecksum(u32 checksum, const u8* in, size_t inSize) const = 0;
};


//-----------------------------------------------------------------------------

#ifndef NO_ZLIB

class ZLibCodec : public ICodec
{
protected:
	ZLibCodec()
	{
		memset(&m_zs, 0, sizeof(m_zs));
		InitializeChecksum();
	}

	void InitializeChecksum()
	{
		m_checksum = crc32(0, 0, 0);
	}

	typedef int ZEXPORT (*ZLibFunc)(z_streamp strm, int flush);

	static LibError LibError_from_zlib(int zlib_err, bool warn_if_failed = true)
	{
		LibError err = ERR::FAIL;
		switch(zlib_err)
		{
		case Z_OK:
			return INFO::OK;
		case Z_STREAM_END:
			err = ERR::IO_EOF; break;
		case Z_MEM_ERROR:
			err = ERR::NO_MEM; break;
		case Z_DATA_ERROR:
			err = ERR::CORRUPTED; break;
		case Z_STREAM_ERROR:
			err = ERR::INVALID_PARAM; break;
		default:
			err = ERR::FAIL; break;
		}

		if(warn_if_failed)
			DEBUG_WARN_ERR(err);
		return err;
	}

	static void WarnIfZLibError(int zlib_ret)
	{
		(void)LibError_from_zlib(zlib_ret, true);
	}

	LibError Process(ZLibFunc func, int flush, const u8* in, const size_t inSize, u8* out, const size_t outSize, size_t& inConsumed, size_t& outConsumed)
	{
		m_zs.next_in  = (Byte*)in;
		m_zs.avail_in = (uInt)inSize;
		m_zs.next_out  = (Byte*)out;
		m_zs.avail_out = (uInt)outSize;

		int ret = func(&m_zs, flush);
		// sanity check: if ZLib reports end of stream, all input data
		// must have been consumed.
		if(ret == Z_STREAM_END)
		{
			debug_assert(m_zs.avail_in == 0);
			ret = Z_OK;
		}

		debug_assert(inSize >= m_zs.avail_in && outSize >= m_zs.avail_out);
		inConsumed  = inSize  - m_zs.avail_in;
		outConsumed = outSize - m_zs.avail_out;

		return LibError_from_zlib(ret);
	}

	virtual u32 UpdateChecksum(u32 checksum, const u8* in, size_t inSize) const
	{
		return (u32)crc32(checksum, in, (uInt)inSize);
	}

	mutable z_stream m_zs;

	// note: z_stream does contain an 'adler' checksum field, but that's
	// not updated in streams lacking a gzip header, so we'll have to
	// calculate a checksum ourselves.
	// adler32 is somewhat weaker than CRC32, but a more important argument
	// is that we should use the latter for compatibility with Zip archives.
	mutable u32 m_checksum;
};

class ZLibCompressor : public ZLibCodec
{
public:
	ZLibCompressor()
	{
		// note: with Z_BEST_COMPRESSION, 78% percent of
		// archive builder CPU time is spent in ZLib, even though
		// that is interleaved with IO; everything else is negligible.
		// we therefore enable this only in final builds; during
		// development, 1.5% bigger archives are definitely worth much
		// faster build time.
#if CONFIG_FINAL
		const int level      = Z_BEST_COMPRESSION;
#else
		const int level      = Z_BEST_SPEED;
#endif
		const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
		const int memLevel   = 9;					// max speed; total mem ~= 384KiB
		const int strategy   = Z_DEFAULT_STRATEGY;	// normal data - not RLE
		const int ret = deflateInit2(&m_zs, level, Z_DEFLATED, windowBits, memLevel, strategy);
		debug_assert(ret == Z_OK);
	}

	virtual ~ZLibCompressor()
	{
		const int ret = deflateEnd(&m_zs);
		WarnIfZLibError(ret);
	}

	virtual size_t MaxOutputSize(size_t inSize) const
	{
		return (size_t)deflateBound(&m_zs, (uLong)inSize);
	}

	virtual LibError Reset()
	{
		ZLibCodec::InitializeChecksum();
		const int ret = deflateReset(&m_zs);
		return LibError_from_zlib(ret);
	}

	virtual LibError Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outConsumed)
	{
		m_checksum = UpdateChecksum(m_checksum, in, inSize);
		return ZLibCodec::Process(deflate, 0, in, inSize, out, outSize, inConsumed, outConsumed);
	}

	virtual LibError Finish(u8*& out, size_t& outSize, u32& checksum)
	{
		// notify zlib that no more data is forthcoming and have it flush output.
		// our output buffer has enough space due to use of deflateBound;
		// therefore, deflate must return Z_STREAM_END.
		const int ret = deflate(&m_zs, Z_FINISH);
		if(ret != Z_STREAM_END)
			debug_warn("deflate: unexpected Z_FINISH behavior");

		out = m_zs.next_out - m_zs.total_out;
		outSize = m_zs.total_out;
		checksum = m_checksum;
		return INFO::OK;
	}
};


class ZLibDecompressor : public ZLibCodec
{
public:
	ZLibDecompressor()
	{
		const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
		const int ret = inflateInit2(&m_zs, windowBits);
		debug_assert(ret == Z_OK);
	}

	virtual ~ZLibDecompressor()
	{
		const int ret = inflateEnd(&m_zs);
		WarnIfZLibError(ret);
	}

	virtual size_t MaxOutputSize(size_t inSize) const
	{
		// relying on an upper bound for the output is a really bad idea for
		// large files. archive formats store the uncompressed file sizes,
		// so callers should use that when allocating the output buffer.
		debug_assert(inSize < 1*MiB);

		// http://www.zlib.org/zlib_tech.html
		return inSize*1032;
	}

	virtual LibError Reset()
	{
		ZLibCodec::InitializeChecksum();
		const int ret = inflateReset(&m_zs);
		return LibError_from_zlib(ret);
	}

	virtual LibError Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outConsumed)
	{
		const LibError ret = ZLibCodec::Process(inflate, Z_SYNC_FLUSH, in, inSize, out, outSize, inConsumed, outConsumed);
		m_checksum = UpdateChecksum(m_checksum, in, inSize);
		return ret;
	}

	virtual LibError Finish(u8*& out, size_t& outSize, u32& checksum)
	{
		// no action needed - decompression always flushes immediately.

		out = m_zs.next_out - m_zs.total_out;
		outSize = m_zs.total_out;
		checksum = m_checksum;
		return INFO::OK;
	}
};

#endif	// #ifndef NO_ZLIB


//-----------------------------------------------------------------------------

#include "lib/nommgr.h"	// protect placement new

class CodecFactory
{
public:
	ICodec* Create(ContextType type, CompressionMethod method)
	{
		debug_assert(type == CT_COMPRESSION || type == CT_DECOMPRESSION);

		switch(method)
		{
#ifndef NO_ZLIB
		case CM_DEFLATE:
			if(type == CT_COMPRESSION)
			{
				cassert(sizeof(ZLibCompressor) <= MAX_CODEC_SIZE);
				return new(AllocateMemory()) ZLibCompressor;
			}
			else
			{
				cassert(sizeof(ZLibDecompressor) <= MAX_CODEC_SIZE);
				return new(AllocateMemory()) ZLibDecompressor;
			}
			break;
#endif
		default:
			WARN_ERR(ERR::COMPRESSION_UNKNOWN_METHOD);
			return 0;
		}
	}

	void Destroy(ICodec* codec)
	{
		codec->~ICodec();
		m_allocator.Deallocate((Allocator::value_type*)codec);
	}

private:
	void* AllocateMemory()
	{
		void* mem = m_allocator.Allocate();
		if(!mem)
			throw std::bad_alloc();
		return mem;
	}

	// double: see explanation in SingleAllocator
	static const size_t MAX_CODEC_SIZE = 100;
	typedef SingleAllocator<double[(MAX_CODEC_SIZE+sizeof(double)-1)/sizeof(double)]> Allocator;
	Allocator m_allocator;
};

#include "lib/mmgr.h"


//-----------------------------------------------------------------------------
// BufferManager

class BufferManager
{
public:
	void Enqueue(const u8* data, size_t size)
	{
		// note: calling with inSize = 0 is allowed and just means
		// we don't enqueue a new buffer. it happens when compressing
		// newly decompressed data if nothing was output (due to a
		// small compressed input buffer).
		if(size != 0)
			m_pendingBuffers.push_back(Buffer(data, size));
	}

	bool GetNext(const u8*& data, size_t& size) const
	{
		if(m_pendingBuffers.empty())
			return false;
		const Buffer& buffer = m_pendingBuffers.front();
		data = buffer.RemainingData();
		size = buffer.RemainingSize();
		return true;
	}

	void MarkAsProcessed(size_t numBytes)
	{
		Buffer& buffer = m_pendingBuffers.front();
		buffer.MarkAsProcessed(numBytes);
		if(buffer.RemainingSize() == 0)
			m_pendingBuffers.pop_front();
	}

	void Reset()
	{
		m_pendingBuffers.clear();
	}

private:
	class Buffer
	{
	public:
		Buffer(const u8* data, size_t size)
			: m_data(data), m_size(size), m_pos(0)
		{
		}

		const u8* RemainingData() const
		{
			return m_data + m_pos;
		}

		size_t RemainingSize() const
		{
			return m_size - m_pos;
		}

		void MarkAsProcessed(size_t numBytes)
		{
			m_pos += numBytes;
			debug_assert(m_pos <= m_size);

			// everything has been consumed. (this buffer will now be
			// destroyed by removing it from the deque)
			if(m_pos == m_size)
				return;

			// if there is any data left, the caller must have "choked"
			// (i.e. filled their output buffer).

			// this buffer currently references data allocated by the caller.
			if(!m_copy.get())
			{
				// since we have to return and they could free it behind our
				// back, we'll need to allocate a copy of the remaining data.
				m_size = RemainingSize();
				m_copy.reset(new u8[m_size]);
				cpu_memcpy(m_copy.get(), RemainingData(), m_size);
				m_data = m_copy.get();	// must happen after cpu_memcpy
				m_pos = 0;
			}
		}

	private:
		const u8* m_data;
		size_t m_size;
		size_t m_pos;
		boost::shared_ptr<u8> m_copy;
	};

	// note: a 'list' (deque is more efficient) is necessary.
	// lack of output space can result in leftover input data;
	// since we do not want Feed() to always have to check for and
	// use up any previous remnants, we allow queuing them.
	std::deque<Buffer> m_pendingBuffers;
};

//-----------------------------------------------------------------------------

class Stream
{
public:
	Stream(ContextType type, CompressionMethod method)
		: m_out(0), m_outSize(0), m_outPos(0)
		, m_codec(m_codecFactory.Create(type, method))
	{
	}

	~Stream()
	{
		m_codecFactory.Destroy(m_codec);
	}

	size_t MaxOutputSize(size_t inSize) const
	{
		return m_codec->MaxOutputSize(inSize);
	}

	void Reset()
	{
		m_bufferManager.Reset();

		m_out = 0;
		m_outSize = 0;
		m_outPos = 0;

		m_codec->Reset();
	}

	void SetOutput(u8* out, size_t outSize)
	{
		debug_assert(IsAllowableOutputBuffer(out, outSize));

		m_out = out;
		m_outSize = outSize;
		m_outPos = 0;
	}

	LibError AllocOutput(size_t size)
	{
		// notes:
		// - this implementation allows reusing previous buffers if they
		//   are big enough, which reduces the number of allocations.
		// - no further attempts to reduce allocations (e.g. by doubling
		//   the current size) are made; this strategy is enough.
		// - Pool etc. cannot be used because files may be huge (larger
		//   than the address space of 32-bit systems).

		// no buffer or the previous one wasn't big enough: reallocate
		if(!m_outMem.get() || m_outMemSize < size)
		{
			m_outMem.reset((u8*)page_aligned_alloc(size), PageAlignedDeleter(size));
			m_outMemSize = size;
		}

		SetOutput(m_outMem.get(), size);

		return INFO::OK;
	}

	ssize_t Feed(const u8* in, size_t inSize)
	{
		size_t outTotal = 0;	// returned unless error occurs

		m_bufferManager.Enqueue(in, inSize);

		// work off any pending buffers and the new one
		const u8* cdata; size_t csize;
		while(m_bufferManager.GetNext(cdata, csize))
		{
			if(m_outSize == m_outPos)	// output buffer full; must not call Process
				break;

			size_t inConsumed, outProduced;
			LibError err = m_codec->Process(cdata, csize, m_out+m_outPos, m_outSize-m_outPos, inConsumed, outProduced);
			if(err < 0)
				return err;

			m_bufferManager.MarkAsProcessed(inConsumed);
			outTotal += outProduced;
			m_outPos += outProduced;
		}

		return (ssize_t)outTotal;
	}

	LibError Finish(u8*& out, size_t& outSize, u32& checksum)
	{
		return m_codec->Finish(out, outSize, checksum);
	}

	u32 UpdateChecksum(u32 checksum, const u8* in, size_t inSize) const
	{
		return m_codec->UpdateChecksum(checksum, in, inSize);
	}

private:
	// ICodec::Finish is allowed to assume that output buffers were identical
	// or contiguous; we verify this here.
	bool IsAllowableOutputBuffer(u8* out, size_t outSize)
	{
		// none yet established
		if(m_out == 0 && m_outSize == 0 && m_outPos == 0)
			return true;

		// same as last time (happens with temp buffers)
		if(m_out == out && m_outSize == outSize)
			return true;

		// located after the last buffer (note: not necessarily after
		// the entire buffer; a lack of input can cause the output buffer
		// to only partially be used before the next call.)
		if((unsigned)(out - m_out) <= m_outSize)
			return true;

		return false;
	}

	BufferManager m_bufferManager;

	u8* m_out;
	size_t m_outSize;
	size_t m_outPos;

	boost::shared_ptr<u8> m_outMem;
	size_t m_outMemSize;

	static CodecFactory m_codecFactory;
	ICodec* m_codec;
};

/*static*/ CodecFactory Stream::m_codecFactory;


//-----------------------------------------------------------------------------

#include "lib/nommgr.h"	// protect placement new

class StreamFactory
{
public:
	Stream* Create(ContextType type, CompressionMethod method)
	{
		void* mem = m_allocator.Allocate();
		if(!mem)
			throw std::bad_alloc();
		return new(mem) Stream(type, method);
	}

	void Destroy(Stream* stream)
	{
		stream->~Stream();
		m_allocator.Deallocate(stream);
	}

private:
	SingleAllocator<Stream> m_allocator;
};

#include "lib/mmgr.h"


//-----------------------------------------------------------------------------

static StreamFactory streamFactory;

uintptr_t comp_alloc(ContextType type, CompressionMethod method)
{
	Stream* stream = streamFactory.Create(type, method);
	return (uintptr_t)stream;
}

void comp_free(uintptr_t ctx)
{
	// no-op if context is 0 (i.e. was never allocated)
	if(!ctx)
		return;

	Stream* stream = (Stream*)ctx;
	streamFactory.Destroy(stream);
}

void comp_reset(uintptr_t ctx)
{
	Stream* stream = (Stream*)ctx;
	stream->Reset();
}

size_t comp_max_output_size(uintptr_t ctx, size_t inSize)
{
	Stream* stream = (Stream*)ctx;
	return stream->MaxOutputSize(inSize);
}

void comp_set_output(uintptr_t ctx, u8* out, size_t outSize)
{
	Stream* stream = (Stream*)ctx;
	stream->SetOutput(out, outSize);
}

LibError comp_alloc_output(uintptr_t ctx, size_t inSize)
{
	Stream* stream = (Stream*)ctx;
	return stream->AllocOutput(inSize);
}

ssize_t comp_feed(uintptr_t ctx, const u8* in, size_t inSize)
{
	Stream* stream = (Stream*)ctx;
	return stream->Feed(in, inSize);
}

LibError comp_finish(uintptr_t ctx, u8** out, size_t* outSize, u32* checksum)
{
	Stream* stream = (Stream*)ctx;
	return stream->Finish(*out, *outSize, *checksum);
}

u32 comp_update_checksum(uintptr_t ctx, u32 checksum, const u8* in, size_t inSize)
{
	Stream* stream = (Stream*)ctx;
	return stream->UpdateChecksum(checksum, in, inSize);
}
