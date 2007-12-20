/**
 * =========================================================================
 * File        : io.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "io.h"

#include "lib/allocators/allocators.h"	// AllocatorChecker
#include "lib/bits.h"	// IsAligned, round_up
#include "lib/sysdep/cpu.h"	// cpu_memcpy
#include "lib/file/common/file_stats.h"
#include "lib/file/path.h"
#include "block_cache.h"
#include "io_internal.h"


ERROR_ASSOCIATE(ERR::FILE_ACCESS, "Insufficient access rights to open file", EACCES);
ERROR_ASSOCIATE(ERR::IO, "Error during IO", EIO);
ERROR_ASSOCIATE(ERR::IO_EOF, "Reading beyond end of file", -1);


// the underlying aio implementation likes buffer and offset to be
// sector-aligned; if not, the transfer goes through an align buffer,
// and requires an extra cpu_memcpy.
//
// if the user specifies an unaligned buffer, there's not much we can
// do - we can't assume the buffer contains padding. therefore,
// callers should let us allocate the buffer if possible.
//
// if ofs misalign = buffer, only the first and last blocks will need
// to be copied by aio, since we read up to the next block boundary.
// otherwise, everything will have to be copied; at least we split
// the read into blocks, so aio's buffer won't have to cover the
// whole file.

// we don't do any caching or alignment here - this is just a thin
// AIO wrapper. rationale:
// - aligning the transfer isn't possible here since we have no control
//   over the buffer, i.e. we cannot read more data than requested.
//   instead, this is done in io_manager.
// - transfer sizes here are arbitrary (i.e. not block-aligned);
//   that means the cache would have to handle this or also split them up
//   into blocks, which would duplicate the abovementioned work.
// - if caching here, we'd also have to handle "forwarding" (i.e.
//   desired block has been issued but isn't yet complete). again, it
//   is easier to let the synchronous io_manager handle this.
// - finally, io_manager knows more about whether the block should be cached
//   (e.g. whether another block request will follow), but we don't
//   currently make use of this.
//
// disadvantages:
// - streamed data will always be read from disk. that's not a problem,
//   because such data (e.g. music, long speech) is unlikely to be used
//   again soon.
// - prefetching (issuing the next few blocks from archive/file during
//   idle time to satisfy potential future IOs) requires extra buffers;
//   this is a bit more complicated than just using the cache as storage.


//-----------------------------------------------------------------------------
// allocator
//-----------------------------------------------------------------------------

#ifndef NDEBUG
static AllocatorChecker allocatorChecker;
#endif

class IoDeleter
{
public:
	IoDeleter(size_t paddedSize)
		: m_paddedSize(paddedSize)
	{
	}

	void operator()(u8* mem)
	{
		debug_assert(m_paddedSize != 0);
#ifndef NDEBUG
		allocatorChecker.notify_free(mem, m_paddedSize);
#endif
		page_aligned_free(mem, m_paddedSize);
		m_paddedSize = 0;
	}

private:
	size_t m_paddedSize;
};


shared_ptr<u8> io_Allocate(size_t size, off_t ofs)
{
	debug_assert(size != 0);

	const size_t misalignment = ofs % BLOCK_SIZE;
	const size_t paddedSize = (size_t)round_up(size+misalignment, BLOCK_SIZE);

	u8* mem = (u8*)page_aligned_alloc(paddedSize);
	if(!mem)
		throw std::bad_alloc();

#ifndef NDEBUG
	allocatorChecker.notify_alloc(mem, paddedSize);
#endif

	return shared_ptr<u8>(mem, IoDeleter(paddedSize));
}


//-----------------------------------------------------------------------------
// File
//-----------------------------------------------------------------------------

File::File()
{
}


File::~File()
{
	Close();
}


LibError File::Open(const Path& pathname, char mode)
{
	debug_assert(mode == 'w' || mode == 'r');

	m_pathname = pathname;
	m_mode = mode;

	int oflag = (mode == 'r')? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
#if OS_WIN
	oflag |= O_BINARY_NP;
#endif
	m_fd = open(m_pathname.external_file_string().c_str(), oflag, S_IRWXO|S_IRWXU|S_IRWXG);
	if(m_fd < 0)
		WARN_RETURN(ERR::FILE_ACCESS);

	stats_open();
	return INFO::OK;
}


void File::Close()
{
	m_mode = '\0';

	close(m_fd);
	m_fd = 0;
}


LibError File::Issue(aiocb& req, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const
{
	memset(&req, 0, sizeof(req));
	req.aio_lio_opcode = (m_mode == 'w')? LIO_WRITE : LIO_READ;
	req.aio_buf        = (volatile void*)alignedBuf;
	req.aio_fildes     = m_fd;
	req.aio_offset     = alignedOfs;
	req.aio_nbytes     = alignedSize;
	struct sigevent* sig = 0;	// no notification signal
	aiocb* const reqs = &req;
	if(lio_listio(LIO_NOWAIT, &reqs, 1, sig) != 0)
		return LibError_from_errno();
	return INFO::OK;
}


/*static*/ LibError File::WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize)
{
	// wait for transfer to complete.
	while(aio_error(&req) == EINPROGRESS)
	{
		aiocb* const reqs = &req;
		aio_suspend(&reqs, 1, (timespec*)0);	// wait indefinitely
	}

	const ssize_t bytesTransferred = aio_return(&req);
	if(bytesTransferred == -1)	// transfer failed
		WARN_RETURN(ERR::IO);

	alignedBuf = (u8*)req.aio_buf;	// cast from volatile void*
	alignedSize = bytesTransferred;
	return INFO::OK;
}


