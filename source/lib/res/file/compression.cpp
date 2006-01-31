// Compression/Decompression interface
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include <deque>

#include "lib/res/mem.h"
#include "lib/allocators.h"
#include "lib/timer.h"
#include "compression.h"

// provision for removing all ZLib code (all inflate calls will fail).
// used for checking DLL dependency; might also simulate corrupt Zip files.
//#define NO_ZLIB

#ifndef NO_ZLIB
# define ZLIB_DLL
# include <zlib.h>

# if MSC_VERSION
#  ifdef NDEBUG
#   pragma comment(lib, "zlib1.lib")
#  else
#   pragma comment(lib, "zlib1d.lib")
#  endif
# endif
#endif

TIMER_ADD_CLIENT(tc_zip_inflate);
TIMER_ADD_CLIENT(tc_zip_memcpy);


/*
what is value added of zlib layer?
it gets the zlib interface cororrect - e.g. checking for return vals.
code can then just use our more simple interfacde

inf_*: in-memory inflate routines (zlib wrapper)
decompresses blocks from file_io callback.
*/

static LibError LibError_from_zlib(int err)
{
	switch(err)
	{
	case Z_OK:
		return ERR_OK;
	case Z_STREAM_END:
		return ERR_EOF;
	case Z_MEM_ERROR:
		return ERR_NO_MEM;
	case Z_DATA_ERROR:
		return ERR_CORRUPTED;
	case Z_STREAM_ERROR:
		return ERR_INVALID_PARAM;
	default:
		return ERR_FAIL;
	}
	UNREACHABLE;
}



// must be dynamically allocated - need one for every open AFile,
// and z_stream is large.
class Compressor
{
public:
	Compressor(ContextType type_)
	{
		type = type_;
	}

	virtual ~Compressor()
	{
		mem_free(out_mem);

		// free all remaining input buffers that we copied (rare)
		for(size_t i = 0; i < pending_bufs.size(); i++)
			free(pending_bufs[i].mem_to_free);
	}


	virtual LibError init() = 0;

	virtual LibError reset() = 0;

	virtual LibError alloc_output(size_t in_size) = 0;

	// consume as much of the given input buffer as possible. the data is
	// decompressed/compressed into the previously established output buffer.
	// reports how many bytes were consumed and produced; either or both
	// can be 0 if input size is small or not enough room in output buffer.
	// caller is responsible for saving any leftover input data,
	// which is why we pass back in_consumed.
	virtual LibError consume(const void* in, size_t in_size, size_t& in_consumed, size_t& out_produced) = 0;

	virtual LibError finish(void** out, size_t* out_size) = 0;

	virtual void release() = 0;


	void set_output(void* out, size_t out_size)
	{
		next_out = out;
		avail_out = out_size;
	}
	void* get_output()
	{
		return next_out;
	}
	ssize_t feed(const void* in, size_t in_size)
	{
		size_t out_total = 0;	// returned unless error occurs
		LibError err;

		// note: calling with in_size = 0 is allowed and just means
		// we don't queue a new buffer. this can happen when compressing
		// newly decompressed data if nothing was output (due to
		// small compressed input buffer).
		if(in_size != 0)
			pending_bufs.push_back(Buf(in, in_size, 0));

		// work off all queued input buffers until output buffer is filled.
		while(!pending_bufs.empty())
		{
			Buf& buf = pending_bufs.front();

			size_t in_consumed, out_consumed;
			err = consume(buf.cdata, buf.csize, in_consumed, out_consumed);
			if(err < 0)
				return err;

			out_total += out_consumed;
			debug_assert(in_consumed <= buf.csize);
			// all input consumed - dequeue input buffer
			if(in_consumed == buf.csize)
			{
				free(buf.mem_to_free);	// no-op unless we allocated it
				pending_bufs.pop_front();
			}
			// limited by output space - mark input buffer as partially used
			else
			{
				buf.cdata += in_consumed;
				buf.csize -= in_consumed;

				// buffer was allocated by caller and may be freed behind our
				// backs after returning (which we must because output buffer
				// is full). allocate a copy of the remaining input data.
				if(!buf.mem_to_free)
				{
					void* cdata_copy = malloc(buf.csize);
					if(!cdata_copy)
						return ERR_NO_MEM;
					memcpy2(cdata_copy, buf.cdata, buf.csize);
					buf.cdata = (const u8*)cdata_copy;
				}

				return (ssize_t)out_total;
			}
		}

		return (ssize_t)out_total;
	}


protected:
	ContextType type;
	CompressionMethod method;

