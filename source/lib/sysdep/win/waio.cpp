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

#include "precompiled.h"

#include "lib.h"
#include "win_internal.h"

#include <assert.h>
#include <stdlib.h>
#include <malloc.h>		// _aligned_malloc


#define lock() win_lock(WAIO_CS)
#define unlock() win_unlock(WAIO_CS)


#pragma data_seg(".LIB$WCC")
WIN_REGISTER_FUNC(waio_init);
#pragma data_seg(".LIB$WTX")
WIN_REGISTER_FUNC(waio_shutdown);
#pragma data_seg()


// Win32 functions require sector aligned transfers.
// max of all drives' size is checked in waio_init().
static size_t sector_size = 4096;	// minimum: one page


//////////////////////////////////////////////////////////////////////////////
//
// associate async-capable handle with POSIX file descriptor (int)
//
//////////////////////////////////////////////////////////////////////////////

// current implementation: open file again for async access on each open();
// wastes 1 HANDLE per file, but that's less overhead than storing the
// filename/mode for every file and re-opening that on demand.
//
// note: current Windows lowio file descriptor limit is 2k

static HANDLE* aio_hs;
	// array; expanded when needed in aio_h_set

static int aio_hs_size;
	// often compared against fd => int


// aio_h: no init needed.

static void aio_h_cleanup()
{
	lock();

	for(int i = 0; i < aio_hs_size; i++)
		if(aio_hs[i] != INVALID_HANDLE_VALUE)
		{
			if(!CloseHandle(aio_hs[i]))
				debug_warn("CloseHandle failed");
			aio_hs[i] = INVALID_HANDLE_VALUE;
		}

	free(aio_hs);
	aio_hs = 0;

	aio_hs_size = 0;

	unlock();
}


static bool is_valid_file_handle(const HANDLE h)
{
	bool valid = (GetFileSize(h, 0) != INVALID_FILE_SIZE);
	assert(valid);
	return valid;
}


// return async-capable handle associated with file <fd>
static HANDLE aio_h_get(const int fd)
{
	HANDLE h = INVALID_HANDLE_VALUE;

	lock();

	if(0 <= fd && fd < aio_hs_size)
	{
		h = aio_hs[fd];
		if(!is_valid_file_handle(h))
			h = INVALID_HANDLE_VALUE;
	}
	else
		debug_warn("aio_h_get: fd's aio handle not set");
		// h already INVALID_HANDLE_VALUE

	unlock();

	return h;
}


// associate h (an async-capable file handle) with fd;
// returned by subsequent aio_h_get(fd) calls.
// setting h = INVALID_HANDLE_VALUE removes the association.
static int aio_h_set(const int fd, const HANDLE h)
{
	lock();

	if(fd < 0)
		goto fail;

	// grow hs array to at least fd+1 entries
	if(fd >= aio_hs_size)
	{
		const uint size2 = (uint)round_up(fd+8, 8);
		HANDLE* hs2 = (HANDLE*)realloc(aio_hs, size2*sizeof(HANDLE));
		if(!hs2)
			goto fail;
		// don't assign directly from realloc -
		// we'd leak the previous array if realloc fails.

		for(uint i = aio_hs_size; i < size2; i++)
			hs2[i] = INVALID_HANDLE_VALUE;
		aio_hs = hs2;
		aio_hs_size = size2;
	}

	// nothing to do; will set aio_hs[fd] to INVALID_HANDLE_VALUE below.
	if(h == INVALID_HANDLE_VALUE)
		;
	else
	{
		// already set
		if(aio_hs[fd] != INVALID_HANDLE_VALUE)
			goto fail;
		// setting invalid handle
		if(!is_valid_file_handle(h))
			goto fail;
	}

	aio_hs[fd] = h;

	unlock();
	return 0;

fail:
	unlock();
	debug_warn("aio_h_set failed");
	return -1;
}




