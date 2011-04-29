/* Copyright (c) 2011 Wildfire Games
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
 * provide asynchronous and synchronous I/O with hooks to allow
 * overlapped processing or progress reporting.
 */

#ifndef INCLUDED_IO
#define INCLUDED_IO

#include "lib/config2.h"
#include "lib/alignment.h"
#include "lib/bits.h"
#include "lib/file/file.h"
#include "lib/sysdep/filesystem.h"	// wtruncate

#include "lib/allocators/unique_range.h"

namespace ERR
{
	const LibError IO = -110301;
}

namespace io {

// @return memory suitable for use as an I/O buffer (address is a
// multiple of alignment, size is rounded up to a multiple of alignment)
//
// use this instead of the file cache for write buffers that are
// never reused (avoids displacing other items).
LIB_API UniqueRange Allocate(size_t size, size_t alignment = maxSectorSize);


#pragma pack(push, 1)

// required information for any I/O (this is basically the same as aiocb,
// but also applies to synchronous I/O and has shorter/nicer names.)
struct Operation
{
	// @param buf can be 0, in which case temporary block buffers are allocated.
	// otherwise, it must be padded to the I/O alignment, e.g. via io::Allocate.
	Operation(const File& file, void* buf, off_t size, off_t offset = 0)
		: fd(file.Descriptor()), opcode(file.Opcode())
		, offset(offset), size(size), buf((void*)buf)
	{
	}

	void Validate() const
	{
		debug_assert(fd >= 0);
		debug_assert(opcode == LIO_READ || opcode == LIO_WRITE);

		debug_assert(offset >= 0);
		debug_assert(size >= 0);
		// buf can legitimately be 0 (see above)
	}

	int fd;
	int opcode;

	off_t offset;
	off_t size;
	void* buf;
};


// optional information how an Operation is to be carried out
struct Parameters
{
	// default to single blocking I/Os
	Parameters()
		: alignment(1)	// no alignment requirements
		// use one huge "block" truncated to the requested size.
		// (this value is a power of two as required by Validate and
		// avoids overflowing off_t in DivideRoundUp)
		, blockSize((SIZE_MAX/2)+1)
		, queueDepth(1)	// disable aio
	{
	}

	// parameters for asynchronous I/O that maximize throughput on current drives
	struct OverlappedTag {};
	Parameters(OverlappedTag)
		: alignment(maxSectorSize), blockSize(128*KiB), queueDepth(32)
	{
	}

	Parameters(size_t blockSize, size_t queueDepth, off_t alignment = maxSectorSize)
		: alignment(alignment), blockSize(blockSize), queueDepth(queueDepth)
	{
	}

	void Validate(const Operation& op) const
	{
		debug_assert(is_pow2(alignment));
		debug_assert(alignment > 0);

		debug_assert(is_pow2(blockSize));
		debug_assert(pageSize <= blockSize);	// no upper limit needed

		debug_assert(1 <= queueDepth && queueDepth <= maxQueueDepth);

		debug_assert(IsAligned(op.offset, alignment));
		// op.size doesn't need to be aligned
		debug_assert(IsAligned(op.buf, alignment));
	}

	// (ATTO only allows 10, which improves upon 8)
	static const size_t maxQueueDepth = 32;

	off_t alignment;

	size_t blockSize;

	size_t queueDepth;
};

#define IO_OVERLAPPED io::Parameters(io::Parameters::OverlappedTag())


struct DefaultCompletedHook
{
	/**
	 * called after a block I/O has completed.
	 *
	 * @return INFO::CB_CONTINUE to proceed; any other value will
	 * be immediately returned by Run.
	 *
	 * allows progress notification and processing data while waiting for
	 * previous I/Os to complete.
	 **/
	LibError operator()(const u8* UNUSED(block), size_t UNUSED(blockSize)) const
	{
		return INFO::CB_CONTINUE;
	}
};


struct DefaultIssueHook
{
	/**
	 * called before a block I/O is issued.
	 *
	 * @return INFO::CB_CONTINUE to proceed; any other value will
	 * be immediately returned by Run.
	 *
	 * allows generating the data to write while waiting for
	 * previous I/Os to complete.
	 **/
	LibError operator()(aiocb& UNUSED(cb)) const
	{
		return INFO::CB_CONTINUE;
	}
};


// ring buffer of partially initialized aiocb that can be passed
// directly to aio_write etc. after setting offset and buffer.
class ControlBlockRingBuffer
{
public:
	ControlBlockRingBuffer(const Operation& op, const Parameters& p)
		: controlBlocks()	// zero-initialize
	{
		// (default p.blockSize is "infinity", so clamp to the total size)
		const size_t blockSize = (size_t)std::min((off_t)p.blockSize, op.size);

		const bool temporaryBuffersRequested = (op.buf == 0);
		if(temporaryBuffersRequested)
			buffers = RVALUE(io::Allocate(blockSize * p.queueDepth, p.alignment));

		for(size_t i = 0; i < ARRAY_SIZE(controlBlocks); i++)
		{
			aiocb& cb = operator[](i);
			cb.aio_fildes = op.fd;
			cb.aio_nbytes = blockSize;
			cb.aio_lio_opcode = op.opcode;
			if(temporaryBuffersRequested)
				cb.aio_buf = (volatile void*)(uintptr_t(buffers.get()) + i * blockSize);
		}
	}

