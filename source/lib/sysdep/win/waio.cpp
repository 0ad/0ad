// POSIX asynchronous I/O for Win32
//
// Copyright (c) 2003 Jan Wassenberg
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

#include <cassert>
#include <cstdlib>
#include <cstdio>

#include <io.h>

#include "lib.h"
#include "win_internal.h"
#include "misc.h"
#include "types.h"


//////////////////////////////////////////////////////////////////////////////
//
// AioHandles
//
//////////////////////////////////////////////////////////////////////////////

// async-capable handles to each lowio file
// current implementation: open both versions of the file on open()
//   wastes 1 handle/file, but we don't have to remember the filename/mode
//
// note: current Windows lowio handle limit is 2k

class AioHandles
{
public:
	AioHandles() : hs(0), size(0) {}
	~AioHandles();

	HANDLE get(int fd);
	int set(int fd, HANDLE h);

private:
	HANDLE* hs;
	uint size;
};


AioHandles::~AioHandles()
{
	win_lock(WAIO_CS);

	for(int i = 0; (unsigned)i < size; i++)
		if(hs[i] != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hs[i]);
			hs[i] = INVALID_HANDLE_VALUE;
		}

	free(hs);
	hs = 0;
	size = 0;

	win_unlock(WAIO_CS);
}


bool is_valid_file_handle(HANDLE h)
{
	SetLastError(0);
	bool valid = (GetFileSize(h, 0) != INVALID_FILE_SIZE);
	assert(valid);
	return valid;
}

// get async capable handle to file <fd>
HANDLE AioHandles::get(int fd)
{
	HANDLE h = INVALID_HANDLE_VALUE;

	win_lock(WAIO_CS);

	if((unsigned)fd < size)
		h = hs[fd];
	else
		assert(0);

	if(!is_valid_file_handle(h))
		return INVALID_HANDLE_VALUE;

	win_unlock(WAIO_CS);

	return h;
}


int AioHandles::set(int fd, HANDLE h)
{
	win_lock(WAIO_CS);

	// grow hs array to at least fd+1 entries
	if((unsigned)fd >= size)
	{
		uint size2 = (uint)round_up(fd+8, 8);
		HANDLE* hs2 = (HANDLE*)realloc(hs, size2*sizeof(HANDLE));
		if(!hs2)
			goto fail;

        for(uint i = size; i < size2; i++)
			hs2[i] = INVALID_HANDLE_VALUE;
		hs = hs2;
		size = size2;
	}

	if(h == INVALID_HANDLE_VALUE)
		;
	else
	{
		if(hs[fd] != INVALID_HANDLE_VALUE)
		{
			assert("AioHandles::set: handle already set!");
			goto fail;
		}
		if(!is_valid_file_handle(h))
		{
			assert("AioHandles::set: setting invalid handle");
			goto fail;
		}
	}

	hs[fd] = h;

	win_unlock(WAIO_CS);
	return 0;

fail:
	win_unlock(WAIO_CS);
	assert(0 && "AioHandles::set failed");
	return -1;
}


//////////////////////////////////////////////////////////////////////////////
//
// Req
//
//////////////////////////////////////////////////////////////////////////////

// information about active transfers (reused)
struct Req
{
	// used to identify this request; != 0 <==> request valid
	aiocb* cb;

	OVERLAPPED ovl;
	// hEvent signals when transfer complete

	// read into a separate align buffer if necessary
	// (note: unaligned writes aren't supported. see aio_rw)
	size_t pad;		// offset from starting sector
	void* buf;		// reused; resize if too small
	size_t buf_size;
};

class Reqs
{
public:
	Reqs();
	~Reqs();
	Req* find(const aiocb* cb);
	Req* alloc(const aiocb* cb);

private:
	enum { MAX_REQS = 8 };
	Req reqs[MAX_REQS];
};


Reqs::Reqs()
{
	for(int i = 0; i < MAX_REQS; i++)
	{
		memset(&reqs[i], 0, sizeof(Req));
		reqs[i].ovl.hEvent = CreateEvent(0,1,0,0);	// manual reset
	}
}


Reqs::~Reqs()
{
	Req* r = reqs;
	for(int i = 0; i < MAX_REQS; i++, r++)
	{
		r->cb = 0;

		HANDLE& h = r->ovl.hEvent;
		if(h != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}

		free(r->buf);
		r->buf = 0;
	}
}


// find request slot currently in use by cb
// cb = 0 => search for empty slot
Req* Reqs::find(const aiocb* cb)
{
	Req* r = reqs;

	for(int i = 0; i < MAX_REQS; i++, r++)
		if(r->cb == cb)
			return r;

	assert(0 && "Reqs::find failed");
	return 0;
}


Req* Reqs::alloc(const aiocb* cb)
{
	win_lock(WAIO_CS);

	// find free request slot
	Req* r = find(0);
	if(!r)
	{
		win_unlock(WAIO_CS);
		assert(0 && "Reqs::alloc: no request slot available!");
		return 0;
	}
	r->cb = (aiocb*)cb;

	win_unlock(WAIO_CS);

	return r;
}


