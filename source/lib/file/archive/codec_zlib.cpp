/* Copyright (C) 2010 Wildfire Games.
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
#include "lib/file/archive/codec_zlib.h"

#include "lib/alignment.h"
#include "lib/file/archive/codec.h"
#include "lib/external_libraries/zlib.h"

#include "lib/sysdep/cpu.h"


class Codec_ZLib : public ICodec
{
public:
	u32 UpdateChecksum(u32 checksum, const u8* in, size_t inSize) const
	{
#if CODEC_COMPUTE_CHECKSUM
		return (u32)crc32(checksum, in, (uInt)inSize);
#else
		UNUSED2(checksum);
		UNUSED2(in);
		UNUSED2(inSize);
		return 0;
#endif
	}

protected:
	u32 InitializeChecksum()
	{
#if CODEC_COMPUTE_CHECKSUM
		return crc32(0, 0, 0);
#else
		return 0;
#endif
	}
};


//-----------------------------------------------------------------------------

class Codec_ZLibNone : public Codec_ZLib
{
public:
	Codec_ZLibNone()
	{
		Reset();
	}

	virtual ~Codec_ZLibNone()
	{
	}

	virtual size_t MaxOutputSize(size_t inSize) const
	{
		return inSize;
	}

	virtual Status Reset()
	{
		m_checksum = InitializeChecksum();
		return INFO::OK;
	}

	virtual Status Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outProduced)
	{
		const size_t transferSize = std::min(inSize, outSize);
		memcpy(out, in, transferSize);
		inConsumed = outProduced = transferSize;
		m_checksum = UpdateChecksum(m_checksum, out, outProduced);
		return INFO::OK;
	}

	virtual Status Finish(u32& checksum, size_t& outProduced)
	{
		outProduced = 0;
		checksum = m_checksum;
		return INFO::OK;
	}

private:
	u32 m_checksum;
};


//-----------------------------------------------------------------------------

class CodecZLibStream : public Codec_ZLib
{
protected:
	CodecZLibStream()
	{
		memset(&m_zs, 0, sizeof(m_zs));
		m_checksum = InitializeChecksum();
	}

	static Status LibError_from_zlib(int zlib_ret)
	{
		switch(zlib_ret)
		{
		case Z_OK:
			return INFO::OK;
		case Z_STREAM_END:
			WARN_RETURN(ERR::FAIL);
		case Z_MEM_ERROR:
			WARN_RETURN(ERR::NO_MEM);
		case Z_DATA_ERROR:
			WARN_RETURN(ERR::CORRUPTED);
		case Z_STREAM_ERROR:
			WARN_RETURN(ERR::INVALID_PARAM);
		default:
			WARN_RETURN(ERR::FAIL);
		}
	}

	static void WarnIfZLibError(int zlib_ret)
	{
		(void)LibError_from_zlib(zlib_ret);
	}

	typedef int ZEXPORT (*ZLibFunc)(z_streamp strm, int flush);

	Status CallStreamFunc(ZLibFunc func, int flush, const u8* in, const size_t inSize, u8* out, const size_t outSize, size_t& inConsumed, size_t& outProduced)
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
			ENSURE(m_zs.avail_in == 0);
			ret = Z_OK;
		}

		ENSURE(inSize >= m_zs.avail_in && outSize >= m_zs.avail_out);
		inConsumed  = inSize  - m_zs.avail_in;
		outProduced = outSize - m_zs.avail_out;

		return LibError_from_zlib(ret);
	}

	mutable z_stream m_zs;

	// note: z_stream does contain an 'adler' checksum field, but that's
	// not updated in streams lacking a gzip header, so we'll have to
	// calculate a checksum ourselves.
	// adler32 is somewhat weaker than CRC32, but a more important argument
	// is that we should use the latter for compatibility with Zip archives.
	mutable u32 m_checksum;
};


//-----------------------------------------------------------------------------

class Compressor_ZLib : public CodecZLibStream
{
public:
	Compressor_ZLib()
	{
		// note: with Z_BEST_COMPRESSION, 78% percent of
		// archive builder CPU time is spent in ZLib, even though
		// that is interleaved with IO; everything else is negligible.
		// we prefer faster speed at the cost of 1.5% larger archives.
		const int level      = Z_BEST_SPEED;
		const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
		const int memLevel   = 9;					// max speed; total mem ~= 384KiB
		const int strategy   = Z_DEFAULT_STRATEGY;	// normal data - not RLE
		const int ret = deflateInit2(&m_zs, level, Z_DEFLATED, windowBits, memLevel, strategy);
		ENSURE(ret == Z_OK);
	}

	virtual ~Compressor_ZLib()
	{
		const int ret = deflateEnd(&m_zs);
		WarnIfZLibError(ret);
	}

	virtual size_t MaxOutputSize(size_t inSize) const
	{
		return (size_t)deflateBound(&m_zs, (uLong)inSize);
	}

	virtual Status Reset()
	{
		m_checksum = InitializeChecksum();
		const int ret = deflateReset(&m_zs);
		return LibError_from_zlib(ret);
	}

	virtual Status Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outProduced)
	{
		m_checksum = UpdateChecksum(m_checksum, in, inSize);
		return CodecZLibStream::CallStreamFunc(deflate, 0, in, inSize, out, outSize, inConsumed, outProduced);
	}

	virtual Status Finish(u32& checksum, size_t& outProduced)
	{
		const uInt availOut = m_zs.avail_out;

		// notify zlib that no more data is forthcoming and have it flush output.
		// our output buffer has enough space due to use of deflateBound;
		// therefore, deflate must return Z_STREAM_END.
		const int ret = deflate(&m_zs, Z_FINISH);
		ENSURE(ret == Z_STREAM_END);

		outProduced = size_t(availOut - m_zs.avail_out);

		checksum = m_checksum;
		return INFO::OK;
	}
};


//-----------------------------------------------------------------------------

class Decompressor_ZLib : public CodecZLibStream
{
public:
	Decompressor_ZLib()
	{
		const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
		const int ret = inflateInit2(&m_zs, windowBits);
		ENSURE(ret == Z_OK);
	}

	virtual ~Decompressor_ZLib()
	{
		const int ret = inflateEnd(&m_zs);
		WarnIfZLibError(ret);
	}

	virtual size_t MaxOutputSize(size_t inSize) const
	{
		// relying on an upper bound for the output is a really bad idea for
		// large files. archive formats store the uncompressed file sizes,
		// so callers should use that when allocating the output buffer.
		ENSURE(inSize < 1*MiB);

		return inSize*1032;	// see http://www.zlib.org/zlib_tech.html
	}

	virtual Status Reset()
	{
		m_checksum = InitializeChecksum();
		const int ret = inflateReset(&m_zs);
		return LibError_from_zlib(ret);
	}

	virtual Status Process(const u8* in, size_t inSize, u8* out, size_t outSize, size_t& inConsumed, size_t& outProduced)
	{
		const Status ret = CodecZLibStream::CallStreamFunc(inflate, Z_SYNC_FLUSH, in, inSize, out, outSize, inConsumed, outProduced);
		m_checksum = UpdateChecksum(m_checksum, out, outProduced);
		return ret;
	}

	virtual Status Finish(u32& checksum, size_t& outProduced)
	{
		// no action needed - decompression always flushes immediately.
		outProduced = 0;

		checksum = m_checksum;
		return INFO::OK;
	}
};


//-----------------------------------------------------------------------------

PICodec CreateCodec_ZLibNone()
{
	return PICodec(new Codec_ZLibNone);
}

PICodec CreateCompressor_ZLibDeflate()
{
	return PICodec(new Compressor_ZLib);
}

PICodec CreateDecompressor_ZLibDeflate()
{
	return PICodec (new Decompressor_ZLib);
}
