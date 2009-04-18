/**
 * =========================================================================
 * File        : waio.h
 * Project     : 0 A.D.
 * Description : emulate POSIX asynchronous I/O on Windows.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WAIO
#define INCLUDED_WAIO

#include "wposix_types.h"

#include "no_crt_posix.h"


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
// <fcntl.h>
//

// .. Win32-only (not specified by POSIX)
#define O_TEXT_NP      0x4000  // file mode is text (translated)
#define O_BINARY_NP    0x8000  // file mode is binary (untranslated)

// .. wposix.cpp only (bit values not used by MS _open)
#define O_NO_AIO_NP    0x20000
// wposix-specific: do not open a separate AIO-capable handle.
// this is used for small files where AIO overhead isn't worth it,
// thus speeding up loading and reducing resource usage.

// .. not supported by Win32 (bit values not used by MS _open)
#define O_NONBLOCK     0x1000000


// redefinition error here => io.h is getting included somewhere.
// we implement this function, so the io.h definition conflicts if
// compiling against the DLL CRT. either rename the io.h def
// (as with vc_stat), or don't include io.h.
extern int open(const char* fn, int mode, ...);
extern int wopen(const wchar_t* fn, int oflag, ...);
extern int close(int);

//
// <unistd.h>
//

extern int read (int fd, void* buf, size_t nbytes);	// thunk
extern int write(int fd, void* buf, size_t nbytes);	// thunk
extern off_t lseek(int fd, off_t ofs, int whence);  // thunk
LIB_API int truncate(const char* path, off_t length);


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

// allocate and return a file descriptor 
extern int aio_assign_handle(uintptr_t handle);

#endif	// #ifndef INCLUDED_WAIO
