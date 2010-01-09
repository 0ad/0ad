/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * emulate POSIX asynchronous I/O on Windows.
 */

#ifndef INCLUDED_WAIO
#define INCLUDED_WAIO

#include "wposix_types.h"

#include "no_crt_posix.h"

#include "lib/lib_errors.h"

// Note: transfer buffers, offsets, and lengths must be sector-aligned
// (we don't bother copying to an align buffer because the file cache
// already requires splitting IOs into aligned blocks)


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
// <unistd.h>
//

extern int read (int fd, void* buf, size_t nbytes);	// thunk
extern int write(int fd, void* buf, size_t nbytes);	// thunk
extern off_t lseek(int fd, off_t ofs, int whence);  // thunk


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

	class Impl;
	shared_ptr<Impl> impl;
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

extern int aio_cancel(int, struct aiocb*);
extern int aio_error(const struct aiocb*);
extern int aio_fsync(int, struct aiocb*);
extern int aio_read(struct aiocb*);
extern ssize_t aio_return(struct aiocb*);
struct timespec;
extern int aio_suspend(const struct aiocb* const[], int, const struct timespec*);
extern int aio_write(struct aiocb*);
extern int lio_listio(int, struct aiocb* const[], int, struct sigevent*);

// for use by wposix_wchar's wopen/wclose:

// (re)open file in asynchronous mode and associate handle with fd.
// (this works because the files default to DENY_NONE sharing)
extern LibError waio_reopen(int fd, const wchar_t* pathname, int oflag, ...);
extern LibError waio_close(int fd);

#endif	// #ifndef INCLUDED_WAIO
