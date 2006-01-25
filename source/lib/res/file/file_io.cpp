#include "precompiled.h"

#include "lib.h"
#include "lib/posix.h"
#include "lib/allocators.h"
#include "lib/adts.h"
#include "file_internal.h"


//-----------------------------------------------------------------------------
// async I/O
//-----------------------------------------------------------------------------

// rationale:
// asynchronous IO routines don't cache; they're just a thin AIO wrapper.
// it's taken care of by file_io, which splits transfers into blocks
// and keeps temp buffers in memory (not user-allocated, because they
// might pull the rug out from under us at any time).
//
// caching here would be more complicated: would have to handle "forwarding",
// i.e. recognizing that the desired block has been issued, but isn't yet
// complete. file_io also knows more about whether a block should be cached.
//
// disadvantages:
// - streamed data will always be read from disk. no problem, because
//   such data (e.g. music, long speech) is unlikely to be used again soon.
// - prefetching (issuing the next few blocks from an archive during idle
//   time, so that future out-of-order reads don't need to seek) isn't
//   possible in the background (unless via thread, but that's discouraged).
//   the utility is questionable, though: how to prefetch so as not to delay
//   real IOs? can't determine "idle time" without completion notification,
//   which is hard.
//   we could get the same effect by bridging small gaps in file_io,
//   and rearranging files in the archive in order of access.


static Pool aiocb_pool;

static inline void aiocb_pool_init()
{
	(void)pool_create(&aiocb_pool, 32*sizeof(aiocb), sizeof(aiocb));
}

static inline void aiocb_pool_shutdown()
{
	(void)pool_destroy(&aiocb_pool);
}

static inline aiocb* aiocb_pool_alloc()
{
	ONCE(aiocb_pool_init());
	return (aiocb*)pool_alloc(&aiocb_pool, 0);
}

static inline void aiocb_pool_free(void* cb)
{
	pool_free(&aiocb_pool, cb);
}


// starts transferring to/from the given buffer.
// no attempt is made at aligning or padding the transfer.
LibError file_io_issue(File* f, off_t ofs, size_t size, void* p, FileIo* io)
{
	// zero output param in case we fail below.
	memset(io, 0, sizeof(FileIo));

	debug_printf("FILE| issue ofs=%d size=%d\n", ofs, size);


	//
	// check params
	//

	CHECK_FILE(f);

	if(!size || !p || !io)
		WARN_RETURN(ERR_INVALID_PARAM);

	const bool is_write = (f->fc.flags & FILE_WRITE) != 0;


	// cut off at EOF.
	if(!is_write)
	{
		const off_t bytes_left = f->fc.size - ofs;
		if(bytes_left < 0)
			WARN_RETURN(ERR_EOF);
		size = MIN(size, (size_t)bytes_left);
		size = round_up(size, AIO_SECTOR_SIZE);
	}

	// (we can't store the whole aiocb directly - glibc's version is
	// 144 bytes large)
	aiocb* cb = aiocb_pool_alloc();
	io->cb = cb;
	if(!cb)
		return ERR_NO_MEM;
	memset(cb, 0, sizeof(*cb));

	// send off async read/write request
	cb->aio_lio_opcode = is_write? LIO_WRITE : LIO_READ;
	cb->aio_buf        = (volatile void*)p;
	cb->aio_fildes     = f->fd;
	cb->aio_offset     = ofs;
	cb->aio_nbytes     = size;
	debug_printf("FILE| issue2 io=%p nbytes=%u\n", io, cb->aio_nbytes);
	int err = lio_listio(LIO_NOWAIT, &cb, 1, (struct sigevent*)0);
	if(err < 0)
	{
		debug_printf("lio_listio: %d, %d[%s]\n", err, errno, strerror(errno));
		(void)file_io_discard(io);
		WARN_RETURN(LibError_from_errno());
	}

	return ERR_OK;
}


// indicates if the IO referenced by <io> has completed.
// return value: 0 if pending, 1 if complete, < 0 on error.
int file_io_has_completed(FileIo* io)
{
	aiocb* cb = (aiocb*)io->cb;
	int ret = aio_error(cb);
	if(ret == EINPROGRESS)
		return 0;
	if(ret == 0)
		return 1;

	WARN_RETURN(ERR_FAIL);
}


