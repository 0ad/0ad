/**
 * =========================================================================
 * File        : 
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "io_manager.h"

#include <boost/shared_ptr.hpp>

#include "../posix/io_posix.h"
#include "../file_stats.h"
#include "block_cache.h"


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


// note: cutting off at EOF is necessary to avoid transfer errors,
// but makes size no longer sector-aligned, which would force
// waio to realign (slow). we want to pad back to sector boundaries
// afterwards (to avoid realignment), but that is not possible here
// since we have no control over the buffer (there might not be
// enough room in it). hence, do cut-off in IOManager.
//
// example: 200-byte file. IOManager issues (large) blocks;
// that ends up way beyond EOF, so ReadFile fails.
// limiting size to 200 bytes works, but causes waio to pad the
// transfer and use align buffer (slow).
// rounding up to 512 bytes avoids realignment and does not fail
// (apparently since NTFS files are sector-padded anyway?)


LibError io_InvokeCallback(const u8* block, size_t size, IoCallback cb, uintptr_t cbData, size_t& bytesProcessed)
{
	if(cb)
	{
		stats_cb_start();
		LibError ret = cb(cbData, block, size, &bytesProcessed);
		stats_cb_finish();

		// failed - reset byte count in case callback didn't
		if(ret != INFO::OK && ret != INFO::CB_CONTINUE)
			bytesProcessed = 0;

		CHECK_ERR(ret);	// user might not have raised a warning; make sure
		return ret;
	}
	// no callback to process data: raw = actual
	else
	{
		bytesProcessed = size;
		return INFO::CB_CONTINUE;
	}
}


//-----------------------------------------------------------------------------

class BlockIo
{
public:
	BlockIo()
		: m_blockId(), cachedBlock(0), tempBlock(0), m_posixIo()
	{
	}

	LibError Issue(File_Posix& file, off_t ofs, IoBuf buf, size_t size)
	{
		m_blockId = BlockId(file.Pathname(), ofs);

		// block already available in cache?
		cachedBlock = s_blockCache.Retrieve(m_blockId);
		if(cachedBlock)
		{
			stats_block_cache(CR_HIT);
			return INFO::OK;
		}

		stats_block_cache(CR_MISS);
		stats_io_check_seek(m_blockId);

		// use a temporary block if not writing to a preallocated buffer.
		if(!buf)
			buf = tempBlock = s_blockCache.Reserve(m_blockId);

		return m_posixIo.Issue(file, ofs, buf, size);
	}

	LibError WaitUntilComplete(const u8*& block, size_t& blockSize)
	{
		if(cachedBlock)
		{
			block = (u8*)cachedBlock;
			blockSize = BLOCK_SIZE;
			return INFO::OK;
		}

		return m_posixIo.WaitUntilComplete(block, blockSize);
	}

	void Discard()
	{
		if(cachedBlock)
		{
			s_blockCache.Release(m_blockId);
			cachedBlock = 0;
			return;
		}

		if(tempBlock)
		{
			s_blockCache.MarkComplete(m_blockId);
			tempBlock = 0;
		}
	}

private:
	static BlockCache s_blockCache;

	BlockId m_blockId;
	IoBuf cachedBlock;

	IoBuf tempBlock;

	Io_Posix m_posixIo;
};


//-----------------------------------------------------------------------------

class IOManager : boost::noncopyable
{
public:
	IOManager(File_Posix& file, off_t ofs, IoBuf buf, size_t size, IoCallback cb = 0, uintptr_t cbData = 0)
		: m_file(file)
		, start_ofs(ofs), user_size(size)
		, m_cb(cb), m_cbData(cbData)
		, m_totalIssued(0), m_totalTransferred(0), m_totalProcessed(0)
		, err(INFO::CB_CONTINUE)
	{
	}


	// now we read the file in 64 KiB chunks, N-buffered.
	// if reading from Zip, inflate while reading the next block.
	LibError run()
	{
		ScopedIoMonitor monitor;

		aio();

		if(err != INFO::CB_CONTINUE && err != INFO::OK)
			return (ssize_t)err;

		debug_assert(m_totalIssued >= m_totalTransferred && m_totalTransferred >= user_size);

		monitor.NotifyOfSuccess(FI_AIO, m_file.Mode(), m_totalTransferred);
		return m_totalProcessed;
	}


private:
	void wait(BlockIo& blockIo, u8*& block, size_t& blockSize)
	{
		LibError ret = blockIo.WaitUntilComplete(block, blockSize);
		if(ret < 0)
			err = ret;
	
		// first time; skip past padding
		if(m_totalTransferred == 0)
		{
			block = (u8*)block + ofs_misalign;
			blockSize -= ofs_misalign;
		}

		// last time: don't include trailing padding
		if(m_totalTransferred + blockSize > user_size)
			blockSize = user_size - m_totalTransferred;

		// we have useable data from a previous temp buffer,
		// but it needs to be copied into the user's buffer
		if(blockIo.cachedBlock && pbuf != IO_BUF_TEMP)
			cpu_memcpy((char*)*pbuf+ofs_misalign+m_totalTransferred, block, blockSize);

		m_totalTransferred += blockSize;
	}


	// align and pad the IO to BLOCK_SIZE
	// (reduces work for AIO implementation).
	LibError prepare()
	{
		ofs_misalign = 0;
		size = user_size;

		if(!is_write && !no_aio)
		{
			// note: we go to the trouble of aligning the first block (instead of
			// just reading up to the next block and letting aio realign it),
			// so that it can be taken from the cache.
			// this is not possible if we don't allocate the buffer because
			// extra space must be added for the padding.

			ofs_misalign = start_ofs % BLOCK_SIZE;
			start_ofs -= (off_t)ofs_misalign;
			size = round_up(ofs_misalign + user_size, BLOCK_SIZE);

			// but cut off at EOF (necessary to prevent IO error).
			const off_t bytes_left = f->size - start_ofs;
			if(bytes_left < 0)
				WARN_RETURN(ERR::IO_EOF);
			size = std::min(size, (size_t)bytes_left);

			// and round back up to sector size.
			// see rationale in file_io_issue.
			const size_t AIO_SECTOR_SIZE = 512;
			size = round_up(size, AIO_SECTOR_SIZE);
		}

		RETURN_ERR(file_io_get_buf(pbuf, size, f->atom_fn, f->flags, cb));

		// see if actual transfer count matches requested size.
		// note: most callers clamp to EOF but round back up to sector size
		// (see explanation in file_io_issue).
		////debug_assert(bytes_transferred >= (ssize_t)(m_aiocb.aio_nbytes-AIO_SECTOR_SIZE));


		return INFO::OK;
	}


	void aio()
	{
		RETURN_ERR(prepare());

again:
		{
			// data remaining to transfer, and no error:
			// start transferring next block.
			if(m_totalIssued < size && err == INFO::CB_CONTINUE && queue.size() < MAX_PENDING_IOS)
			{
				queue.push_back(BlockIo());
				BlockIo& blockIo = queue.back();

				const off_t ofs = start_ofs+(off_t)m_totalIssued;
				// for both reads and writes, do not issue beyond end of file/data
				const size_t issue_size = std::min(BLOCK_SIZE, size - m_totalIssued);
				// try to grab whole blocks (so we can put them in the cache).
				// any excess data (can only be within first or last) is
				// discarded in wait().

				if(pbuf == IO_BUF_TEMP)
					buf = 0;
				else
					buf = (char*)*pbuf + m_totalIssued;

				LibError ret = blockIo.Issue();
				// transfer failed - loop will now terminate after
				// waiting for all pending transfers to complete.
				if(ret != INFO::OK)
					err = ret;

				m_totalIssued += issue_size;

				goto again;
			}

			// IO pending: wait for it to complete, and process it.
			if(!queue.empty())
			{
				BlockIo& blockIo = queue.front();
				u8* block; size_t blockSize;
				wait(blockIo, block, blockSize);


				if(err == INFO::CB_CONTINUE)
				{
					size_t bytesProcessed;
					LibError ret = io_InvokeCallback(block, blockSize, m_cb, m_cbData, bytesProcessed);
					if(ret == INFO::CB_CONTINUE || ret == INFO::OK)
						m_totalProcessed += bytesProcessed;
					// processing failed - loop will now terminate after
					// waiting for all pending transfers to complete.
					else
						err = ret;
				}

				blockIo.Discard();


				queue.pop_front();
				goto again;
			}
		}
		// (all issued OR error) AND no pending transfers - done.


		// we allocated the memory: skip any leading padding
		if(not_temp && !is_write)
		{
			IoBuf org_buf = *pbuf;
			*pbuf = (u8*)org_buf + ofs_misalign;
			if(ofs_misalign || size != user_size)
				assert(0);	// TODO": no longer supported, rule this out
		}

	}

	File_Posix& m_file;
	bool m_isWrite;

off_t start_ofs;
size_t user_size;
	IoCallback m_cb;
	uintptr_t m_cbData;

	// (useful, raw data: possibly compressed, but doesn't count padding)
	size_t m_totalIssued;
	size_t m_totalTransferred;
	// if callback, sum of what it reports; otherwise, = m_totalTransferred
	// this is what we'll return.
	size_t m_totalProcessed;

	// stop issuing and processing as soon as this changes
	LibError err;

IoBuf* pbuf;
size_t ofs_misalign;
size_t size;

	static const uint MAX_PENDING_IOS = 4;
	//RingBuf<BlockIo, MAX_PENDING_IOS> queue;
	std::deque<BlockIo> queue;
};