	INLINE aiocb& operator[](size_t counter)
	{
		return controlBlocks[counter % ARRAY_SIZE(controlBlocks)];
	}

private:
	UniqueRange buffers;
	aiocb controlBlocks[Parameters::maxQueueDepth];
};

#pragma pack(pop)


LIB_API LibError Issue(aiocb& cb, size_t queueDepth);
LIB_API LibError WaitUntilComplete(aiocb& cb, size_t queueDepth);


//-----------------------------------------------------------------------------
// Run

// (hooks must be passed by const reference to allow passing rvalues.
// functors with non-const member data can mark them as mutable.)
template<class CompletedHook, class IssueHook>
static inline LibError Run(const Operation& op, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook(), const IssueHook& issueHook = IssueHook())
{
	op.Validate();
	p.Validate(op);

	ControlBlockRingBuffer controlBlockRingBuffer(op, p);

	const off_t numBlocks = DivideRoundUp(op.size, (off_t)p.blockSize);
	for(off_t blocksIssued = 0, blocksCompleted = 0; blocksCompleted < numBlocks; blocksCompleted++)
	{
		for(; blocksIssued != numBlocks && blocksIssued < blocksCompleted + (off_t)p.queueDepth; blocksIssued++)
		{
			aiocb& cb = controlBlockRingBuffer[blocksIssued];
			cb.aio_offset = op.offset + blocksIssued * p.blockSize;
			if(op.buf)
				cb.aio_buf = (volatile void*)(uintptr_t(op.buf) + blocksIssued * p.blockSize);
			if(blocksIssued == numBlocks-1)
				cb.aio_nbytes = round_up(size_t(op.size - blocksIssued * p.blockSize), size_t(p.alignment));

			RETURN_IF_NOT_CONTINUE(issueHook(cb));

			RETURN_ERR(Issue(cb, p.queueDepth));
		}

		aiocb& cb = controlBlockRingBuffer[blocksCompleted];
		RETURN_ERR(WaitUntilComplete(cb, p.queueDepth));

		RETURN_IF_NOT_CONTINUE(completedHook((u8*)cb.aio_buf, cb.aio_nbytes));
	}

	return INFO::OK;
}

// (overloads allow omitting parameters without requiring a template argument list)
template<class CompletedHook>
static inline LibError Run(const Operation& op, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook())
{
	return Run(op, p, completedHook, DefaultIssueHook());
}

static inline LibError Run(const Operation& op, const Parameters& p = Parameters())
{
	return Run(op, p, DefaultCompletedHook(), DefaultIssueHook());
}


//-----------------------------------------------------------------------------
// Store

// efficient writing requires preallocation, and the resulting file is
// padded to the sector size and needs to be truncated afterwards.
// this function takes care of both.
template<class CompletedHook, class IssueHook>
static inline LibError Store(const OsPath& pathname, const void* data, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook(), const IssueHook& issueHook = IssueHook())
{
	File file(pathname, LIO_WRITE);
	io::Operation op(file, (void*)data, size);

#if OS_WIN && CONFIG2_FILE_ENABLE_AIO
	(void)waio_Preallocate(op.fd, (off_t)size, p.alignment);
#endif

	RETURN_ERR(io::Run(op, p, completedHook, issueHook));

	file.Close();	// (required by wtruncate)

	RETURN_ERR(wtruncate(pathname, size));

	return INFO::OK;
}

template<class CompletedHook>
static inline LibError Store(const OsPath& pathname, const void* data, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook())
{
	return Store(pathname, data, size, p, completedHook, DefaultIssueHook());
}

static inline LibError Store(const OsPath& pathname, const void* data, size_t size, const Parameters& p = Parameters())
{
	return Store(pathname, data, size, p, DefaultCompletedHook(), DefaultIssueHook());
}


//-----------------------------------------------------------------------------
// Load

// convenience function provided for symmetry with Store
template<class CompletedHook, class IssueHook>
static inline LibError Load(const OsPath& pathname, void* buf, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook(), const IssueHook& issueHook = IssueHook())
{
	File file(pathname, LIO_READ);
	io::Operation op(file, buf, size);
	return io::Run(op, p, completedHook, issueHook);
}

template<class CompletedHook>
static inline LibError Load(const OsPath& pathname, void* buf, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook())
{
	return Load(pathname, buf, size, p, completedHook, DefaultIssueHook());
}

static inline LibError Load(const OsPath& pathname, void* buf, size_t size, const Parameters& p = Parameters())
{
	return Load(pathname, buf, size, p, DefaultCompletedHook(), DefaultIssueHook());
}

}	// namespace io

#endif	// #ifndef INCLUDED_IO
