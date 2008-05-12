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

#include "crt_posix.h"      // correct definitions of _open() etc.

#include "wposix_internal.h"
#include "wfilesystem.h"    // mode_t
#include "wtime.h"          // timespec
#include "lib/sysdep/cpu.h"
#include "lib/bits.h"


WINIT_REGISTER_MAIN_INIT(waio_Init);
WINIT_REGISTER_MAIN_SHUTDOWN(waio_Shutdown);

// note: we assume sector sizes no larger than a page.
// (GetDiskFreeSpace allows querying the actual size, but we'd
// have to do so for all drives, and that may change depending on whether
// there is a DVD in the drive or not)
// sector size is relevant because Windows aio requires all IO
// buffers, offsets and lengths to be a multiple of it. this requirement
// is also carried over into the vfs / file.cpp interfaces for efficiency
// (avoids the need for copying to/from align buffers).
const uintptr_t sectorSize = 0x1000;

//-----------------------------------------------------------------------------

// note: the Windows lowio file descriptor limit is currrently 2048.

/**
 * thread-safe association between POSIX file descriptor and Win32 HANDLE
 **/
class HandleManager
{
public:
	/**
	 * associate an aio handle with a file descriptor.
	 **/
	void Associate(int fd, HANDLE hFile)
	{
		debug_assert(fd > 2);
		debug_assert(GetFileSize(hFile, 0) != INVALID_FILE_SIZE);

		WinScopedLock lock(WAIO_CS);
		std::pair<Map::iterator, bool> ret = m_map.insert(std::make_pair(fd, hFile));
		debug_assert(ret.second);	// fd better not already have been associated
	}

	void Dissociate(int fd)
	{
		WinScopedLock lock(WAIO_CS);
		const size_t numRemoved = m_map.erase(fd);
		debug_assert(numRemoved == 1);
	}

	/**
	 * @return aio handle associated with file descriptor or
	 * INVALID_HANDLE_VALUE if there is none.
	 **/
	HANDLE Get(int fd) const
	{
		WinScopedLock lock(WAIO_CS);
		Map::const_iterator it = m_map.find(fd);
		if(it == m_map.end())
			return INVALID_HANDLE_VALUE;
		return it->second;
	}

private:
	typedef std::map<int, HANDLE> Map;
	Map m_map;
};

static HandleManager* handleManager;