LibError File::IO(off_t ofs, u8* buf, size_t size) const
{
	ScopedIoMonitor monitor;

	lseek(m_fd, ofs, SEEK_SET);

	errno = 0;
	const ssize_t ret = (m_mode == 'w')? write(m_fd, buf, size) : read(m_fd, buf, size);
	if(ret < 0)
		return LibError_from_errno();

	const size_t totalTransferred = (size_t)ret;
	if(totalTransferred != size)
		WARN_RETURN(ERR::IO);

	monitor.NotifyOfSuccess(FI_LOWIO, m_mode, totalTransferred);
	return INFO::OK;
}


LibError File::Write(off_t ofs, const u8* buf, size_t size) const
{
	return IO(ofs, const_cast<u8*>(buf), size);
}


LibError File::Read(off_t ofs, u8* buf, size_t size) const
{
	return IO(ofs, buf, size);
}


//-----------------------------------------------------------------------------
// BlockIo
//-----------------------------------------------------------------------------

class BlockIo
{
public:
	LibError Issue(const File& file, off_t alignedOfs, u8* alignedBuf)
	{
		m_blockId = BlockId(file.Pathname(), alignedOfs);
		if(file.Mode() == 'r' && s_blockCache.Retrieve(m_blockId, m_cachedBlock))
		{
			stats_block_cache(CR_HIT);

			// copy from cache into user buffer
			if(alignedBuf)
			{
				cpu_memcpy(alignedBuf, m_cachedBlock.get(), BLOCK_SIZE);
				m_alignedBuf = alignedBuf;
			}
			// return cached block
			else
			{
				m_alignedBuf = const_cast<u8*>(m_cachedBlock.get());
			}

			return INFO::OK;
		}
		else
		{
			stats_block_cache(CR_MISS);
			stats_io_check_seek(m_blockId);

			// transfer directly to/from user buffer
			if(alignedBuf)
			{
				m_alignedBuf = alignedBuf;
			}
			// transfer into newly allocated temporary block
			else
			{
				m_tempBlock = io_Allocate(BLOCK_SIZE);
				m_alignedBuf = const_cast<u8*>(m_tempBlock.get());
			}

			return file.Issue(m_req, alignedOfs, m_alignedBuf, BLOCK_SIZE);
		}
	}