	void* next_out;
	size_t avail_out;

	void* out_mem;
	size_t out_mem_size;

// may be several IOs in flight => list needed
	struct Buf
	{
		const u8* cdata;
		size_t csize;
		void* mem_to_free;
		Buf(const void* cdata_, size_t csize_, void* mem_to_free_)
		{
			cdata = (const u8*)cdata_;
			csize = csize_;
			mem_to_free = mem_to_free_;
		}
	};
	std::deque<Buf> pending_bufs;

	LibError alloc_output_impl(size_t required_out_size)
	{
		size_t alloc_size = required_out_size;

		// .. already had a buffer
		if(out_mem)
		{
			// it was big enough - reuse
			if(out_mem_size >= required_out_size)
				goto have_out_mem;

			// free previous
			// note: mem.cpp doesn't support realloc; don't use Pool etc. because
			// maximum file size may be huge (more address space than we can afford)
			mem_free(out_mem);

			// TODO: make sure difference in required_out_size vs. out_mem_size
			// is big enough - i.e. don't only increment in small chunks.
			// set alloc_size...

			// fall through..
		}

		// .. need to allocate anew
		out_mem = mem_alloc(alloc_size, 32*KiB);
		if(!out_mem)
			WARN_RETURN(ERR_NO_MEM);
		out_mem_size = alloc_size;

have_out_mem:
		next_out  = out_mem;
		avail_out = out_mem_size;

		return ERR_OK;
	}

};	// class Compressor


#ifndef NO_ZLIB

class ZLibCompressor : public Compressor
{
	z_stream zs;

public:
	// default ctor cannot be generated
	ZLibCompressor(ContextType type)
		: Compressor(type), zs() {}

	virtual LibError init()
	{
		memset(&zs, 0, sizeof(zs));

		int ret;
		if(type == CT_COMPRESSION)
		{
			const int level      = Z_BEST_COMPRESSION;
			const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
			const int memLevel   = 8;					// default; total mem ~= 256KiB
			const int strategy   = Z_DEFAULT_STRATEGY;	// normal data - not RLE
			ret = deflateInit2(&zs, level, Z_DEFLATED, windowBits, memLevel, strategy);
		}
		else
		{
			const int windowBits = -MAX_WBITS;	// max window size; omit ZLib header
			ret = inflateInit2(&zs, windowBits);
		}
		CHECK_ERR(LibError_from_zlib(ret));
		return ERR_OK;
	}

	virtual LibError reset()
	{
		int ret;
		if(type == CT_COMPRESSION)
			ret = deflateReset(&zs);
		else
			ret = inflateReset(&zs);
		CHECK_ERR(LibError_from_zlib(ret));
		return ERR_OK;
	}


	// out:
	// compression ratios can be enormous (1000x), so we require
	// callers to allocate the output buffer themselves
	// (since they know the actual size).
	// allocate buffer
	// caller can't do it because they don't know what compression ratio
	// will be achieved.
	virtual LibError alloc_output(size_t in_size)
	{
		if(type == CT_COMPRESSION)
		{
			size_t required_size = (size_t)deflateBound(&zs, (uLong)in_size);
			RETURN_ERR(alloc_output_impl(required_size));
			return ERR_OK;
		}
		else
			WARN_RETURN(ERR_LOGIC);
	}