LibError file_io_wait(FileIo* io, void*& p, size_t& size)
{
	debug_printf("FILE| wait io=%p\n", io);

	// zero output params in case something (e.g. H_DEREF) fails.
	p = 0;
	size = 0;

	aiocb* cb = (aiocb*)io->cb;

	// wait for transfer to complete.
	const aiocb** cbs = (const aiocb**)&cb;	// pass in an "array"
	while(aio_error(cb) == EINPROGRESS)
		aio_suspend(cbs, 1, (timespec*)0);	// wait indefinitely

	// query number of bytes transferred (-1 if the transfer failed)
	const ssize_t bytes_transferred = aio_return(cb);
	debug_printf("FILE| bytes_transferred=%d aio_nbytes=%u\n", bytes_transferred, cb->aio_nbytes);

// disabled: we no longer clamp to EOF
//	// (size was clipped to EOF in file_io => this is an actual IO error)
//	if(bytes_transferred < (ssize_t)cb->aio_nbytes)
//		return ERR_IO;

	p = (void*)cb->aio_buf;	// cast from volatile void*
	size = bytes_transferred;
	return ERR_OK;
}


LibError file_io_discard(FileIo* io)
{
	memset(io->cb, 0, sizeof(aiocb));	// prevent further use.
	aiocb_pool_free(io->cb);
	io->cb = 0;
	return ERR_OK;
}


LibError file_io_validate(const FileIo* io)
{
	const aiocb* cb = (const aiocb*)io->cb;
	// >= 0x100 is not necessarily bogus, but suspicious.
	// this also catches negative values.
	if((uint)cb->aio_fildes >= 0x100)
		return ERR_1;
	if(debug_is_pointer_bogus((void*)cb->aio_buf))
		return ERR_2;
	if(cb->aio_lio_opcode != LIO_WRITE && cb->aio_lio_opcode != LIO_READ && cb->aio_lio_opcode != LIO_NOP)
		return ERR_3;
	// all other aiocb fields have no invariants we could check.
	return ERR_OK;
}


//-----------------------------------------------------------------------------
// sync I/O
//-----------------------------------------------------------------------------