// open fn in async mode and associate handle with fd.
int aio_reopen(int fd, const char* fn, int oflag, ...)
{
	WinScopedPreserveLastError s;	// CreateFile

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
	const DWORD flags = FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN;
	const HANDLE hFile = CreateFile(fn, access, share, 0, create, flags, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		WARN_RETURN(-1);

	handleManager->Associate(fd, hFile);
	return 0;
}


int aio_close(int fd)
{
	HANDLE hFile = handleManager->Get(fd);
	// early out for files that were never re-opened for AIO.
	// since there is no way for wposix close to know this, we mustn't
	// return an error (which would cause it to WARN_ERR).
	if(hFile == INVALID_HANDLE_VALUE)
		return 0;

	if(!CloseHandle(hFile))
		WARN_RETURN(-1);

	handleManager->Dissociate(fd);

	return 0;
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

	WinScopedPreserveLastError s;	// _open's CreateFile
	int fd = _open(fn, oflag, mode);

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

int truncate(const char* path, off_t length)
{
	HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	debug_assert(hFile != INVALID_HANDLE_VALUE);
	LARGE_INTEGER ofs; ofs.QuadPart = length;
	WARN_IF_FALSE(SetFilePointerEx(hFile, ofs, 0, FILE_BEGIN));
	WARN_IF_FALSE(SetEndOfFile(hFile));
	WARN_IF_FALSE(CloseHandle(hFile));
	return 0;
}


//-----------------------------------------------------------------------------

class aiocb::Impl
{
public:
	Impl()
	{
		m_hFile = INVALID_HANDLE_VALUE;
		const BOOL manualReset = TRUE;
		const BOOL initialState = FALSE;
		m_overlapped.hEvent = CreateEvent(0, manualReset, initialState, 0);
	}

	~Impl()
	{
		CloseHandle(m_overlapped.hEvent);
	}

	LibError Issue(HANDLE hFile, off_t ofs, void* buf, size_t size, bool isWrite)
	{
		WinScopedPreserveLastError s;

		m_hFile = hFile;

		// note: Read-/WriteFile reset m_overlapped.hEvent, so we don't have to.
		m_overlapped.Internal = m_overlapped.InternalHigh = 0;
		m_overlapped.Offset     = u64_lo(ofs);
		m_overlapped.OffsetHigh = u64_hi(ofs);

		DWORD bytesTransferred;
		BOOL ok;
		if(isWrite)
			ok = WriteFile(hFile, buf, u64_lo(size), &bytesTransferred, &m_overlapped);
		else
			ok =  ReadFile(hFile, buf, u64_lo(size), &bytesTransferred, &m_overlapped);
		if(!ok && GetLastError() == ERROR_IO_PENDING)	// "pending" isn't an error
		{
			ok = TRUE;
			SetLastError(0);
		}
		return LibError_from_win32(ok);
	}

	bool HasCompleted() const
	{
		debug_assert(m_overlapped.Internal == 0 || m_overlapped.Internal == STATUS_PENDING);
		return HasOverlappedIoCompleted(&m_overlapped);
	}

	// required for WaitForMultipleObjects
	HANDLE Event() const
	{
		return m_overlapped.hEvent;
	}

	LibError GetResult(size_t* pBytesTransferred)
	{
		DWORD bytesTransferred;
		const BOOL wait = FALSE;	// callers should wait until HasCompleted
		if(!GetOverlappedResult(m_hFile, &m_overlapped, &bytesTransferred, wait))
		{
			*pBytesTransferred = 0;
			return LibError_from_GLE();
		}
		else
		{
			*pBytesTransferred = bytesTransferred;
			return INFO::OK;
		}
	}

private:
	OVERLAPPED m_overlapped;
	HANDLE m_hFile;
};


// called by aio_read, aio_write, and lio_listio
// cb->aio_lio_opcode specifies desired operation
static int aio_issue(struct aiocb* cb)
{
	// no-op from lio_listio
	if(!cb || cb->aio_lio_opcode == LIO_NOP)
		return 0;

	// extract aiocb fields for convenience
	const bool isWrite = (cb->aio_lio_opcode == LIO_WRITE);
	const int fd       = cb->aio_fildes;
	const size_t size  = cb->aio_nbytes;
	const off_t ofs    = cb->aio_offset;
	void* const buf    = (void*)cb->aio_buf; // from volatile void*

	// Win32 requires transfers to be sector-aligned.
	if(!IsAligned(ofs, sectorSize) || !IsAligned(buf, sectorSize) || !IsAligned(size, sectorSize))
		WARN_RETURN(-EINVAL);

	const HANDLE hFile = handleManager->Get(fd);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		errno = -EINVAL;
		WARN_RETURN(-1);
	}

	debug_assert(!cb->impl);	// fail if aiocb is already in use (forbidden by SUSv3)
	cb->impl.reset(new aiocb::Impl);
	LibError ret = cb->impl->Issue(hFile, ofs, buf, size, isWrite);
	if(ret < 0)
	{
		LibError_set_errno(ret);
		WARN_RETURN(-1);
	}
	return 0;
}


// return status of transfer
int aio_error(const struct aiocb* cb)
{
	return cb->impl->HasCompleted()? 0 : EINPROGRESS;
}


// get bytes transferred. call exactly once for each op.
ssize_t aio_return(struct aiocb* cb)
{
	debug_assert(cb->impl->HasCompleted());	// SUSv3 says we mustn't be callable before IO completes
	size_t bytesTransferred;
	LibError ret = cb->impl->GetResult(&bytesTransferred);
	cb->impl.reset();	// disallow calling again, as required by SUSv3
	if(ret < 0)
	{
		LibError_set_errno(ret);
		WARN_RETURN(-1);
	}
	return (ssize_t)bytesTransferred;
}


int aio_suspend(const struct aiocb* const cbs[], int n, const struct timespec* ts)
{
	if(n <= 0 || n > MAXIMUM_WAIT_OBJECTS)
		WARN_RETURN(-1);

	// build array of event handles
	HANDLE hEvents[MAXIMUM_WAIT_OBJECTS];
	size_t numPendingIos = 0;
	for(int i = 0; i < n; i++)
	{
		if(!cbs[i])	// SUSv3 says NULL entries are to be ignored
			continue;

		aiocb::Impl* impl = cbs[i]->impl.get();
		if(!impl->HasCompleted())
			hEvents[numPendingIos++] = impl->Event();
	}
	if(!numPendingIos)	// done, don't need to suspend.
		return 0;

	const BOOL waitAll = FALSE;
	// convert timespec to milliseconds (ts == 0 => no timeout)
	const DWORD timeout = ts? (DWORD)(ts->tv_sec*1000 + ts->tv_nsec/1000000) : INFINITE;
	DWORD result = WaitForMultipleObjects((DWORD)numPendingIos, hEvents, waitAll, timeout);

	for(size_t i = 0; i < numPendingIos; i++)
		ResetEvent(hEvents[i]);

	switch(result)
	{
	case WAIT_FAILED:
		WARN_RETURN(-1);

	case WAIT_TIMEOUT:
		errno = -EAGAIN;
		return -1;

	default:
		return 0;
	}
}


int aio_cancel(int fd, struct aiocb* cb)
{
	// Win32 limitation: can't cancel single transfers -
	// all pending reads on this file are cancelled.
	UNUSED2(cb);

	const HANDLE hFile = handleManager->Get(fd);
	if(hFile == INVALID_HANDLE_VALUE)
		WARN_RETURN(-1);

	CancelIo(hFile);
	return AIO_CANCELED;
}


int aio_read(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_READ;
	return aio_issue(cb);
}


int aio_write(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_WRITE;
	return aio_issue(cb);
}


int lio_listio(int mode, struct aiocb* const cbs[], int n, struct sigevent* UNUSED(se))
{
	for(int i = 0; i < n; i++)
		RETURN_ERR(aio_issue(cbs[i]));

	if(mode == LIO_WAIT)
		RETURN_ERR(aio_suspend(cbs, n, 0));

	return 0;
}


int aio_fsync(int, struct aiocb*)
{
	WARN_RETURN(-ENOSYS);
}


//-----------------------------------------------------------------------------

static LibError waio_Init()
{
	handleManager = new HandleManager;
	return INFO::OK;
}

static LibError waio_Shutdown()
{
	delete handleManager;
	return INFO::OK;
}
