// POSIX asynchronous I/O for Win32
//
// Copyright (c) 2003-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

// included by wposix.h and waio.cpp

#ifndef WAIO_H__
#define WAIO_H__

#include "lib/types.h"
#include "wposix_types.h"


// Note: for maximum efficiency, transfer buffers, offsets, and lengths
// should be sector aligned (otherwise, buffer is copied).


//
// <signal.h>
//

union sigval
{
	int sival_int;				// Integer signal value.
	void* sival_ptr;			// Pointer signal value.
};

struct sigevent
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
	int             aio_reqprio;    // Request priority offset.
	struct sigevent aio_sigevent;   // Signal number and value.
	int             aio_lio_opcode; // Operation to be performed.
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
extern int aio_suspend(const struct aiocb* const[], int, const struct timespec*);
extern int aio_write(struct aiocb*);
extern int lio_listio(int, struct aiocb* const[], int, struct sigevent*);

extern int aio_close(int fd);
extern int aio_reopen(int fd, const char* fn, int oflag, ...);

// allocate and return a file descriptor 
extern int aio_assign_handle(uintptr_t handle);

#endif	// #ifndef WAIO_H__