	LibError WaitUntilComplete(const u8*& block, size_t& blockSize)
	{
		if(m_cachedBlock)
		{
			block = m_alignedBuf;
			blockSize = BLOCK_SIZE;
			return INFO::OK;
		}

		RETURN_ERR(File::WaitUntilComplete(m_req, const_cast<u8*&>(block), blockSize));

		if(m_tempBlock)
			s_blockCache.Add(m_blockId, m_tempBlock);

		return INFO::OK;
	}

private:
	static BlockCache s_blockCache;

	BlockId m_blockId;

	// the address that WaitUntilComplete will return
	// (cached or temporary block, or user buffer)
	u8* m_alignedBuf;

	shared_ptr<u8> m_cachedBlock;
	shared_ptr<u8> m_tempBlock;

	aiocb m_req;
};

BlockCache BlockIo::s_blockCache;


//-----------------------------------------------------------------------------
// IoSplitter
//-----------------------------------------------------------------------------

class IoSplitter : boost::noncopyable
{
public:
	IoSplitter(off_t ofs, u8* alignedBuf, off_t size)
		: m_ofs(ofs), m_alignedBuf(alignedBuf), m_size(size)
		, m_totalIssued(0), m_totalTransferred(0)
	{
		m_misalignment = ofs % BLOCK_SIZE;
		m_alignedOfs = ofs - (off_t)m_misalignment;
		m_alignedSize = round_up(m_misalignment+size, BLOCK_SIZE);
	}

	LibError Run(const File& file, IoCallback cb = 0, uintptr_t cbData = 0)
	{
		ScopedIoMonitor monitor;

		// (issue even if cache hit because blocks must be processed in order)
		std::deque<BlockIo> pendingIos;
		for(;;)
		{
			while(pendingIos.size() < ioDepth && m_totalIssued < m_alignedSize)
			{
				pendingIos.push_back(BlockIo());
				const off_t alignedOfs = m_alignedOfs + m_totalIssued;
				u8* const alignedBuf = m_alignedBuf? m_alignedBuf+m_totalIssued : 0;
				RETURN_ERR(pendingIos.back().Issue(file, alignedOfs, alignedBuf));
				m_totalIssued += BLOCK_SIZE;
			}

			if(pendingIos.empty())
				break;

			Process(pendingIos.front(), cb, cbData);
			pendingIos.pop_front();
		}

		debug_assert(m_totalIssued >= m_totalTransferred && m_totalTransferred >= m_size);

		monitor.NotifyOfSuccess(FI_AIO, file.Mode(), m_totalTransferred);
		return INFO::OK;
	}

	size_t Misalignment() const
	{
		return m_misalignment;
	}

private:
	LibError Process(BlockIo& blockIo, IoCallback cb, uintptr_t cbData) const
	{
		const u8* block; size_t blockSize;
		RETURN_ERR(blockIo.WaitUntilComplete(block, blockSize));

		// first block: skip past alignment
		if(m_totalTransferred == 0)
		{
			block += m_misalignment;
			blockSize -= m_misalignment;
		}

		// last block: don't include trailing padding
		if(m_totalTransferred + (off_t)blockSize > m_size)
			blockSize = m_size - m_totalTransferred;

		m_totalTransferred += (off_t)blockSize;

		if(cb)
		{
			stats_cb_start();
			LibError ret = cb(cbData, block, blockSize);
			stats_cb_finish();
			CHECK_ERR(ret);
		}

		return INFO::OK;
	}

	off_t m_ofs;
	u8* m_alignedBuf;
	off_t m_size;

	size_t m_misalignment;
	off_t m_alignedOfs;
	off_t m_alignedSize;

	// (useful, raw data: possibly compressed, but doesn't count padding)
	mutable off_t m_totalIssued;
	mutable off_t m_totalTransferred;
};


LibError io_Scan(const File& file, off_t ofs, off_t size, IoCallback cb, uintptr_t cbData)
{
	u8* alignedBuf = 0;	// use temporary block buffers
	IoSplitter splitter(ofs, alignedBuf, size);
	return splitter.Run(file, cb, cbData);
}


