/**
 * =========================================================================
 * File        : waio.cpp
 * Project     : 0 A.D.
 * Description : emulate POSIX asynchronous I/O on Windows.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "waio.h"

#include <malloc.h>         // _aligned_malloc
#include "crt_posix.h"      // correct definitions of _open() etc.

#include "wposix_internal.h"
#include "wfilesystem.h"    // mode_t
#include "wtime.h"          // timespec
#include "lib/sysdep/cpu.h"
#include "lib/bits.h"


#pragma SECTION_INIT(5)
WINIT_REGISTER_FUNC(waio_init);
#pragma FORCE_INCLUDE(waio_init)
#pragma SECTION_SHUTDOWN(5)
WINIT_REGISTER_FUNC(waio_shutdown);
#pragma FORCE_INCLUDE(waio_shutdown)
#pragma SECTION_RESTORE


static void lock()
{
	win_lock(WAIO_CS);
}

static void unlock()
{
	win_unlock(WAIO_CS);
}


// return the largest sector size [bytes] of any storage medium
// (HD, optical, etc.) in the system.
//
// this may be a bit slow to determine (iterates over all drives),
// but caches the result so subsequent calls are free.
// (caveat: device changes won't be noticed during this program run)
//
// sector size is relevant because Windows aio requires all IO
// buffers, offsets and lengths to be a multiple of it. this requirement
// is also carried over into the vfs / file.cpp interfaces for efficiency
// (avoids the need for copying to/from align buffers).
//
// waio uses the sector size to (in some cases) align IOs if
// they aren't already, but it's also needed by user code when
// aligning their buffers to meet the requirements.
//
// the largest size is used so that we can read from any drive. while this
// is a bit wasteful (more padding) and requires iterating over all drives,
// it is the only safe way: this may be called before we know which
// drives will be needed, and hardlinks may confuse things.
size_t sys_max_sector_size()
{
	// users may call us more than once, so cache the results.
	static DWORD cached_sector_size;
	if(cached_sector_size)
		return static_cast<size_t>(cached_sector_size);
	
			// currently disabled: DVDs have 2..4KB, but this causes
			// waio to unnecessarily align some file transfers (when at EOF)
			// this means that we might not be able to read from CD/DVD drives
			// (ReadFile will return error)
	// reactivated for correctness.

	// temporarily disable the "insert disk into drive" error box; we are
	// only interested in fixed drives anyway.
	//
	// note: use SetErrorMode (crappy interface, grr) twice so as not to
	// stomp on other flags (e.g. alignment exception).
	const UINT old_err_mode = SetErrorMode(0);
	SetErrorMode(old_err_mode|SEM_FAILCRITICALERRORS);

	// find maximum of all drive's sector sizes.
	const DWORD drives = GetLogicalDrives();
	char drive_str[4] = "?:\\";
	for(int drive = 2; drive <= 25; drive++)	// C: .. Z:
	{
		// avoid BoundsChecker warning by skipping invalid drives
		if(!(drives & BIT(drive)))
			continue;

		drive_str[0] = (char)('A'+drive);

		DWORD spc, nfc, tnc;	// don't need these
		DWORD cur_sector_size;
		if(GetDiskFreeSpace(drive_str, &spc, &cur_sector_size, &nfc, &tnc))
			cached_sector_size = std::max(cached_sector_size, cur_sector_size);
		// otherwise, it's probably an empty CD drive. ignore the
		// BoundsChecker error; GetDiskFreeSpace seems to be the
		// only way of getting at the sector size.
	}

	SetErrorMode(old_err_mode);

	// sanity check; believed to be the case for all drives.
	debug_assert(cached_sector_size % 512 == 0);

	return cached_sector_size;
}


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
	{
		if(aio_hs[i] != INVALID_HANDLE_VALUE)
		{
			WARN_IF_FALSE(CloseHandle(aio_hs[i]));
			aio_hs[i] = INVALID_HANDLE_VALUE;
		}
	}

	SAFE_FREE(aio_hs);
	aio_hs_size = 0;

	unlock();
}


static bool is_valid_file_handle(const HANDLE h)
{
	const bool valid = (GetFileSize(h, 0) != INVALID_FILE_SIZE);
	if(!valid)
		debug_warn("waio: invalid file handle");
	return valid;
}


// return true iff an aio-capable HANDLE has been attached to <fd>.
// used by aio_close.
static bool aio_h_is_set(const int fd)
{
	lock();
	bool is_set = (0 <= fd && fd < aio_hs_size && aio_hs[fd] != INVALID_HANDLE_VALUE);
	unlock();
	return is_set;
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
		// h is already INVALID_HANDLE_VALUE

	unlock();

	return h;
}


// associate h (an async-capable file handle) with fd;
// returned by subsequent aio_h_get(fd) calls.
// setting h = INVALID_HANDLE_VALUE removes the association.
static LibError aio_h_set(const int fd, const HANDLE h)
{
	if(fd < 0)
		WARN_RETURN(ERR::INVALID_PARAM);

	lock();

	LibError err;

	// grow hs array to at least fd+1 entries
	if(fd >= aio_hs_size)
	{
		const uint size2 = (uint)round_up(fd+8, 8);
		HANDLE* hs2 = (HANDLE*)realloc(aio_hs, size2*sizeof(HANDLE));
		if(!hs2)
		{
			err  = ERR::NO_MEM;
			goto fail;
		}
		// don't assign directly from realloc -
		// we'd leak the previous array if realloc fails.

		for(uint i = aio_hs_size; i < size2; i++)
			hs2[i] = INVALID_HANDLE_VALUE;
		aio_hs = hs2;
		aio_hs_size = size2;
	}


	if(h == INVALID_HANDLE_VALUE)
	{
		// nothing to do; will set aio_hs[fd] to INVALID_HANDLE_VALUE below.
	}
	else
	{
		// already set
		if(aio_hs[fd] != INVALID_HANDLE_VALUE)
		{
			err = ERR::LOGIC;
			goto fail;
		}
		// setting invalid handle
		if(!is_valid_file_handle(h))
		{
			err = ERR::INVALID_HANDLE;
			goto fail;
		}
	}

	aio_hs[fd] = h;

	unlock();
	return INFO::OK;

fail:
	unlock();
	WARN_RETURN(err);
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
	debug_warn("failed");
	return -1;
}


int aio_close(int fd)
{
	// early out for files that were never re-opened for AIO.
	// since there is no way for wposix close to know this, we mustn't
	// return an error (which would cause it to WARN_ERR).
	if(!aio_h_is_set(fd))
		return 0;

	HANDLE h = aio_h_get(fd);
	// out of bounds or already closed
	if(h == INVALID_HANDLE_VALUE)
		goto fail;

	if(!CloseHandle(h))
		goto fail;
	RETURN_ERR(aio_h_set(fd, INVALID_HANDLE_VALUE));

	return 0;

fail:
	debug_warn("failed");
	return -1;
}




// do we want to open a second AIO-capable handle?
static bool isAioPossible(int fd, bool is_com_port, int oflag)
{
	// stdin/stdout/stderr
	if(fd <= 2)
		return false;

	// COM port - we don't currently need AIO access for those, and
	// aio_reopen's CreateFile would fail with "access denied".
	if(is_com_port)
		return false;

	// caller is requesting we skip it (see file_open)
	if(oflag & O_NO_AIO_NP)
		return false;

	return true;
}

int open(const char* fn, int oflag, ...)
{
	const bool is_com_port = strncmp(fn, "/dev/tty", 8) == 0;
	// also used later, before aio_reopen

	// translate "/dev/tty%d" to "COM%d"
	if(is_com_port)
	{
		char port[] = "COM1";
		const char digit = fn[8]+1;
		// PCs only support COM1..COM4.
		if(!('1' <= digit && digit <= '4'))
			return -1;
		port[3] = digit;
		fn = port;
	}

	mode_t mode = 0;
	if(oflag & O_CREAT)
	{
		va_list args;
		va_start(args, oflag);
		mode = va_arg(args, mode_t);
		va_end(args);
	}

	WIN_SAVE_LAST_ERROR;	// CreateFile
	int fd = _open(fn, oflag, mode);
	WIN_RESTORE_LAST_ERROR;

	// none of the above apply; now re-open the file.
	// note: this is possible because _open defaults to DENY_NONE sharing.
	if(isAioPossible(fd, is_com_port, oflag))
		WARN_ERR(aio_reopen(fd, fn, oflag));

	// CRT doesn't like more than 255 files open.
	// warn now, so that we notice why so many are open.
#ifndef NDEBUG
	if(fd > 256)
		WARN_ERR(ERR::LIMIT);
#endif

	return fd;
}


int close(int fd)
{
	debug_assert(3 <= fd && fd < 256);

	// note: there's no good way to notify us that <fd> wasn't opened for
	// AIO, so we could skip aio_close. storing a bit in the fd is evil and
	// a fd -> info map is redundant (waio already has one).
	// therefore, we require aio_close to fail gracefully.
	WARN_ERR(aio_close(fd));

	return _close(fd);
}


// we don't want to #define read to _read, since that's a fairly common
// identifier. therefore, translate from MS CRT names via thunk functions.
// efficiency is less important, and the overhead could be optimized away.

int read(int fd, void* buf, size_t nbytes)
{
	return _read(fd, buf, (int)nbytes);
}

int write(int fd, void* buf, size_t nbytes)
{
	return _write(fd, buf, (int)nbytes);
}

off_t lseek(int fd, off_t ofs, int whence)
{
	return _lseek(fd, ofs, whence);
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
		debug_assert(h != INVALID_HANDLE_VALUE);
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
	debug_assert(cb);

	// first free Req, or 0
	Req* r = req_find(0);
	// .. found one: mark it in-use
	if(r)
		r->cb = cb;

	return r;
}


static LibError req_free(Req* r)
{
	debug_assert(r->cb != 0 && "req_free: not currently in use");
	r->cb = 0;
	return INFO::OK;
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
		debug_warn("aiocb is already in use");
		goto fail;
	}

	// extract aiocb fields for convenience
	const bool is_write = (cb->aio_lio_opcode == LIO_WRITE);
	const int fd        = cb->aio_fildes;
	const size_t size   = cb->aio_nbytes;
	const off_t ofs     = cb->aio_offset;
	void* buf     = (void*)cb->aio_buf; // from volatile void*
	debug_assert(buf);

	// allocate IO request
	r = req_alloc(cb);
	if(!r)
	{
		debug_warn("cannot allocate a Req (too many concurrent IOs)");
		goto fail;
	}

	HANDLE h = aio_h_get(fd);
	if(h == INVALID_HANDLE_VALUE)
	{
		debug_warn("associated handle is invalid");
		ret = -EINVAL;
		goto fail;
	}


	r->hFile = h;
	r->pad = 0;
	r->read_into_align_buffer = false;


	//
	// align
	//

	// Win32 requires transfers to be sector aligned.
	// we check if the transfer is aligned to sector size (the max of
	// all drives in the system) and copy to/from align buffer if not.

	// actual transfer parameters (possibly rounded up/down)
	size_t actual_ofs = 0;
		// assume socket; if file, set below
	size_t actual_size = size;
	void* actual_buf = buf;

	const size_t sector_size = sys_max_sector_size();

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
				// unaligned write offset: not supported.
				// (we'd have to read padding, then write our data. ugh.)
				if(ofs_misaligned)
				{
					ret = -EINVAL;
					goto fail;
				}

				// unaligned buffer: copy to align buffer and write from there.
				if(buf_misaligned)
				{
					cpu_memcpy(r->buf, buf, size);
					actual_buf = r->buf;
					// clear previous contents at end of align buf
					memset((char*)r->buf + size, 0, actual_size - size);
				}

				// unaligned size: already taken care of (we round up)
			}
		}	// misaligned
	}	// is_file

	// set OVERLAPPED fields
	// note: Read-/WriteFile reset ovl.hEvent - no need to do that.
	r->ovl.Internal = r->ovl.InternalHigh = 0;
	// note: OVERLAPPED.Pointer is more convenient but not defined on VC6.
	r->ovl.Offset     = u64_lo(actual_ofs);
	r->ovl.OffsetHigh = u64_hi(actual_ofs);

	DWORD size32 = (DWORD)(actual_size & 0xFFFFFFFF);
	BOOL ok;

	DWORD bytes_transferred;
	if(is_write)
		ok = WriteFile(h, actual_buf, size32, &bytes_transferred, &r->ovl);
	else
		ok =  ReadFile(h, actual_buf, size32, &bytes_transferred, &r->ovl);		

	// check result.
	// .. "pending" isn't an error
	if(!ok && GetLastError() == ERROR_IO_PENDING)
		ok = TRUE;
	// .. translate from Win32 result code to POSIX
	LibError err = LibError_from_win32(ok);
	if(err == INFO::OK)
		ret = 0;
	LibError_set_errno(err);

done:
	WIN_RESTORE_LAST_ERROR;

	return ret;

fail:
	debug_warn("waio failure");
	req_free(r);
	goto done;
}


// return status of transfer
int aio_error(const struct aiocb* cb)
{
	// must not pass 0 to req_find - we'd look for a free cb!
	if(!cb)
	{
		debug_warn("invalid cb");
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
		debug_warn("invalid cb");
		return -1;
	}

	Req* r = req_find(cb);
	if(!r)
	{
		debug_warn("cb not found (already called aio_return?)");
		return -1;
	}

	debug_assert(r->ovl.Internal == 0 && "aio_return with transfer in progress");

	const BOOL wait = FALSE;	// should already be done!
	DWORD bytes_transferred;
	if(!GetOverlappedResult(r->hFile, &r->ovl, &bytes_transferred, wait))
	{
		debug_warn("GetOverlappedResult failed");
		return -1;
	}

	// we read into align buffer - copy to user's buffer
	if(r->read_into_align_buffer)
		cpu_memcpy((void*)cb->aio_buf, (u8*)r->buf + r->pad, cb->aio_nbytes);

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
	UNUSED2(cb);

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
	UNUSED2(se);

	int err = 0;

	for(int i = 0; i < n; i++)
	{
		int ret = aio_rw(cbs[i]);		// checks for cbs[i] == 0
		// don't RETURN_ERR yet - we want to try to issue each one
		if(ret < 0 && !err)
			err = ret;
	}

	RETURN_ERR(err);

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


static LibError waio_init()
{
	req_init();
	return INFO::OK;
}


static LibError waio_shutdown()
{
	req_cleanup();
	aio_h_cleanup();
	return INFO::OK;
}
