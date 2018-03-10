/* Copyright (C) 2018 Wildfire Games.
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
#include "lib/timer.h"
#include "lib/file/file.h"
#include "lib/sysdep/filesystem.h"	// wtruncate
#include "lib/posix/posix_aio.h"	// LIO_READ, LIO_WRITE

#include "lib/allocators/unique_range.h"

namespace ERR
{
	const Status IO = -110301;
}

namespace io {

// @return memory suitable for use as an I/O buffer (address is a
// multiple of alignment, size is rounded up to a multiple of alignment)
// @param alignment is automatically increased if smaller than the
// UniqueRange requirement.
//
// use this instead of the file cache for write buffers that are
// never reused (avoids displacing other items).
static inline UniqueRange Allocate(size_t size, size_t alignment = maxSectorSize)
{
	return AllocateAligned(size, alignment);
}


#pragma pack(push, 1)

// required information for any I/O (this is basically the same as aiocb,
// but also applies to synchronous I/O and has shorter/nicer names.)
struct Operation
{
	// @param m_Buffer can be 0, in which case temporary block buffers are allocated.
	// otherwise, it must be aligned and padded to the I/O alignment, e.g. via
	// io::Allocate.
	Operation(const File& file, void* buf, off_t size, off_t offset = 0)
		: m_FileDescriptor(file.Descriptor()), m_OpenFlag((file.Flags() & O_WRONLY)? LIO_WRITE : LIO_READ)
		, m_Offset(offset), m_Size(size), m_Buffer(buf)
	{
	}

	void Validate() const
	{
		ENSURE(m_FileDescriptor >= 0);
		ENSURE(m_OpenFlag == LIO_READ || m_OpenFlag == LIO_WRITE);

		ENSURE(m_Offset >= 0);
		ENSURE(m_Size >= 0);
		// m_Buffer can legitimately be 0 (see above)
	}

	int m_FileDescriptor;
	int m_OpenFlag;

	off_t m_Offset;
	off_t m_Size;
	void* m_Buffer;
};


// optional information how an Operation is to be carried out
struct Parameters
{
	// default to single blocking I/Os
	Parameters()
		: alignment(1)	// no alignment requirements
		, blockSize(0)	// do not split into blocks
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
		ENSURE(is_pow2(alignment));
		ENSURE(alignment > 0);

		if(blockSize != 0)
		{
			ENSURE(is_pow2(blockSize));
			ENSURE(g_PageSize <= blockSize);	// (don't bother checking an upper bound)
		}

		ENSURE(1 <= queueDepth && queueDepth <= maxQueueDepth);

		ENSURE(IsAligned(op.m_Offset, alignment));
		// op.size doesn't need to be aligned
		ENSURE(IsAligned(op.m_Buffer, alignment));
	}

	// (ATTO only allows 10, which improves upon 8)
	static const size_t maxQueueDepth = 32;

	off_t alignment;

	size_t blockSize;	// 0 for one big "block"

	size_t queueDepth;
};

#define IO_OVERLAPPED io::Parameters(io::Parameters::OverlappedTag())


struct DefaultCompletedHook
{
	/**
	 * called after a block I/O has completed.
	 * @return Status (see RETURN_STATUS_FROM_CALLBACK).
	 *
	 * allows progress notification and processing data while waiting for
	 * previous I/Os to complete.
	 **/
	Status operator()(const u8* UNUSED(block), size_t UNUSED(blockSize)) const
	{
		return INFO::OK;
	}
};