// the underlying aio implementation likes buffer and offset to be
// sector-aligned; if not, the transfer goes through an align buffer,
// and requires an extra memcpy2.
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


		u64 block_id;
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
			temp_buf = 0;
			block_id = 0;
			cached_block = 0;
		}
	};
	static const uint MAX_PENDING_IOS = 4;
	//RingBuf<IOSlot, MAX_PENDING_IOS> queue;
	std::deque<IOSlot> queue;

	// stop issuing and processing as soon as this changes
	LibError err;


	// bytes_processed is 0 if return value != { ERR_OK, INFO_CB_CONTINUE }
	// note: don't abort if = 0: zip callback may not actually
	// output anything if passed very little data.
	static LibError call_back(const void* block, size_t size,
		FileIOCB cb, uintptr_t ctx, size_t& bytes_processed)
	{
		if(cb)
		{
			LibError ret = cb(ctx, block, size, &bytes_processed);

			// failed - reset byte count in case callback didn't
			if(ret != ERR_OK && ret != INFO_CB_CONTINUE)
				bytes_processed = 0;

			return ret;
		}
		// no callback to process data: raw = actual
		else
		{
			bytes_processed = size;
			return INFO_CB_CONTINUE;
		}
	}


	ssize_t lowio()
	{
		const int fd = f->fd;

		lseek(fd, start_ofs, SEEK_SET);

		// emulate temp buffers - we take care of allocating and freeing.
		void* dst;
		void* dst_mem = 0;
		if(pbuf == FILE_BUF_TEMP)
		{
			dst_mem = malloc(size);
			if(!dst_mem)
				return ERR_NO_MEM;
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
			WARN_RETURN(LibError_from_errno());
		}

		size_t total_processed;
		LibError ret = call_back(dst, total_transferred, cb, cb_ctx, total_processed);
		free(dst_mem);
		CHECK_ERR(ret);
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
		}

		RETURN_ERR(file_buf_get(pbuf, size, f->fc.atom_fn, is_write, cb));

		return ERR_OK;
	}

	void issue(IOSlot& slot)
	{
		const off_t ofs = start_ofs+(off_t)total_issued;
		size_t issue_size;
		// write: must not issue beyond end of data.
		if(is_write)
			issue_size = MIN(FILE_BLOCK_SIZE, size - total_issued);
		// read: always grab whole blocks so we can put them in the cache.
		// any excess data (can only be within first or last block) is
		// discarded in wait().
		else
			issue_size = FILE_BLOCK_SIZE;

		// check if in cache
		slot.block_id = block_cache_make_id(f->fc.atom_fn, ofs);
		slot.cached_block = block_cache_find(slot.block_id);
		if(slot.cached_block)
			goto skip_issue;

		// if using buffer, set position in it; otherwise, use temp buffer
		void* buf;
		if(pbuf == FILE_BUF_TEMP)
			buf = slot.temp_buf = block_cache_alloc(slot.block_id);
		else
			buf = (char*)*pbuf + total_issued;

		LibError ret = file_io_issue(f, ofs, issue_size, buf, &slot.io);
		// transfer failed - loop will now terminate after
		// waiting for all pending transfers to complete.
		if(ret != ERR_OK)
			err = ret;

skip_issue:

		total_issued += issue_size;
	}

	void wait(IOSlot& slot, void*& block, size_t& block_size)
	{
		if(slot.cached_block)
		{
			block = (u8*)slot.cached_block;
			block_size = FILE_BLOCK_SIZE;
		}
		// wasn't in cache; it was issued, so wait for it
		else
		{
			LibError ret = file_io_wait(&slot.io, block, block_size);
			if(ret < 0)
				err = ret;
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
			memcpy2((char*)*pbuf+ofs_misalign+total_transferred, block, block_size);

		total_transferred += block_size;
	}

	void process(IOSlot& slot, void* block, size_t block_size, FileIOCB cb, uintptr_t ctx)
	{
		if(err == INFO_CB_CONTINUE)
		{
			size_t bytes_processed;
			err = call_back(block, block_size, cb, ctx, bytes_processed);
			if(err == INFO_CB_CONTINUE || err == ERR_OK)
				total_processed += bytes_processed;
			// else: processing failed.
			// loop will now terminate after waiting for all
			// pending transfers to complete.
		}

		if(!slot.cached_block)
			file_io_discard(&slot.io);

		if(!slot.cached_block && pbuf == FILE_BUF_TEMP)
			block_cache_mark_completed(slot.block_id);
	}


	ssize_t aio()
	{
again:
		{
			// data remaining to transfer, and no error:
			// start transferring next block.
			if(total_issued < size && err == INFO_CB_CONTINUE && queue.size() < MAX_PENDING_IOS)
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
		is_write = (f->fc.flags & FILE_WRITE ) != 0;
		no_aio =   (f->fc.flags & FILE_NO_AIO) != 0;

		cb = cb_;
		cb_ctx = cb_ctx_;

		start_ofs = ofs_;
		user_size = size_;
		pbuf = pbuf_;

		total_issued = 0;
		total_transferred = 0;
		total_processed = 0;
		err = INFO_CB_CONTINUE;
	}

	// now we read the file in 64 KiB chunks, N-buffered.
	// if reading from Zip, inflate while reading the next block.
	ssize_t run()
	{
		RETURN_ERR(prepare());

		ssize_t ret = no_aio? lowio() : aio();
///		FILE_STATS_NOTIFY_IO(fi, is_write? FO_WRITE : FO_READ, user_size, total_issued, start_time);

		debug_printf("FILE| err=%d, total_processed=%u\n", err, total_processed);

		// we allocated the memory: skip any leading padding
		if(pbuf != FILE_BUF_TEMP && !is_write)
			*pbuf = (u8*)*pbuf + ofs_misalign;

		if(err != INFO_CB_CONTINUE && err != ERR_OK)
			return (ssize_t)err;
		return ret;
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
	debug_printf("FILE| io: size=%u ofs=%u fn=%s\n", size, ofs, f->fc.atom_fn);

	CHECK_FILE(f);

	IOManager mgr(f, ofs, size, pbuf, cb, ctx);
	return mgr.run();
}




void file_io_shutdown()
{
	aiocb_pool_shutdown();
}
