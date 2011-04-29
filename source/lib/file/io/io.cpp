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

#include "precompiled.h"
#include "lib/file/io/io.h"

#include "lib/sysdep/rtl.h"

ERROR_ASSOCIATE(ERR::IO, L"Error during IO", EIO);

namespace io {

// the Windows aio implementation requires buffer and offset to be
// sector-aligned.
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
//   instead, this is done in manager.
// - transfer sizes here are arbitrary (i.e. not block-aligned);
//   that means the cache would have to handle this or also split them up
//   into blocks, which would duplicate the above mentioned work.
// - if caching here, we'd also have to handle "forwarding" (i.e.
//   desired block has been issued but isn't yet complete). again, it
//   is easier to let the synchronous manager handle this.
// - finally, manager knows more about whether the block should be cached
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


UniqueRange Allocate(size_t size, size_t alignment)
{
	debug_assert(is_pow2(alignment));
	if(alignment <= (size_t)idxDeleterBits)
		alignment = idxDeleterBits+1;

	const size_t alignedSize = round_up(size, alignment);
	const UniqueRange::pointer p = rtl_AllocateAligned(alignedSize, alignment);
	return UniqueRange(p, size, idxDeleterAligned);
}


LibError Issue(aiocb& cb, size_t queueDepth)
{
#if CONFIG2_FILE_ENABLE_AIO
	if(queueDepth > 1)
	{
		const int ret = (cb.aio_lio_opcode == LIO_WRITE)? aio_write(&cb): aio_read(&cb);
		RETURN_ERR(LibError_from_posix(ret));
	}
	else
#else
	UNUSED2(queueDepth);
#endif
	{
		debug_assert(lseek(cb.aio_fildes, cb.aio_offset, SEEK_SET) == cb.aio_offset);

		void* buf = (void*)cb.aio_buf;	// cast from volatile void*
		const ssize_t bytesTransferred = (cb.aio_lio_opcode == LIO_WRITE)? write(cb.aio_fildes, buf, cb.aio_nbytes) : read(cb.aio_fildes, buf, cb.aio_nbytes);
		if(bytesTransferred < 0)
			return LibError_from_errno();

		cb.aio_nbytes = (size_t)bytesTransferred;
	}

	return INFO::OK;
}


LibError WaitUntilComplete(aiocb& cb, size_t queueDepth)
{
#if CONFIG2_FILE_ENABLE_AIO
	if(queueDepth > 1)
	{
		aiocb* const cbs = &cb;
		timespec* const timeout = 0;	// infinite
SUSPEND_AGAIN:
		errno = 0;
		const int ret = aio_suspend(&cbs, 1, timeout);
		if(ret != 0)
		{
			if(errno == EINTR) // interrupted by signal
				goto SUSPEND_AGAIN;
			return LibError_from_errno();
		}

		const int err = aio_error(&cb);
		debug_assert(err != EINPROGRESS);	// else aio_return is undefined
		ssize_t bytesTransferred = aio_return(&cb);
		if(bytesTransferred == -1)	// transfer failed
		{
			errno = err;
			return LibError_from_errno();
		}
		cb.aio_nbytes = (size_t)bytesTransferred;
	}
#else
	UNUSED2(cb);
	UNUSED2(queueDepth);
#endif

	return INFO::OK;
}

}	// namespace io