// open fn in async mode; associate with fd (retrieve via aio_h(fd))
int aio_reopen(int fd, const char* fn, int oflag, ...)
{
	// interpret oflag
	DWORD access = GENERIC_READ;	// assume O_RDONLY
	DWORD share = FILE_SHARE_READ;
	DWORD create = OPEN_EXISTING;
	if(oflag & O_WRONLY)
	{
		access = GENERIC_WRITE;
		share = FILE_SHARE_WRITE;
	}
	else if(oflag & O_RDWR)
	{
		access |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
	}
	if(oflag & O_CREAT)
		create = (oflag & O_EXCL)? CREATE_NEW : CREATE_ALWAYS;

	// open file
	DWORD flags = FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN;
WIN_SAVE_LAST_ERROR;	// CreateFile
	HANDLE h = CreateFile(fn, access, share, 0, create, flags, 0);
WIN_RESTORE_LAST_ERROR;
	if(h == INVALID_HANDLE_VALUE)
		goto fail;

	if(aio_h_set(fd, h) < 0)
	{
		CloseHandle(h);
		goto fail;
	}

	return 0;

fail:
	debug_warn("aio_reopen failed");
	return -1;
}


int aio_close(int fd)
{
	HANDLE h = aio_h_get(fd);
	if(h == INVALID_HANDLE_VALUE)	// out of bounds or already closed
		goto fail;

	if(!CloseHandle(h))
		goto fail;
	CHECK_ERR(aio_h_set(fd, INVALID_HANDLE_VALUE));

	return 0;

fail:
	debug_warn("aio_close failed");
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
	// used to identify this request; != 0 <==> request valid.
	// set by req_alloc.
	aiocb* cb;

	OVERLAPPED ovl;
		// hEvent signals when transfer complete

	// align buffer - unaligned reads are padded to sector boundaries and
	// go here; the desired data is then copied into the user's buffer.
	// reused, since the Req has global lifetime; resized if too small.
	void* buf;
	size_t buf_size;

	HANDLE hFile;
		// needed to GetOverlappedResult in aio_return

	size_t pad;		// offset from starting sector
	bool read_into_align_buffer;
};


// an aiocb is used to pass the request from caller to aio,
// and serves as a "token" identifying the IO - its address is unique.
// Req holds some state needed for the Windows AIO calls (OVERLAPPED).
//
// cb -> req (e.g. in aio_return) is accomplished by searching reqs
// for the given cb (no problem since MAX_REQS is small).
// req stores a pointer to its associated cb.


const int MAX_REQS = 8;
static Req reqs[MAX_REQS];


static void req_cleanup(void)
{
	Req* r = reqs;

	for(int i = 0; i < MAX_REQS; i++, r++)
	{
		HANDLE& h = r->ovl.hEvent;
		assert(h != INVALID_HANDLE_VALUE);
		CloseHandle(h);
		h = INVALID_HANDLE_VALUE;

		_aligned_free(r->buf);
		r->buf = 0;
	}
}


static void req_init()
{
	for(int i = 0; i < MAX_REQS; i++)
		reqs[i].ovl.hEvent = CreateEvent(0,1,0,0);	// manual reset

	// buffers are allocated on-demand.
}


// return first Req with given cb field
// (0 if searching for a free Req)
static Req* req_find(const aiocb* cb)
{
	Req* r = reqs;
	for(int i = 0; i < MAX_REQS; i++, r++)
		if(r->cb == cb)
			return r;

	// not found
	return 0;
}


static Req* req_alloc(aiocb* cb)
{
	assert(cb);

	// first free Req, or 0
	Req* r = req_find(0);
	// .. found one: mark it in-use
	if(r)
		r->cb = cb;

	return r;
}


static int req_free(Req* r)
{
	assert(r->cb != 0 && "req_free: not currently in use");
	r->cb = 0;
	return 0;
}