struct DefaultIssueHook
{
	/**
	 * called before a block I/O is issued.
	 * @return Status (see RETURN_STATUS_FROM_CALLBACK).
	 *
	 * allows generating the data to write while waiting for
	 * previous I/Os to complete.
	 **/
	Status operator()(aiocb& UNUSED(cb)) const
	{
		return INFO::OK;
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
		const size_t blockSize = p.blockSize? p.blockSize : static_cast<size_t>(op.m_Size);

		const bool temporaryBuffersRequested = (op.m_Buffer == 0);
		if(temporaryBuffersRequested)
			buffers = io::Allocate(blockSize * p.queueDepth, p.alignment);

		for(size_t i = 0; i < ARRAY_SIZE(controlBlocks); i++)
		{
			aiocb& cb = operator[](i);
			cb.aio_fildes = op.m_FileDescriptor;
			cb.aio_nbytes = blockSize;
			cb.aio_lio_opcode = op.m_OpenFlag;
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


LIB_API Status Issue(aiocb& cb, size_t queueDepth);
LIB_API Status WaitUntilComplete(aiocb& cb, size_t queueDepth);


//-----------------------------------------------------------------------------
// Run

#ifndef ENABLE_IO_STATS
#define ENABLE_IO_STATS 0
#endif

// (hooks must be passed by const reference to allow passing rvalues.
// functors with non-const member data can mark them as mutable.)
template<class CompletedHook, class IssueHook>
static inline Status Run(const Operation& op, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook(), const IssueHook& issueHook = IssueHook())
{
	op.Validate();
	p.Validate(op);

	ControlBlockRingBuffer controlBlockRingBuffer(op, p);

#if ENABLE_IO_STATS
	const double t0 = timer_Time();
	COMPILER_FENCE;
#endif

	size_t numBlocks = p.blockSize? DivideRoundUp(static_cast<size_t>(op.m_Size), p.blockSize) : 1;
	for(size_t blocksIssued = 0, blocksCompleted = 0; blocksCompleted < numBlocks; blocksCompleted++)
	{
		for(; blocksIssued != numBlocks && blocksIssued < blocksCompleted + (off_t)p.queueDepth; blocksIssued++)
		{
			aiocb& cb = controlBlockRingBuffer[blocksIssued];
			cb.aio_offset = op.m_Offset + blocksIssued * p.blockSize;
			if(op.m_Buffer)
				cb.aio_buf = (volatile void*)(uintptr_t(op.m_Buffer) + blocksIssued * p.blockSize);
			if(blocksIssued == numBlocks-1)
				cb.aio_nbytes = round_up(size_t(op.m_Size - blocksIssued * p.blockSize), size_t(p.alignment));

			RETURN_STATUS_FROM_CALLBACK(issueHook(cb));

			RETURN_STATUS_IF_ERR(Issue(cb, p.queueDepth));
		}

		aiocb& cb = controlBlockRingBuffer[blocksCompleted];
		RETURN_STATUS_IF_ERR(WaitUntilComplete(cb, p.queueDepth));

		RETURN_STATUS_FROM_CALLBACK(completedHook((u8*)cb.aio_buf, cb.aio_nbytes));
	}

#if ENABLE_IO_STATS
	COMPILER_FENCE;
	const double t1 = timer_Time();
	const off_t totalSize = p.blockSize? numBlocks*p.blockSize : op.m_Size;
	debug_printf("IO: %.2f MB/s (%.2f)\n", totalSize/(t1-t0)/1e6, (t1-t0)*1e3);
#endif

	return INFO::OK;
}

// (overloads allow omitting parameters without requiring a template argument list)
template<class CompletedHook>
static inline Status Run(const Operation& op, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook())
{
	return Run(op, p, completedHook, DefaultIssueHook());
}

static inline Status Run(const Operation& op, const Parameters& p = Parameters())
{
	return Run(op, p, DefaultCompletedHook(), DefaultIssueHook());
}


//-----------------------------------------------------------------------------
// Store

// efficient writing requires preallocation; the resulting file is
// padded to the sector size and needs to be truncated afterwards.
// this function takes care of both.
template<class CompletedHook, class IssueHook>
static inline Status Store(const OsPath& pathname, const void* data, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook(), const IssueHook& issueHook = IssueHook())
{
	File file;
	int oflag = O_WRONLY;
	if(p.queueDepth != 1)
		oflag |= O_DIRECT;
	RETURN_STATUS_IF_ERR(file.Open(pathname, oflag));
	io::Operation op(file, (void*)data, size);

#if OS_WIN
	UNUSED2(waio_Preallocate(op.m_FileDescriptor, (off_t)size));
#endif

	RETURN_STATUS_IF_ERR(io::Run(op, p, completedHook, issueHook));

	file.Close();	// (required by wtruncate)

	RETURN_STATUS_IF_ERR(wtruncate(pathname, size));

	return INFO::OK;
}

template<class CompletedHook>
static inline Status Store(const OsPath& pathname, const void* data, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook())
{
	return Store(pathname, data, size, p, completedHook, DefaultIssueHook());
}

static inline Status Store(const OsPath& pathname, const void* data, size_t size, const Parameters& p = Parameters())
{
	return Store(pathname, data, size, p, DefaultCompletedHook(), DefaultIssueHook());
}


//-----------------------------------------------------------------------------
// Load

// convenience function provided for symmetry with Store.
template<class CompletedHook, class IssueHook>
static inline Status Load(const OsPath& pathname, void* buf, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook(), const IssueHook& issueHook = IssueHook())
{
	File file;
	int oflag = O_RDONLY;
	if(p.queueDepth != 1)
		oflag |= O_DIRECT;
	RETURN_STATUS_IF_ERR(file.Open(pathname, oflag));
	io::Operation op(file, buf, size);
	return io::Run(op, p, completedHook, issueHook);
}

template<class CompletedHook>
static inline Status Load(const OsPath& pathname, void* buf, size_t size, const Parameters& p = Parameters(), const CompletedHook& completedHook = CompletedHook())
{
	return Load(pathname, buf, size, p, completedHook, DefaultIssueHook());
}

static inline Status Load(const OsPath& pathname, void* buf, size_t size, const Parameters& p = Parameters())
{
	return Load(pathname, buf, size, p, DefaultCompletedHook(), DefaultIssueHook());
}

}	// namespace io

#endif	// #ifndef INCLUDED_IO