	virtual LibError consume(const void* in, size_t in_size, size_t& in_consumed, size_t& out_consumed)
	{
		zs.avail_in = (uInt)in_size;
		zs.next_in  = (Byte*)in;
		zs.next_out  = (Byte*)next_out;
		zs.avail_out = (uInt)avail_out;
		const size_t prev_avail_in  = zs.avail_in;
		const size_t prev_avail_out = zs.avail_out;

		int ret;
		if(type == CT_COMPRESSION)
			ret = deflate(&zs, 0);
		else
			ret = inflate(&zs, Z_SYNC_FLUSH);
		// sanity check: if ZLib reports end of stream, all input data
		// must have been consumed.
		if(ret == Z_STREAM_END)
		{
			debug_assert(zs.avail_in == 0);
			ret = Z_OK;
		}

		debug_assert(prev_avail_in >= zs.avail_in && prev_avail_out >= avail_out);
		in_consumed  = prev_avail_in - zs.avail_in;
		out_consumed = prev_avail_out- zs.avail_out;
		next_out  = zs.next_out;
		avail_out = zs.avail_out;

		CHECK_ERR(LibError_from_zlib(ret));
		return ERR_OK;
	}


	virtual LibError finish(void** out, size_t* out_size)
	{
		if(type == CT_COMPRESSION)
		{
			// notify zlib that no more data is forthcoming and have it flush output.
			// our output buffer has enough space due to use of deflateBound;
			// therefore, deflate must return Z_STREAM_END.
			int ret = deflate(&zs, Z_FINISH);
			if(ret != Z_STREAM_END)
				debug_warn("deflate: unexpected Z_FINISH behavior");
		}
		else
		{
			// nothing to do - decompression always flushes immediately
		}

		*out = zs.next_out - zs.total_out;
		*out_size = zs.total_out;
		return ERR_OK;
	}


	virtual void release()
	{
		// can have both input or output data remaining
		// (if not all data in uncompressed stream was needed)

		int ret;
		if(type == CT_COMPRESSION)
			ret = deflateEnd(&zs);
		else
			ret = inflateEnd(&zs);
		WARN_ERR(LibError_from_zlib(ret));
	}
};

#endif	// #ifndef NO_ZLIB


//-----------------------------------------------------------------------------

// allocator
static const size_t MAX_COMPRESSOR_SIZE = sizeof(ZLibCompressor);
static SingleAllocator<u8[MAX_COMPRESSOR_SIZE]> compressor_allocator;

uintptr_t comp_alloc(ContextType type, CompressionMethod method)
{
	void* c_mem = compressor_allocator.alloc();
	if(!c_mem)
		return 0;
	Compressor* c;

#include "nommgr.h"	// protect placement new and free() from macros
	switch(method)
	{
#ifndef NO_ZLIB
	case CM_DEFLATE:
		cassert(sizeof(ZLibCompressor) <= MAX_COMPRESSOR_SIZE);
		c = new(c_mem) ZLibCompressor(type);
		break;
#endif
	default:
		debug_warn("unknown compression type");
		compressor_allocator.free(c_mem);
		return 0;
#include "mmgr.h"
	}
#include "mmgr.h"

	c->init();
	return (uintptr_t)c;
}

LibError comp_reset(uintptr_t c_)
{
	Compressor* c = (Compressor*)c_;
	return c->reset();
}

// subsequent calls to comp_feed will unzip into <out>.
void comp_set_output(uintptr_t c_, void* out, size_t out_size)
{
	Compressor* c = (Compressor*)c_;
	c->set_output(out, out_size);
}

LibError comp_alloc_output(uintptr_t c_, size_t in_size)
{
	Compressor* c = (Compressor*)c_;
	return c->alloc_output(in_size);
}

void* comp_get_output(uintptr_t c_)
{
	Compressor* c = (Compressor*)c_;
	return c->get_output();
}

// unzip into output buffer. returns bytes written
// (may be 0, if not enough data is passed in), or < 0 on error.
ssize_t comp_feed(uintptr_t c_, const void* in, size_t in_size)
{
	Compressor* c = (Compressor*)c_;
	return c->feed(in, in_size);
}

LibError comp_finish(uintptr_t c_, void** out, size_t* out_size)
{
	Compressor* c = (Compressor*)c_;
	return c->finish(out, out_size);
}

void comp_free(uintptr_t c_)
{
	// no-op if context is 0 (i.e. was never allocated)
	if(!c_)
		return;

	Compressor* c = (Compressor*)c_;
	c->release();

	c->~Compressor();
#include "nommgr.h"
	compressor_allocator.free(c);
#include "mmgr.h"
}
