/**
 * =========================================================================
 * File        : file_io.cpp
 * Project     : 0 A.D.
 * Description : provide fast I/O via POSIX aio and splitting into blocks.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "file_io.h"

#include <deque>

#include "lib/posix/posix_aio.h"
#include "lib/lib.h"
#include "lib/allocators.h"
#include "lib/adts.h"
#include "file_internal.h"


AT_STARTUP(\
	error_setDescription(ERR::IO, "Error during IO");\
	error_setDescription(ERR::IO_EOF, "Reading beyond end of file");\
	\
	error_setEquivalent(ERR::IO, EIO);\
)


//-----------------------------------------------------------------------------
// async I/O
//-----------------------------------------------------------------------------

struct PosixFileIo
{
	void* cb;	// aiocb
};
cassert(sizeof(PosixFileIo) <= FILE_IO_OPAQUE_SIZE);

// we don't do any caching or alignment here - this is just a thin AIO wrapper.
// rationale:
// - aligning the transfer isn't possible here since we have no control
//   over the buffer, i.e. we cannot read more data than requested.
//   instead, this is done in file_io.
// - transfer sizes here are arbitrary (viz. not block-aligned);
//   that means the cache would have to handle this or also split them up
//   into blocks, which is redundant (already done by file_io).
// - if caching here, we'd also have to handle "forwarding" (i.e.
//   desired block has been issued but isn't yet complete). again, it
//   is easier to let the synchronous file_io manager handle this.
// - finally, file_io knows more about whether the block should be cached
//   (e.g. whether another block request will follow), but we don't
//   currently make use of this.
//
// disadvantages:
// - streamed data will always be read from disk. no problem, because
//   such data (e.g. music, long speech) is unlikely to be used again soon.
// - prefetching (issuing the next few blocks from archive/file during
//   idle time to satisfy potential future IOs) requires extra buffers;
//   this is a bit more complicated than just using the cache as storage.

// FileIO must reference an aiocb, which is used to pass IO params to the OS.
// unfortunately it is 144 bytes on Linux - too much to put in FileIO,
// since that is stored in a 'resource control block' (see h_mgr.h).
// we therefore allocate dynamically, but via suballocator to avoid
// hitting the heap on every IO.
class AiocbAllocator
{
	Pool pool;
public:
	void init()
	{
		(void)pool_create(&pool, 32*sizeof(aiocb), sizeof(aiocb));
	}
	void shutdown()
	{
		(void)pool_destroy(&pool);
	}
	aiocb* alloc()
	{
		return (aiocb*)pool_alloc(&pool, 0);
	}
	// weird name to avoid trouble with mem tracker macros
	// (renaming is less annoying than #include "lib/nommgr.h")
	void free_(void* cb)
	{
		pool_free(&pool, cb);
	}
};
static AiocbAllocator aiocb_allocator;


// starts transferring to/from the given buffer.
// no attempt is made at aligning or padding the transfer.
LibError file_io_issue(File* f, off_t ofs, size_t size, void* p, FileIo* io)
{
	debug_printf("FILE| issue ofs=0x%X size=0x%X\n", ofs, size);

	// zero output param in case we fail below.
	memset(io, 0, sizeof(FileIo));

	// check params
	CHECK_FILE(f);
	if(!size || !p || !io)
		WARN_RETURN(ERR::INVALID_PARAM);
	const bool is_write = (f->flags & FILE_WRITE) != 0;

	PosixFileIo* pio = (PosixFileIo*)io;

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

	// (we can't store the whole aiocb directly - glibc's version is
	// 144 bytes large)
	aiocb* cb = aiocb_allocator.alloc();
	pio->cb = cb;
	if(!cb)
		WARN_RETURN(ERR::NO_MEM);
	memset(cb, 0, sizeof(*cb));

	// send off async read/write request
	cb->aio_lio_opcode = is_write? LIO_WRITE : LIO_READ;
	cb->aio_buf        = (volatile void*)p;
	cb->aio_fildes     = file_fd_from_PosixFile(f);
	cb->aio_offset     = ofs;
	cb->aio_nbytes     = size;
	int err = lio_listio(LIO_NOWAIT, &cb, 1, (struct sigevent*)0);
	if(err < 0)
	{
		debug_printf("lio_listio: %d, %d[%s]\n", err, errno, strerror(errno));
		(void)file_io_discard(io);
		return LibError_from_errno();
	}

	const BlockId disk_pos = block_cache_make_id(f->atom_fn, ofs);
	stats_io_check_seek(disk_pos.atom_fn, disk_pos.block_num);

	return INFO::OK;
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int file_io_has_completed(FileIo* io)
{
	PosixFileIo* pio = (PosixFileIo*)io;
	aiocb* cb = (aiocb*)pio->cb;
	int ret = aio_error(cb);
	if(ret == EINPROGRESS)
		return 0;
	if(ret == 0)
		return 1;

	WARN_RETURN(ERR::FAIL);
}


LibError file_io_wait(FileIo* io, void*& p, size_t& size)
{
	PosixFileIo* pio = (PosixFileIo*)io;
//	debug_printf("FILE| wait io=%p\n", io);

	// zero output params in case something (e.g. H_DEREF) fails.
	p = 0;
	size = 0;

	aiocb* cb = (aiocb*)pio->cb;

	// wait for transfer to complete.
	const aiocb** cbs = (const aiocb**)&cb;	// pass in an "array"
	while(aio_error(cb) == EINPROGRESS)
		aio_suspend(cbs, 1, (timespec*)0);	// wait indefinitely

	// query number of bytes transferred (-1 if the transfer failed)
	const ssize_t bytes_transferred = aio_return(cb);
//	debug_printf("FILE| bytes_transferred=%d aio_nbytes=%u\n", bytes_transferred, cb->aio_nbytes);

	// see if actual transfer count matches requested size.
	// note: most callers clamp to EOF but round back up to sector size
	// (see explanation in file_io_issue).
	debug_assert(bytes_transferred >= (ssize_t)(cb->aio_nbytes-AIO_SECTOR_SIZE));

	p = (void*)cb->aio_buf;	// cast from volatile void*
	size = bytes_transferred;
	return INFO::OK;
}


LibError file_io_discard(FileIo* io)
{
	PosixFileIo* pio = (PosixFileIo*)io;
	memset(pio->cb, 0, sizeof(aiocb));	// prevent further use.
	aiocb_allocator.free_(pio->cb);
	pio->cb = 0;
	return INFO::OK;
}


LibError file_io_validate(const FileIo* io)
{
	PosixFileIo* pio = (PosixFileIo*)io;
	const aiocb* cb = (const aiocb*)pio->cb;
	// >= 0x100 is not necessarily bogus, but suspicious.
	// this also catches negative values.
	if((uint)cb->aio_fildes >= 0x100)
		WARN_RETURN(ERR::_1);
	if(debug_is_pointer_bogus((void*)cb->aio_buf))
		WARN_RETURN(ERR::_2);
	if(cb->aio_lio_opcode != LIO_WRITE && cb->aio_lio_opcode != LIO_READ && cb->aio_lio_opcode != LIO_NOP)
		WARN_RETURN(ERR::_3);
	// all other aiocb fields have no invariants we could check.
	return INFO::OK;
}


//-----------------------------------------------------------------------------
// sync I/O
//-----------------------------------------------------------------------------

// set from sys_max_sector_size(); see documentation there.
size_t file_sector_size;


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


// helper routine used by functions that call back to a FileIOCB.
//
// bytes_processed is 0 if return value != { INFO::OK, INFO::CB_CONTINUE }
// note: don't abort if = 0: zip callback may not actually
// output anything if passed very little data.
LibError file_io_call_back(const void* block, size_t size,
	FileIOCB cb, uintptr_t ctx, size_t& bytes_processed)
{
	if(cb)
	{
		stats_cb_start();
		LibError ret = cb(ctx, block, size, &bytes_processed);
		stats_cb_finish();

		// failed - reset byte count in case callback didn't
		if(ret != INFO::OK && ret != INFO::CB_CONTINUE)
			bytes_processed = 0;

		CHECK_ERR(ret);	// user might not have raised a warning; make sure
		return ret;
	}
	// no callback to process data: raw = actual
	else
	{
		bytes_processed = size;
		return INFO::CB_CONTINUE;
	}
}


// interpret file_io parameters (pbuf, size, flags, cb) and allocate a
// file buffer if necessary.
// called by file_io and afile_read.
LibError file_io_get_buf(FileIOBuf* pbuf, size_t size,
	const char* atom_fn, uint file_flags, FileIOCB cb)
{
	// decode *pbuf - exactly one of these is true
	const bool temp  = (pbuf == FILE_BUF_TEMP);
	const bool alloc = !temp && (*pbuf == FILE_BUF_ALLOC);
	const bool user  = !temp && !alloc;

	const bool is_write = (file_flags & FILE_WRITE) != 0;
	const uint fb_flags = (file_flags & FILE_LONG_LIVED)? FB_LONG_LIVED : 0;

	// reading into temp buffers - ok.
	if(!is_write && temp && cb != 0)
		return INFO::OK;

	// reading and want buffer allocated.
	if(!is_write && alloc)
	{
		*pbuf = file_buf_alloc(size, atom_fn, fb_flags);
		if(!*pbuf)	// very unlikely (size totally bogus or cache hosed)
			WARN_RETURN(ERR::NO_MEM);
		return INFO::OK;
	}

	// writing from user-specified buffer - ok
	if(is_write && user)
		return INFO::OK;

	WARN_RETURN(ERR::INVALID_PARAM);
}


class IOManager
{
	File* f;
	bool is_write;
	bool no_aio;

	FileIOCB cb;
	uintptr_t cb_ctx;

	off_t start_ofs;
	FileIOBuf* pbuf;

	size_t user_size;
	size_t ofs_misalign;
	size_t size;

	// (useful, raw data: possibly compressed, but doesn't count padding)
	size_t total_issued;
	size_t total_transferred;
	// if callback, sum of what it reports; otherwise, = total_transferred
	// this is what we'll return.
	size_t total_processed;


	struct IOSlot
	{
		FileIo io;

		const void* cached_block;


		BlockId block_id;
		// needed so that we can add the block to the cache when
		// its IO is complete. if we add it when issuing, we'd no longer be
		// thread-safe: someone else might find it in the cache before its
		// transfer has completed. don't want to add an "is_complete" flag,
		// because that'd be hard to update (on every wait_io).


		void* temp_buf;

		IOSlot()
		{
			reset();
		}
		void reset()
		{
			memset(&io, 0, sizeof(io));
			cached_block = 0;
			memset(&block_id, 0, sizeof(block_id));
			temp_buf = 0;
		}
	};
	static const uint MAX_PENDING_IOS = 4;
	//RingBuf<IOSlot, MAX_PENDING_IOS> queue;
	std::deque<IOSlot> queue;

	// stop issuing and processing as soon as this changes
	LibError err;


	ssize_t lowio()
	{
		const int fd = file_fd_from_PosixFile(f);
		lseek(fd, start_ofs, SEEK_SET);

		// emulate temp buffers - we take care of allocating and freeing.
		void* dst;
		void* dst_mem = 0;
		if(pbuf == FILE_BUF_TEMP)
		{
			dst_mem = malloc(size);
			if(!dst_mem)
				WARN_RETURN(ERR::NO_MEM);
			dst = dst_mem;
		}
		else
			dst = (void*)*pbuf;

		ssize_t total_transferred;
		if(is_write)
			total_transferred = write(fd, dst, size);
		else
			total_transferred = read (fd, dst, size);
		if(total_transferred < 0)
		{
			free(dst_mem);
			return LibError_from_errno();
		}

		size_t total_processed;
		LibError ret = file_io_call_back(dst, total_transferred, cb, cb_ctx, total_processed);
		free(dst_mem);
		RETURN_ERR(ret);
		return (ssize_t)total_processed;
	}


	// align and pad the IO to FILE_BLOCK_SIZE
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

			ofs_misalign = start_ofs % FILE_BLOCK_SIZE;
			start_ofs -= (off_t)ofs_misalign;
			size = round_up(ofs_misalign + user_size, FILE_BLOCK_SIZE);

			// but cut off at EOF (necessary to prevent IO error).
			const off_t bytes_left = f->size - start_ofs;
			if(bytes_left < 0)
				WARN_RETURN(ERR::IO_EOF);
			size = MIN(size, (size_t)bytes_left);

			// and round back up to sector size.
			// see rationale in file_io_issue.
			size = round_up(size, AIO_SECTOR_SIZE);
		}

		RETURN_ERR(file_io_get_buf(pbuf, size, f->atom_fn, f->flags, cb));

		return INFO::OK;
	}

	void issue(IOSlot& slot)
	{
		const off_t ofs = start_ofs+(off_t)total_issued;
		// for both reads and writes, do not issue beyond end of file/data
		const size_t issue_size = MIN(FILE_BLOCK_SIZE, size - total_issued);
// try to grab whole blocks (so we can put them in the cache).
// any excess data (can only be within first or last) is
// discarded in wait().

		// check if in cache
		slot.block_id = block_cache_make_id(f->atom_fn, ofs);
		slot.cached_block = block_cache_find(slot.block_id);
		if(!slot.cached_block)
		{
			void* buf;

			// if using buffer, set position in it; otherwise, use temp buffer
			if(pbuf == FILE_BUF_TEMP)
				buf = slot.temp_buf = block_cache_alloc(slot.block_id);
			else
				buf = (char*)*pbuf + total_issued;

			LibError ret = file_io_issue(f, ofs, issue_size, buf, &slot.io);
			// transfer failed - loop will now terminate after
			// waiting for all pending transfers to complete.
			if(ret != INFO::OK)
				err = ret;
		}

		total_issued += issue_size;
	}

	void wait(IOSlot& slot, void*& block, size_t& block_size)
	{
		// get completed block address/size
		if(slot.cached_block)
		{
			block = (u8*)slot.cached_block;
			block_size = FILE_BLOCK_SIZE;
		}
		// .. wasn't in cache; it was issued, so wait for it
		else
		{
			LibError ret = file_io_wait(&slot.io, block, block_size);
			if(ret < 0)
				err = ret;
		}

		// special forwarding path: copy into block cache from
		// user's buffer. this necessary to efficiently support direct
		// IO of uncompressed files in archives.
		// note: must occur before skipping padding below.
		if(!slot.cached_block && pbuf != FILE_BUF_TEMP && f->flags & FILE_CACHE_BLOCK)
		{
			slot.temp_buf = block_cache_alloc(slot.block_id);
			cpu_memcpy(slot.temp_buf, block, block_size);
			// block_cache_mark_completed will be called in process()
		}

		// first time; skip past padding
		if(total_transferred == 0)
		{
			block = (u8*)block + ofs_misalign;
			block_size -= ofs_misalign;
		}

		// last time: don't include trailing padding
		if(total_transferred + block_size > user_size)
			block_size = user_size - total_transferred;

		// we have useable data from a previous temp buffer,
		// but it needs to be copied into the user's buffer
		if(slot.cached_block && pbuf != FILE_BUF_TEMP)
			cpu_memcpy((char*)*pbuf+ofs_misalign+total_transferred, block, block_size);

		total_transferred += block_size;
	}

	void process(IOSlot& slot, void* block, size_t block_size, FileIOCB cb, uintptr_t ctx)
	{
		if(err == INFO::CB_CONTINUE)
		{
			size_t bytes_processed;
			err = file_io_call_back(block, block_size, cb, ctx, bytes_processed);
			if(err == INFO::CB_CONTINUE || err == INFO::OK)
				total_processed += bytes_processed;
			// else: processing failed.
			// loop will now terminate after waiting for all
			// pending transfers to complete.
		}

		if(slot.cached_block)
			block_cache_release(slot.block_id);
		else
		{
			file_io_discard(&slot.io);

			if(slot.temp_buf)
				block_cache_mark_completed(slot.block_id);
		}
	}


	ssize_t aio()
	{
again:
		{
			// data remaining to transfer, and no error:
			// start transferring next block.
			if(total_issued < size && err == INFO::CB_CONTINUE && queue.size() < MAX_PENDING_IOS)
			{
				queue.push_back(IOSlot());
				IOSlot& slot = queue.back();
				issue(slot);
				goto again;
			}

			// IO pending: wait for it to complete, and process it.
			if(!queue.empty())
			{
				IOSlot& slot = queue.front();
				void* block; size_t block_size;
				wait(slot, block, block_size);
				process(slot, block, block_size, cb, cb_ctx);
				queue.pop_front();
				goto again;
			}
		}
		// (all issued OR error) AND no pending transfers - done.

		debug_assert(total_issued >= total_transferred && total_transferred >= user_size);
		return (ssize_t)total_processed;
	}

public:
	IOManager(File* f_, off_t ofs_, size_t size_, FileIOBuf* pbuf_,
		FileIOCB cb_, uintptr_t cb_ctx_)
	{
		f = f_;
		is_write = (f->flags & FILE_WRITE ) != 0;
		no_aio =   (f->flags & FILE_NO_AIO) != 0;

		cb = cb_;
		cb_ctx = cb_ctx_;

		start_ofs = ofs_;
		user_size = size_;
		pbuf = pbuf_;

		total_issued = 0;
		total_transferred = 0;
		total_processed = 0;
		err = INFO::CB_CONTINUE;
	}

	// now we read the file in 64 KiB chunks, N-buffered.
	// if reading from Zip, inflate while reading the next block.
	ssize_t run()
	{
		RETURN_ERR(prepare());

		const FileIOImplentation fi = no_aio? FI_LOWIO : FI_AIO;
		const FileOp fo = is_write? FO_WRITE : FO_READ;
		double start_time = 0.0;
		stats_io_sync_start(&start_time);
		ssize_t bytes_transferred = no_aio? lowio() : aio();
		stats_io_sync_finish(fi, fo, bytes_transferred, &start_time);

		// we allocated the memory: skip any leading padding
		if(pbuf != FILE_BUF_TEMP && !is_write)
		{
			FileIOBuf org_buf = *pbuf;
			*pbuf = (u8*)org_buf + ofs_misalign;
			if(ofs_misalign || size != user_size)
				file_buf_add_padding(org_buf, size, ofs_misalign);
		}

		if(err != INFO::CB_CONTINUE && err != INFO::OK)
			return (ssize_t)err;
		return bytes_transferred;
	}

};	// IOManager


// transfer <size> bytes, starting at <ofs>, to/from the given file.
// (read or write access was chosen at file-open time).
//
// if non-NULL, <cb> is called for each block transferred, passing <ctx>.
// it returns how much data was actually transferred, or a negative error
// code (in which case we abort the transfer and return that value).
// the callback mechanism is useful for user progress notification or
// processing data while waiting for the next I/O to complete
// (quasi-parallel, without the complexity of threads).
//
// return number of bytes transferred (see above), or a negative error code.
ssize_t file_io(File* f, off_t ofs, size_t size, FileIOBuf* pbuf,
	FileIOCB cb, uintptr_t ctx) // optional
{
	CHECK_FILE(f);

	// note: do not update stats/trace here: this includes Zip IOs,
	// which shouldn't be reported.

	IOManager mgr(f, ofs, size, pbuf, cb, ctx);
	return mgr.run();
}




void file_io_init()
{
	aiocb_allocator.init();
}


void file_io_shutdown()
{
	aiocb_allocator.shutdown();
}