//////////////////////////////////////////////////////////////////////////////
//
// init / cleanup
//
//////////////////////////////////////////////////////////////////////////////

static AioHandles* aio_hs;
static Reqs* reqs;


// Win32 functions require sector aligned transfers.
// max of all drives' size is checked in init().
static size_t sector_size = 4096;	// minimum: one page


static void cleanup(void)
{
	delete aio_hs;
	delete reqs;
}


// caller ensures this is not re-entered!
static void init()
{
	reqs = new Reqs;
	aio_hs = new AioHandles;

	atexit(cleanup);

	// Win32 requires transfers to be sector aligned.
	// find maximum of all drive's sector sizes, then use that.
	// (it's good to know this up-front, and checking every open() is slow).
	DWORD drives = GetLogicalDrives();
	char drive_str[4] = "?:\\";
	for(int drive = 2; drive <= 26; drive++)	// C: .. Z:
	{
		// avoid BoundsChecker warning by skipping invalid drives
		if(!(drives & (1ul << drive)))
			continue;

		drive_str[0] = 'A'+drive;

		DWORD spc, nfc, tnc;	// don't need these
		DWORD sector_size2;
		if(GetDiskFreeSpace(drive_str, &spc, &sector_size2, &nfc, &tnc))
		{
			if(sector_size < sector_size2)
				sector_size = sector_size2;
		}
		// otherwise, it's probably an empty CD drive. ignore the
		// BoundsChecker error; GetDiskFreeSpace seems to be the
		// only way of getting at the sector size.
	}
	assert(is_pow2((long)sector_size));
}




int aio_assign_handle(uintptr_t handle)
{
	// 
	// CRT stores osfhandle. if we pass an invalid handle (say, 0),
	// we get an exception when closing the handle if debugging.
	// events can be created relatively quickly (~1800 clocks = 1µs),
	// and are also freed with CloseHandle, so just pass that.
	HANDLE h = CreateEvent(0,0,0,0);
	if(h == INVALID_HANDLE_VALUE)
	{
		assert(0 && "aio_assign_handle failed");
		return -1;
	}

	int fd = _open_osfhandle((intptr_t)h, 0);
	if(fd < 0)
	{
		assert(0 && "aio_assign_handle failed");
		return fd;
	}

	return aio_hs->set(fd, (HANDLE)handle);
}




// open fn in async mode; associate with fd (retrieve via aio_h(fd))
int aio_open(const char* fn, int mode, int fd)
{
WIN_ONCE(init());	// TODO: need to do this elsewhere in case other routines called first?

	// interpret mode
	DWORD access = GENERIC_READ;	// assume O_RDONLY
	DWORD share = 0;
	if(mode & O_WRONLY)
		access = GENERIC_WRITE;
	else if(mode & O_RDWR)
		access = GENERIC_READ|GENERIC_WRITE;
	else
		share = FILE_SHARE_READ;
	DWORD create = OPEN_EXISTING;
	if(mode & O_CREAT)
		create = (mode & O_EXCL)? CREATE_NEW : CREATE_ALWAYS;
	DWORD flags = FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN;

	// open file
	HANDLE h = CreateFile(fn, access, share, 0, create, flags, 0);
	if(h == INVALID_HANDLE_VALUE)
	{
		assert(0 && "aio_open failed");
		return -1;
	}

	if(aio_hs->set(fd, h) < 0)
	{
		assert(0 && "aio_open failed");
		CloseHandle(h);
		return -1;
	}

	return 0;
}


int aio_close(int fd)
{
	HANDLE h = aio_hs->get(fd);
	if(h == INVALID_HANDLE_VALUE)	// out of bounds or already closed
	{
		assert(0 && "aio_close failed");
		return -1;
	}

	SetLastError(0);
	if(!CloseHandle(h))
		assert(0);
	aio_hs->set(fd, INVALID_HANDLE_VALUE);
	return 0;
}