LibError io_Read(const File& file, off_t ofs, u8* alignedBuf, size_t size, u8*& data)
{
	IoSplitter splitter(ofs, alignedBuf, (off_t)size);
	RETURN_ERR(splitter.Run(file));
	data = alignedBuf + splitter.Misalignment();
	return INFO::OK;
}


LibError io_WriteAligned(const File& file, off_t alignedOfs, const u8* alignedData, size_t size)
{
	debug_assert(IsAligned(alignedOfs, BLOCK_SIZE));
	debug_assert(IsAligned(alignedData, BLOCK_SIZE));

	IoSplitter splitter(alignedOfs, const_cast<u8*>(alignedData), (off_t)size);
	return splitter.Run(file);
}


LibError io_ReadAligned(const File& file, off_t alignedOfs, u8* alignedBuf, size_t size)
{
	debug_assert(IsAligned(alignedOfs, BLOCK_SIZE));
	debug_assert(IsAligned(alignedBuf, BLOCK_SIZE));

	IoSplitter splitter(alignedOfs, alignedBuf, (off_t)size);
	return splitter.Run(file);
}


//-----------------------------------------------------------------------------
// UnalignedWriter
//-----------------------------------------------------------------------------

class UnalignedWriter::Impl : boost::noncopyable
{
public:
	Impl(const File& file, off_t ofs)
		: m_file(file), m_alignedBuf(io_Allocate(BLOCK_SIZE))
	{
		const size_t misalignment = (size_t)ofs % BLOCK_SIZE;
		m_alignedOfs = ofs - (off_t)misalignment;
		if(misalignment)
			io_ReadAligned(m_file, m_alignedOfs, m_alignedBuf.get(), BLOCK_SIZE);
		m_bytesUsed = misalignment;
	}

	~Impl()
	{
		Flush();
	}

	LibError Append(const u8* data, size_t size) const
	{
		while(size != 0)
		{
			// optimization: write directly from the input buffer, if possible
			const size_t alignedSize = (size / BLOCK_SIZE) * BLOCK_SIZE;
			if(m_bytesUsed == 0 && IsAligned(data, BLOCK_SIZE) && alignedSize != 0)
			{
				RETURN_ERR(io_WriteAligned(m_file, m_alignedOfs, data, alignedSize));
				m_alignedOfs += (off_t)alignedSize;
				data += alignedSize;
				size -= alignedSize;
			}

			const size_t chunkSize = std::min(size, BLOCK_SIZE-m_bytesUsed);
			cpu_memcpy(m_alignedBuf.get()+m_bytesUsed, data, chunkSize);
			m_bytesUsed += chunkSize;
			data += chunkSize;
			size -= chunkSize;

			if(m_bytesUsed == BLOCK_SIZE)
				RETURN_ERR(WriteBlock());
		}

		return INFO::OK;
	}

	void Flush() const
	{
		if(m_bytesUsed)
		{
			memset(m_alignedBuf.get()+m_bytesUsed, 0, BLOCK_SIZE-m_bytesUsed);
			(void)WriteBlock();
		}
	}

private:
	LibError WriteBlock() const
	{
		RETURN_ERR(io_WriteAligned(m_file, m_alignedOfs, m_alignedBuf.get(), BLOCK_SIZE));
		m_alignedOfs += BLOCK_SIZE;
		m_bytesUsed = 0;
		return INFO::OK;
	}

	const File& m_file;
	mutable off_t m_alignedOfs;
	shared_ptr<u8> m_alignedBuf;
	mutable size_t m_bytesUsed;
};


UnalignedWriter::UnalignedWriter(const File& file, off_t ofs)
: impl(new Impl(file, ofs))
{
}

LibError UnalignedWriter::Append(const u8* data, size_t size) const
{
	return impl->Append(data, size);
}

void UnalignedWriter::Flush() const
{
	impl->Flush();
}
