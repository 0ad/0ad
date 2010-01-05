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
 * association between POSIX file descriptor and Win32 HANDLE.
 * NB: callers must ensure thread safety.
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
		std::pair<Map::iterator, bool> ret = m_map.insert(std::make_pair(fd, hFile));
		debug_assert(ret.second);	// fd better not already have been associated
	}

	void Dissociate(int fd)
	{
		const size_t numRemoved = m_map.erase(fd);
		debug_assert(numRemoved == 1);
	}

	bool IsAssociated(int fd) const
	{
		return m_map.find(fd) != m_map.end();
	}

	/**
	 * @return aio handle associated with file descriptor or
	 * INVALID_HANDLE_VALUE if there is none.
	 **/
	HANDLE Get(int fd) const
	{
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


// do we want to open a second aio-capable handle?
static bool IsAioPossible(int fd, bool is_com_port, int oflag)
{
	// stdin/stdout/stderr
	if(fd <= 2)
		return false;

	// COM port - we don't currently need aio access for those, and
	// aio_reopen's CreateFileW would fail with "access denied".
	if(is_com_port)
		return false;

	// caller is requesting we skip it (see open())
	if(oflag & O_NO_AIO_NP)
		return false;

	return true;
}

// (re)open file in asynchronous mode and associate handle with fd.
// (this works because the files default to DENY_NONE sharing)
LibError waio_reopen(int fd, const wchar_t* pathname, int oflag, ...)
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

	if(!IsAioPossible(fd, false, oflag))
		return INFO::SKIPPED;

	// open file
	const DWORD flags = FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING|FILE_FLAG_SEQUENTIAL_SCAN;
	const HANDLE hFile = CreateFileW(pathname, access, share, 0, create, flags, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		return LibError_from_GLE();

	{
		WinScopedLock lock(WAIO_CS);
		handleManager->Associate(fd, hFile);
	}
	return INFO::OK;
}


LibError waio_close(int fd)
{
	HANDLE hFile;
	{
		WinScopedLock lock(WAIO_CS);
		if(!handleManager->IsAssociated(fd))	// wasn't opened for aio
			return INFO::SKIPPED;
		hFile = handleManager->Get(fd);
		handleManager->Dissociate(fd);
	}

	if(!CloseHandle(hFile))
		WARN_RETURN(ERR::INVALID_HANDLE);

	return INFO::OK;
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
	return _lseeki64(fd, ofs, whence);
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


// called by aio_read, aio_write, and lio_listio.
// cb->aio_lio_opcode specifies desired operation.
// @return LibError, also setting errno in case of failure.
static LibError aio_issue(struct aiocb* cb)
{
	// no-op (probably from lio_listio)
	if(!cb || cb->aio_lio_opcode == LIO_NOP)
		return INFO::SKIPPED;

	// extract aiocb fields for convenience
	const bool isWrite = (cb->aio_lio_opcode == LIO_WRITE);
	const int fd       = cb->aio_fildes;
	const size_t size  = cb->aio_nbytes;
	const off_t ofs    = cb->aio_offset;
	void* const buf    = (void*)cb->aio_buf; // from volatile void*

	// Win32 requires transfers to be sector-aligned.
	if(!IsAligned(ofs, sectorSize) || !IsAligned(buf, sectorSize) || !IsAligned(size, sectorSize))
	{
		errno = EINVAL;
		WARN_RETURN(ERR::INVALID_PARAM);
	}

	HANDLE hFile;
	{
		WinScopedLock lock(WAIO_CS);
		hFile = handleManager->Get(fd);
	}
	if(hFile == INVALID_HANDLE_VALUE)
	{
		errno = EINVAL;
		WARN_RETURN(ERR::INVALID_HANDLE);
	}

	debug_assert(!cb->impl);	// SUSv3 requires that the aiocb not be in use
	cb->impl.reset(new aiocb::Impl);

	LibError ret = cb->impl->Issue(hFile, ofs, buf, size, isWrite);
	if(ret < 0)
	{
		LibError_set_errno(ret);
		return ret;
	}

	return INFO::OK;
}


// return status of transfer
int aio_error(const struct aiocb* cb)
{
	return cb->impl->HasCompleted()? 0 : EINPROGRESS;
}


// get bytes transferred. call exactly once for each issued request.
ssize_t aio_return(struct aiocb* cb)
{
	// SUSv3 says we mustn't be callable before the request has completed
	debug_assert(cb->impl);
	debug_assert(cb->impl->HasCompleted());
	size_t bytesTransferred;
	LibError ret = cb->impl->GetResult(&bytesTransferred);
	cb->impl.reset();	// disallow calling again, as required by SUSv3
	if(ret < 0)
	{
		LibError_set_errno(ret);
		return (ssize_t)-1;
	}
	return (ssize_t)bytesTransferred;
}


int aio_suspend(const struct aiocb* const cbs[], int n, const struct timespec* ts)
{
	if(n <= 0 || n > MAXIMUM_WAIT_OBJECTS)
	{
		WARN_ERR(ERR::INVALID_PARAM);
		errno = EINVAL;
		return -1;
	}

	// build array of event handles
	HANDLE hEvents[MAXIMUM_WAIT_OBJECTS];
	size_t numPendingIos = 0;
	for(int i = 0; i < n; i++)
	{
		if(!cbs[i])	// SUSv3 says NULL entries are to be ignored
			continue;

		aiocb::Impl* impl = cbs[i]->impl.get();
		debug_assert(impl);
		if(!impl->HasCompleted())
			hEvents[numPendingIos++] = impl->Event();
	}
	if(!numPendingIos)	// done, don't need to suspend.
		return 0;

	const BOOL waitAll = FALSE;
	// convert timespec to milliseconds (ts == 0 => no timeout)
	const DWORD timeout = ts? (DWORD)(ts->tv_sec*1000 + ts->tv_nsec/1000000) : INFINITE;
	const DWORD result = WaitForMultipleObjects((DWORD)numPendingIos, hEvents, waitAll, timeout);

	for(size_t i = 0; i < numPendingIos; i++)
		ResetEvent(hEvents[i]);

	switch(result)
	{
	case WAIT_FAILED:
		WARN_ERR(ERR::FAIL);
		errno = EIO;
		return -1;

	case WAIT_TIMEOUT:
		errno = EAGAIN;
		return -1;

	default:
		return 0;
	}
}


int aio_cancel(int fd, struct aiocb* cb)
{
	// Win32 limitation: can't cancel single transfers -
	// all pending reads on this file are canceled.
	UNUSED2(cb);

	HANDLE hFile;
	{
		WinScopedLock lock(WAIO_CS);
		hFile = handleManager->Get(fd);
	}
	if(hFile == INVALID_HANDLE_VALUE)
	{
		WARN_ERR(ERR::INVALID_HANDLE);
		errno = EINVAL;
		return -1;
	}

	WARN_IF_FALSE(CancelIo(hFile));
	return AIO_CANCELED;
}


int aio_read(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_READ;
	return (aio_issue(cb) < 0)? 0 : -1;
}


int aio_write(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_WRITE;
	return (aio_issue(cb) < 0)? 0 : -1;
}


int lio_listio(int mode, struct aiocb* const cbs[], int n, struct sigevent* se)
{
	debug_assert(mode == LIO_WAIT || mode == LIO_NOWAIT);
	UNUSED2(se);	// signaling is not implemented.

	for(int i = 0; i < n; i++)
	{
		if(aio_issue(cbs[i]) < 0)
			return -1;
	}

	if(mode == LIO_WAIT)
		return aio_suspend(cbs, n, 0);

	return 0;
}


int aio_fsync(int, struct aiocb*)
{
	WARN_ERR(ERR::NOT_IMPLEMENTED);
	errno = ENOSYS;
	return -1;
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