// called by aio_read, aio_write, and lio_listio
// cb->aio_lio_opcode specifies desired operation
//
// if cb->aio_fildes doesn't support seeking (e.g. a socket),
// cb->aio_offset must be 0.
static int aio_rw(struct aiocb* cb)
{
	int ret = -1;
	Req* r = 0;

	WIN_SAVE_LAST_ERROR;

	// no-op from lio_listio
	if(!cb || cb->aio_lio_opcode == LIO_NOP)
		return 0;

	// fail if aiocb is already in use (forbidden by SUSv3)
	if(req_find(cb))
	{
		debug_warn("aio_rw: aiocb is already in use");
		goto fail;
	}

	// extract aiocb fields for convenience
	const bool is_write = (cb->aio_lio_opcode == LIO_WRITE);
	const int fd        = cb->aio_fildes;
	const size_t size   = cb->aio_nbytes;
	const off_t ofs     = cb->aio_offset;
	void* buf     = (void*)cb->aio_buf; // from volatile void*
	assert(buf);

	// allocate IO request
	r = req_alloc(cb);
	if(!r)
	{
		debug_warn("aio_rw: cannot allocate a Req (too many concurrent IOs)");
		goto fail;
	}

	HANDLE h = aio_h_get(fd);
	if(h == INVALID_HANDLE_VALUE)
	{
		debug_warn("aio_rw: associated handle is invalid");
		ret = -EINVAL;
		goto fail;
	}


	r->hFile = h;
	r->pad = 0;
	r->read_into_align_buffer = false;


	//
	// align
	//

	// actual transfer parameters
	// (possibly rounded up/down to satisfy Win32 alignment requirements)
	size_t actual_ofs = 0;
		// assume socket; if file, set below
	size_t actual_size = size;
	void* actual_buf = buf;

	// leave offset 0 if h is a socket (don't support seeking);
	// otherwise, calculate aligned offset/size
	const bool is_file = (GetFileType(h) == FILE_TYPE_DISK);
	if(is_file)
	{
		// round offset down to start of previous sector, and total
		// transfer size up to an integral multiple of sector_size.
		r->pad = ofs % sector_size;
		actual_ofs = ofs - r->pad;
		actual_size = round_up(size + r->pad, sector_size);

		// now decide if any of the original parameters was unaligned,
		// and whether it was ofs or buf in particular
		// (needed for unaligned write handling below).
		const bool ofs_misaligned = r->pad != 0;
		const bool buf_misaligned = (uintptr_t)buf % sector_size != 0;
		const bool misaligned = ofs_misaligned || buf_misaligned || actual_size != size;
			// note: actual_size != size if ofs OR size is unaligned

		// misaligned => will need to go through align buffer
		// (we fail some types of misalignment for convenience; see below).
		if(misaligned)
		{
			// expand current align buffer if too small
			if(r->buf_size < actual_size)
			{
				void* buf2 = _aligned_realloc(r->buf, actual_size, sector_size);
				if(!buf2)
				{
					ret = -ENOMEM;
					goto fail;
				}
				r->buf = buf2;
				r->buf_size = actual_size;
			}

			if(!is_write)
			{
				actual_buf = r->buf;
				r->read_into_align_buffer = true;
			}
			else
			{
				// unaligned offset: not supported.
				// (we'd have to read padding, then write our data. ugh.)
				if(ofs_misaligned)
				{
					ret = -EINVAL;
					goto fail;
				}

				// unaligned buffer: copy to align buffer and write from there.
				if(buf_misaligned)
				{
					memcpy(r->buf, buf, size);
					memset((char*)r->buf + size, 0, actual_size - size);
						// clear previous contents at end of align buf
					actual_buf = r->buf;
				}

				// unaligned size: already taken care of (we round up)
			}
		}	// misaligned
	}	// is_file

	// set OVERLAPPED fields
	ResetEvent(r->ovl.hEvent);
	r->ovl.Internal = r->ovl.InternalHigh = 0;
	*(size_t*)&r->ovl.Offset = actual_ofs;
		// HACK: use this instead of OVERLAPPED.Pointer,
		// which isn't defined in older headers (e.g. VC6).
		// 64-bit clean, but endian dependent!

	DWORD size32 = (DWORD)(actual_size & 0xffffffff);
	BOOL ok;
	if(is_write)
		ok = WriteFile(h, actual_buf, size32, 0, &r->ovl);
	else
		ok =  ReadFile(h, actual_buf, size32, 0, &r->ovl);		

	// "pending" isn't an error
	if(GetLastError() == ERROR_IO_PENDING)
		ok = true;

	if(ok)
		ret = 0;

done:
	WIN_RESTORE_LAST_ERROR;

	return ret;

fail:
	req_free(r);
	goto done;
}


