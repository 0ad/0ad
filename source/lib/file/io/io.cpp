/* Copyright (C) 2011 Wildfire Games.
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

static const StatusDefinition ioStatusDefinitions[] = {
	{ ERR::IO, L"Error during IO", EIO }
};
STATUS_ADD_DEFINITIONS(ioStatusDefinitions);

namespace io {

// this is just a thin wrapper on top of lowio and POSIX aio.
// note that the Windows aio implementation requires buffers, sizes and
// offsets to be sector-aligned.

Status Issue(aiocb& cb, size_t queueDepth)
{
#if CONFIG2_FILE_ENABLE_AIO
	if(queueDepth > 1)
	{
		const int ret = (cb.aio_lio_opcode == LIO_WRITE)? aio_write(&cb): aio_read(&cb);
		if(ret != 0)
			WARN_RETURN(StatusFromErrno());
	}
	else
#else
	UNUSED2(queueDepth);
#endif
	{
		ENSURE(lseek(cb.aio_fildes, cb.aio_offset, SEEK_SET) == cb.aio_offset);

		void* buf = (void*)cb.aio_buf;	// cast from volatile void*
		const ssize_t bytesTransferred = (cb.aio_lio_opcode == LIO_WRITE)? write(cb.aio_fildes, buf, cb.aio_nbytes) : read(cb.aio_fildes, buf, cb.aio_nbytes);
		if(bytesTransferred < 0)
			WARN_RETURN(StatusFromErrno());

		cb.aio_nbytes = (size_t)bytesTransferred;
	}

	return INFO::OK;
}


Status WaitUntilComplete(aiocb& cb, size_t queueDepth)
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
			WARN_RETURN(StatusFromErrno());
		}

		const int err = aio_error(&cb);
		ENSURE(err != EINPROGRESS);	// else aio_return is undefined
		ssize_t bytesTransferred = aio_return(&cb);
		if(bytesTransferred == -1)	// transfer failed
		{
			errno = err;
			WARN_RETURN(StatusFromErrno());
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