// called by aio_read, aio_write, and lio_listio
// cb->aio_lio_opcode specifies desired operation
//
// if cb->aio_fildes doesn't support seeking (e.g. a socket),
// cb->aio_offset must be 0.
static int aio_rw(struct aiocb* cb)
{
	if(!cb)
	{
		assert(0);
		return -EINVAL;
	}
	if(cb->aio_lio_opcode == LIO_NOP)
	{
		assert(0);
		return 0;
	}

	HANDLE h = aio_hs->get(cb->aio_fildes);
	if(h == INVALID_HANDLE_VALUE)
	{
		assert(0 && "aio_rw: associated handle is invalid");
		return -EINVAL;
	}

	Req* r = reqs->alloc(cb);
	if(!r)
	{
		assert(0);
		return -1;
	}

	size_t ofs = 0;
	size_t size = cb->aio_nbytes;
	void* buf = cb->aio_buf;

#define SOL_SOCKET 0xffff
#define SO_TYPE 0x1008

	unsigned long opt = 0;
	socklen_t optlen = sizeof(opt);
	if (getsockopt((int)(intptr_t)h, SOL_SOCKET, SO_TYPE, &opt, &optlen) != -1)
//		||	(WSAGetLastError() != WSAENOTSOCK))
		cb->aio_offset = 0;
	else
	{
		// calculate alignment
		r->pad = cb->aio_offset % sector_size;		// offset to start of sector
		ofs = cb->aio_offset - r->pad;
		size += r->pad + sector_size-1;
		size &= ~(sector_size-1);	// align (sector_size = 2**n)

		// not aligned
		if(r->pad || (uintptr_t)buf % sector_size)
		{
			// current align buffer is too small - resize
			if(r->buf_size < size)
			{
				void* buf2 = realloc(r->buf, size);
				if(!buf2)
					return -ENOMEM;
				r->buf = buf2;
				r->buf_size = size;
			}

			// unaligned writes are not supported -
			// we'd have to read padding, then write our data. ugh.
			if(cb->aio_lio_opcode == LIO_WRITE)
			{
				return -EINVAL;
			}

			buf = r->buf;
		}
	}

	r->ovl.Internal = r->ovl.InternalHigh = 0;

#if _MSC_VER >= 1300
	r->ovl.Pointer = (void*)ofs;
#else
	r->ovl.Offset = ofs;
#endif

assert(cb->aio_buf != 0);

	SetLastError(0);

	DWORD size32 = (DWORD)(size & 0xffffffff);
ResetEvent(r->ovl.hEvent);
	BOOL ok = (cb->aio_lio_opcode == LIO_READ)?
		ReadFile(h, buf, size32, 0, &r->ovl) : WriteFile(h, buf, size32, 0, &r->ovl);

	if(ok || GetLastError() == ERROR_IO_PENDING)
		return 0;
	return -1;
}


int aio_read(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_READ;
	return aio_rw(cb);
}


int aio_write(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_WRITE;
	return aio_rw(cb);
}


int lio_listio(int mode, struct aiocb* const cbs[], int n, struct sigevent* se)
{
	UNUSED(se)

	int err = 0;

	for(int i = 0; i < n; i++)
	{
		int ret = aio_rw(cbs[i]);		// aio_rw checks for 0 param
		if(ret < 0)
			err = ret;
	}

	if(err < 0)
		return err;

	if(mode == LIO_WAIT)
		return aio_suspend(cbs, n, 0);

	return 0;
}


// return status of transfer
int aio_error(const struct aiocb* cb)
{
	Req* const r = reqs->find(cb);
	if(!r)
		return -1;

	switch(r->ovl.Internal)	// I/O status
	{
	case 0:
		return 0;
	case STATUS_PENDING:
		return EINPROGRESS;

	// TODO: errors
	default:
		return -1;
	}
}


// get bytes transferred. call exactly once for each op.
ssize_t aio_return(struct aiocb* cb)
{
	Req* const r = reqs->find(cb);
	if(!r)
		return -1;

	assert(r->ovl.Internal == 0 && "aio_return with transfer in progress");

	// read wasn't aligned - need to copy to user's buffer
	const size_t _buf = (char*)cb->aio_buf - (char*)0;
	if(r->pad || _buf % sector_size)
		memcpy(cb->aio_buf, (u8*)r->buf + r->pad, cb->aio_nbytes);

	// free this request slot
	r->cb = 0;

	return (ssize_t)cb->aio_nbytes;
}


int aio_cancel(int fd, struct aiocb* cb)
{
	UNUSED(cb)

	const HANDLE h = aio_hs->get(fd);
	if(h == INVALID_HANDLE_VALUE)
		return -1;

	// Win32 limitation: can't cancel single transfers -
	// all pending reads on this file are cancelled.
	CancelIo(h);
	return AIO_CANCELED;
}


int aio_fsync(int, struct aiocb*)
{
	return -1;
}


int aio_suspend(const struct aiocb* const cbs[], int n, const struct timespec* ts)
{
	int i;

	if(n <= 0 || n > MAXIMUM_WAIT_OBJECTS)
		return -1;

	int cnt = 0;	// actual number of valid cbs
	HANDLE hs[MAXIMUM_WAIT_OBJECTS];

	for(i = 0; i < n; i++)
	{
		// ignore NULL list entries
		if(!cbs[i])
			continue;

		Req* r = reqs->find(cbs[i]);
		if(r)
		{
			if(r->ovl.Internal == STATUS_PENDING)
				hs[cnt++] = r->ovl.hEvent;
		}
	}

	// no valid, pending transfers - done
	if(!cnt)
		return 0;

	// timeout: convert timespec to ms (NULL ptr -> no timeout)
	DWORD timeout = INFINITE;
	if(ts)
		timeout = (DWORD)(ts->tv_sec*1000 + ts->tv_nsec/1000000);

	DWORD result = WaitForMultipleObjects(cnt, hs, 0, timeout);
	for(i = 0; i < cnt; i++)
		ResetEvent(hs[i]);

	if(result == WAIT_TIMEOUT)
	{
		//errno = -EAGAIN;
		return -1;
	}
	else
		return (result == WAIT_FAILED)? -1 : 0;
}