// return status of transfer
int aio_error(const struct aiocb* cb)
{
	// must not pass 0 to req_find - we'd look for a free cb!
	if(!cb)
	{
		debug_warn("aio_error: invalid cb");
		return -1;
	}

	Req* r = req_find(cb);
	if(!r)
		return -1;

	switch(r->ovl.Internal)	// I/O status
	{
	case 0:
		return 0;
	case STATUS_PENDING:
		return EINPROGRESS;
	default:
		return -1;
	}
}


// get bytes transferred. call exactly once for each op.
ssize_t aio_return(struct aiocb* cb)
{
	// must not pass 0 to req_find - we'd look for a free cb!
	if(!cb)
	{
		debug_warn("aio_return: invalid cb");
		return -1;
	}

	Req* r = req_find(cb);
	if(!r)
	{
		debug_warn("aio_return: cb not found (already called aio_return?)");
		return -1;
	}

	assert(r->ovl.Internal == 0 && "aio_return with transfer in progress");

	const BOOL wait = FALSE;	// should already be done!
	DWORD bytes_transferred;
	if(!GetOverlappedResult(r->hFile, &r->ovl, &bytes_transferred, wait))
	{
		debug_warn("aio_return: GetOverlappedResult failed");
		return -1;
	}

	// we read into align buffer - copy to user's buffer
	if(r->read_into_align_buffer)
		memcpy((void*)cb->aio_buf, (u8*)r->buf + r->pad, cb->aio_nbytes);

	// TODO: this copies data back into original buffer from align buffer
	// when writing from unaligned buffer. unnecessarily slow.

	req_free(r);

	return (ssize_t)bytes_transferred;
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

		Req* r = req_find(cbs[i]);
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

	const BOOL wait_all = FALSE;
	DWORD result = WaitForMultipleObjects(cnt, hs, wait_all, timeout);

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


int aio_cancel(int fd, struct aiocb* cb)
{
	// Win32 limitation: can't cancel single transfers -
	// all pending reads on this file are cancelled.
	UNUSED(cb);

	const HANDLE h = aio_h_get(fd);
	if(h == INVALID_HANDLE_VALUE)
		return -1;

	CancelIo(h);
	return AIO_CANCELED;
}




int aio_read(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_READ;
	return aio_rw(cb);	// checks for cb == 0
}


int aio_write(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_WRITE;
	return aio_rw(cb);	// checks for cb == 0
}


int lio_listio(int mode, struct aiocb* const cbs[], int n, struct sigevent* se)
{
	UNUSED(se);

	int err = 0;

	for(int i = 0; i < n; i++)
	{
		int ret = aio_rw(cbs[i]);		// checks for cbs[i] == 0
		// don't CHECK_ERR - want to try to issue each one
		if(ret < 0 && !err)
			err = ret;
	}

	CHECK_ERR(err);

	if(mode == LIO_WAIT)
		return aio_suspend(cbs, n, 0);

	return 0;
}


int aio_fsync(int, struct aiocb*)
{
	return -ENOSYS;
}


//////////////////////////////////////////////////////////////////////////////
//
// init / cleanup
//
//////////////////////////////////////////////////////////////////////////////


static int waio_init()
{
	req_init();

	const UINT old_err_mode = SetErrorMode(SEM_FAILCRITICALERRORS);

	// Win32 requires transfers to be sector aligned.
	// find maximum of all drive's sector sizes, then use that.
	// (it's good to know this up-front, and checking every open() is slow).
	const DWORD drives = GetLogicalDrives();
	char drive_str[4] = "?:\\";
	for(int drive = 2; drive <= 26; drive++)	// C: .. Z:
	{
		// avoid BoundsChecker warning by skipping invalid drives
		if(!(drives & BIT(drive)))
			continue;

		drive_str[0] = (char)('A'+drive);

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

	SetErrorMode(old_err_mode);

	assert(is_pow2((long)sector_size));

	return 0;
}


static int waio_shutdown()
{
	req_cleanup();
	aio_h_cleanup();
	return 0;
}
