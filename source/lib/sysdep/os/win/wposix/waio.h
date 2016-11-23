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
 * emulate POSIX asynchronous I/O on Windows.
 */

#ifndef INCLUDED_WAIO
#define INCLUDED_WAIO

#include "lib/status.h"
#include "lib/os_path.h"
#include "lib/posix/posix_time.h"	// timespec
#include "lib/sysdep/os/win/wposix/wposix_types.h"

// Note: transfer buffers, offsets, and lengths must be sector-aligned
// (we don't bother copying to an align buffer because our block cache
// already requires splitting IOs into naturally-aligned blocks)


//
// <signal.h>
//

union sigval	// unused
{
	int sival_int;				// Integer signal value.
	void* sival_ptr;			// Pointer signal value.
};

struct sigevent	// unused
{
	int sigev_notify;			// notification mode
	int sigev_signo;			// signal number
	union sigval sigev_value;	// signal value
	void (*sigev_notify_function)(union sigval);
};


//
// <aio.h>
//

struct aiocb
{
	int             aio_fildes;     // File descriptor.
	off_t           aio_offset;     // File offset.
	volatile void*  aio_buf;        // Location of buffer.
	size_t          aio_nbytes;     // Length of transfer.
	int             aio_reqprio;    // Request priority offset. (unused)
	struct sigevent aio_sigevent;   // Signal number and value. (unused)
	int             aio_lio_opcode; // Operation to be performed.

	// internal use only; must be zero-initialized before
	// calling the first aio_read/aio_write/lio_listio
	// (aio_return resets it to 0)
	void* ovl;
};

enum
{
	// aio_cancel return
	AIO_ALLDONE,     // None of the requested operations could be canceled since they are already complete.
	AIO_CANCELED,    // All requested operations have been canceled.
	AIO_NOTCANCELED, // Some of the requested operations could not be canceled since they are in progress.

	// lio_listio mode
	LIO_WAIT,        // wait until all I/O is complete
	LIO_NOWAIT,

	// lio_listio ops
	LIO_NOP,
	LIO_READ,
	LIO_WRITE
};

extern int aio_read(struct aiocb*);
extern int aio_write(struct aiocb*);
extern int lio_listio(int, struct aiocb* const[], int, struct sigevent*);

// (if never called, IOCP notifications will pile up.)
extern int aio_suspend(const struct aiocb* const[], int, const struct timespec*);

// @return status of transfer (0 or an errno)
extern int aio_error(const struct aiocb*);

// @return bytes transferred or -1 on error.
// frees internal storage related to the request and MUST be called
// exactly once for each aiocb after aio_error != EINPROGRESS.
extern ssize_t aio_return(struct aiocb*);

extern int aio_cancel(int, struct aiocb*);

extern int aio_fsync(int, struct aiocb*);

// open the file for aio (not possible via _wsopen_s since it cannot
// set FILE_FLAG_NO_BUFFERING).
//
// @return the smallest available file descriptor. NB: these numbers
// are not 0-based to avoid confusion with lowio descriptors and
// must only be used with waio functions.
extern Status waio_open(const OsPath& pathname, int oflag, ...);

extern Status waio_close(int fd);

// call this before writing a large file to preallocate clusters, thus
// reducing fragmentation.
//
// @param fd file descriptor from _wsopen_s OR waio_open
// @param size is rounded up to a multiple of maxSectorSize (required by
// SetEndOfFile; this could be avoided by using the undocumented
// NtSetInformationFile or SetFileInformationByHandle on Vista and later).
// use wtruncate after I/O is complete to chop off the excess padding.
//
// NB: writes that extend a file (i.e. ALL WRITES when creating new files)
// are synchronous, which prevents overlapping I/O and other work.
// (http://support.microsoft.com/default.aspx?scid=kb%3Ben-us%3B156932)
// if Windows XP and the SE_MANAGE_VOLUME_NAME privileges are available,
// this function sets the valid data length to avoid the synchronous zero-fill.
// to avoid exposing the previous disk contents until the application
// successfully writes to the file, deny sharing when opening the file.
LIB_API Status waio_Preallocate(int fd, off_t size);

#endif	// #ifndef INCLUDED_WAIO
