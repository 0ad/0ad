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

/*
 * emulate POSIX asynchronous I/O on Windows.
 */

// NB: this module is significantly faster than Intel's aio library,
// which also returns ERROR_INVALID_PARAMETER from aio_error if the
// file is opened with FILE_FLAG_OVERLAPPED. (it looks like they are
// using threaded blocking IO)

#include "precompiled.h"
#include "lib/sysdep/os/win/wposix/waio.h"

#include "lib/bits.h"	// round_up
#include "lib/alignment.h"	// IsAligned
#include "lib/module_init.h"
#include "lib/sysdep/cpu.h"	// cpu_AtomicAdd
#include "lib/sysdep/filesystem.h"	// O_DIRECT
#include "lib/sysdep/os/win/wutil.h"	// wutil_SetPrivilege
#include "lib/sysdep/os/win/wiocp.h"
#include "lib/sysdep/os/win/winit.h"
#include "lib/sysdep/os/win/wposix/crt_posix.h"	// _get_osfhandle

WINIT_REGISTER_MAIN_SHUTDOWN(waio_Shutdown);

// (dynamic linking preserves compatibility with previous Windows versions)
static WUTIL_FUNC(pSetFileCompletionNotificationModes, BOOL, (HANDLE, UCHAR));
static WUTIL_FUNC(pSetFileIoOverlappedRange, BOOL, (HANDLE, PUCHAR, ULONG));
static WUTIL_FUNC(pSetFileValidData, BOOL, (HANDLE, LONGLONG));

// (there must be one global IOCP because aio_suspend might be called for
// requests from different files)
static HANDLE hIOCP;


//-----------------------------------------------------------------------------
// OpenFile

static DWORD DesiredAccess(int oflag)
{
	switch(oflag & (O_RDONLY|O_WRONLY|O_RDWR))
	{
	case O_RDONLY:
		// (WinXP x64 requires FILE_WRITE_ATTRIBUTES for SetFileCompletionNotificationModes)
		return GENERIC_READ|FILE_WRITE_ATTRIBUTES;
	case O_WRONLY:
		return GENERIC_WRITE;
	case O_RDWR:
		return GENERIC_READ|GENERIC_WRITE;
	default:
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return 0;
	}
}

static DWORD ShareMode(int oflag)
{
	switch(oflag & (O_RDONLY|O_WRONLY|O_RDWR))
	{
	case O_RDONLY:
		return FILE_SHARE_READ;
	case O_WRONLY:
		return FILE_SHARE_WRITE;
	case O_RDWR:
		return FILE_SHARE_WRITE;	// deny read access (c.f. waio_Preallocate)
	default:
		DEBUG_WARN_ERR(ERR::INVALID_PARAM);
		return 0;
	}
}

static DWORD CreationDisposition(int oflag)
{
	if(oflag & O_CREAT)
		return (oflag & O_EXCL)? CREATE_NEW : CREATE_ALWAYS;

	if(oflag & O_TRUNC)
		return TRUNCATE_EXISTING;

	return OPEN_EXISTING;
}

static DWORD FlagsAndAttributes()
{
	// - FILE_FLAG_SEQUENTIAL_SCAN is ignored when FILE_FLAG_NO_BUFFERING
	//   is set (c.f. "Windows via C/C++", p. 324)
	// - FILE_FLAG_WRITE_THROUGH is ~5% slower (diskspd.cpp suggests it
	//   disables hardware caching; the overhead may also be due to the
	//   Windows cache manager)
	const DWORD flags = FILE_FLAG_OVERLAPPED|FILE_FLAG_NO_BUFFERING;
	const DWORD attributes = FILE_ATTRIBUTE_NORMAL;
	return flags|attributes;
}

static Status OpenFile(const OsPath& pathname, int oflag, HANDLE& hFile)
{
	WinScopedPreserveLastError s;

	const DWORD access = DesiredAccess(oflag);
	const DWORD share  = ShareMode(oflag);
	const DWORD create = CreationDisposition(oflag);
	const DWORD flags  = FlagsAndAttributes();
	hFile = CreateFileW(OsString(pathname).c_str(), access, share, 0, create, flags, 0);
	if(hFile == INVALID_HANDLE_VALUE)
		WARN_RETURN(StatusFromWin());

	return INFO::OK;
}


//-----------------------------------------------------------------------------
// OvlAllocator

// allocator for OVERLAPPED (enables a special optimization, see Associate)
struct OvlAllocator	// POD
{
	// freelist entries for (padded) OVERLAPPED from our storage
	struct Entry
	{
		SLIST_ENTRY entry;
		OVERLAPPED ovl;
	};

	Status Init()
	{
		// the allocation must be naturally aligned to ensure it doesn't
		// overlap another page, which might prevent SetFileIoOverlappedRange
		// from pinning the pages if one of them is PAGE_NOACCESS.
		storage = _mm_malloc(storageSize, storageSize);
		if(!storage)
			WARN_RETURN(ERR::NO_MEM);
		memset(storage, 0, storageSize);

		InitializeSListHead(&freelist);

		// storageSize provides more than enough OVERLAPPED, so we
		// pad them to the cache line size to maybe avoid a few RFOs.
		const size_t size = Align<cacheLineSize>(sizeof(Entry));
		for(uintptr_t offset = 0; offset+size <= storageSize; offset += size)
		{
			Entry* entry = (Entry*)(uintptr_t(storage) + offset);
			ENSURE(IsAligned(entry, MEMORY_ALLOCATION_ALIGNMENT));
			InterlockedPushEntrySList(&freelist, &entry->entry);
		}

		extant = 0;

		return INFO::OK;
	}

	void Shutdown()
	{
		if(extant != 0)
			debug_printf("waio: OvlAllocator::Shutdown with extant=%d\n", extant);

		InterlockedFlushSList(&freelist);

		_mm_free(storage);
		storage = 0;
	}

	// irrevocably enable a special optimization for all I/Os requests
	// concerning this file, ending when the file is closed. has no effect
	// unless Vista+ and SeLockMemoryPrivilege are available.
	void Associate(HANDLE hFile)
	{
		ENSURE(extant == 0);

		// pin the page in kernel address space, which means our thread
		// won't have to be the one to service the I/O, thus avoiding an APC.
		// ("thread agnostic I/O")
		if(pSetFileIoOverlappedRange)
			WARN_IF_FALSE(pSetFileIoOverlappedRange(hFile, (PUCHAR)storage, storageSize));
	}

	// @return OVERLAPPED initialized for I/O starting at offset,
	// or 0 if all available structures have already been allocated.
	OVERLAPPED* Allocate(off_t offset)
	{
		Entry* entry = (Entry*)InterlockedPopEntrySList(&freelist);
		if(!entry)
			return 0;

		OVERLAPPED& ovl = entry->ovl;
		ovl.Internal = 0;
		ovl.InternalHigh = 0;
		ovl.Offset = u64_lo(offset);
		ovl.OffsetHigh = u64_hi(offset);
		ovl.hEvent = 0;	// (notification is via IOCP and/or polling)

		cpu_AtomicAdd(&extant, +1);

		return &ovl;
	}

	void Deallocate(OVERLAPPED* ovl)
	{
		cpu_AtomicAdd(&extant, -1);

		const uintptr_t address = uintptr_t(ovl);
		ENSURE(uintptr_t(storage) <= address && address < uintptr_t(storage)+storageSize);
		InterlockedPushEntrySList(&freelist, (PSLIST_ENTRY)(address - offsetof(Entry, ovl)));
	}

	// one 4 KiB page is enough for 64 OVERLAPPED per file (i.e. plenty).
	static const size_t storageSize = pageSize;

	void* storage;

#if MSC_VERSION
# pragma warning(push)
# pragma warning(disable:4324)	// structure was padded due to __declspec(align())
#endif
	__declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) SLIST_HEADER freelist;
#if MSC_VERSION
# pragma warning(pop)
#endif

	volatile intptr_t extant;
};


//-----------------------------------------------------------------------------
// FileControlBlock

// information required to start asynchronous I/Os from a file
struct FileControlBlock // POD
{
	HANDLE hFile;

	OvlAllocator ovl;

	Status Init()
	{
		hFile = INVALID_HANDLE_VALUE;
		return ovl.Init();
	}

	void Shutdown()
	{
		ENSURE(hFile == INVALID_HANDLE_VALUE);
		ovl.Shutdown();
	}

	Status Open(const OsPath& pathname, int oflag)
	{
		RETURN_STATUS_IF_ERR(OpenFile(pathname, oflag, hFile));

		ovl.Associate(hFile);

		AttachToCompletionPort(hFile, hIOCP, (ULONG_PTR)this);

		// minor optimization: avoid posting to IOCP in rare cases
		// where the I/O completes synchronously
		if(pSetFileCompletionNotificationModes)
		{
			// (for reasons as yet unknown, this fails when the file is
			// opened for read-only access)
			(void)pSetFileCompletionNotificationModes(hFile, FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);
		}

		return INFO::OK;
	}

	Status Close()
	{
		const BOOL ok = CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		if(!ok)
			WARN_RETURN(ERR::INVALID_HANDLE);
		return INFO::OK;
	}
};


//-----------------------------------------------------------------------------
// FileControlBlocks

struct FileControlBlocks // POD
{
	// (we never open more than a few files at a time.)
	static const size_t maxFiles = 8;

	// (our descriptors exceed _NHANDLE_ (2048) to ensure they are not
	// confused with lowio descriptors.)
	static const int firstDescriptor = 4000;

	FileControlBlock fcbs[maxFiles];
	CACHE_ALIGNED(volatile intptr_t) inUse[maxFiles];

	void Init()
	{
		for(size_t i = 0; i < maxFiles; i++)
		{
			inUse[i] = 0;
			fcbs[i].Init();
		}
	}

	void Shutdown()
	{
		for(size_t i = 0; i < maxFiles; i++)
		{
			ENSURE(inUse[i] == 0);
			fcbs[i].Shutdown();
		}
	}

	FileControlBlock* Allocate()
	{
		for(size_t i = 0; i < maxFiles; i++)
		{
			if(cpu_CAS(&inUse[i], 0, 1))
				return &fcbs[i];
		}

		return 0;
	}

	void Deallocate(FileControlBlock* fcb)
	{
		const size_t index = fcb - &fcbs[0];
		inUse[index] = 0;
	}

	int Descriptor(FileControlBlock* fcb) const
	{
		const size_t index = fcb - &fcbs[0];
		return firstDescriptor + (int)index;
	}

	FileControlBlock* FromDescriptor(int descriptor)
	{
		const size_t index = size_t(descriptor - firstDescriptor);
		if(index >= maxFiles)
			return 0;
		if(!inUse[index])
			return 0;
		return &fcbs[index];
	}
};

static FileControlBlocks fileControlBlocks;


//-----------------------------------------------------------------------------
// init/shutdown

static ModuleInitState waio_initState;

// called from waio_Open (avoids overhead if this module is never used)
static Status waio_Init()
{
	fileControlBlocks.Init();

	WUTIL_IMPORT_KERNEL32(SetFileCompletionNotificationModes, pSetFileCompletionNotificationModes);

	// NB: using these functions when the privileges are not available would
	// trigger warnings. since callers have to check the function pointers
	// anyway, just refrain from setting them in such cases.

	if(wutil_SetPrivilege(L"SeLockMemoryPrivilege", true) == INFO::OK)
		WUTIL_IMPORT_KERNEL32(SetFileIoOverlappedRange, pSetFileIoOverlappedRange);

	if(wutil_SetPrivilege(L"SeManageVolumePrivilege", true) == INFO::OK)
		WUTIL_IMPORT_KERNEL32(SetFileValidData, pSetFileValidData);

	return INFO::OK;
}


static Status waio_Shutdown()
{
	if(waio_initState == 0)	// we were never initialized
		return INFO::OK;

	fileControlBlocks.Shutdown();

	WARN_IF_FALSE(CloseHandle(hIOCP));

	return INFO::OK;
}


//-----------------------------------------------------------------------------
// Windows-only APIs

Status waio_open(const OsPath& pathname, int oflag, ...)
{
	ASSERT(oflag & O_DIRECT);
	ENSURE(!(oflag & O_APPEND));	// not supported

	RETURN_STATUS_IF_ERR(ModuleInit(&waio_initState, waio_Init));

	FileControlBlock* fcb = fileControlBlocks.Allocate();
	if(!fcb)
		WARN_RETURN(ERR::LIMIT);

	RETURN_STATUS_IF_ERR(fcb->Open(pathname, oflag));
	return (Status)fileControlBlocks.Descriptor(fcb);
}


Status waio_close(int fd)
{
	FileControlBlock* fcb = fileControlBlocks.FromDescriptor(fd);
	if(!fcb)
		return ERR::INVALID_HANDLE;	// NOWARN - fd might be from lowio

	Status ret = fcb->Close();
	fileControlBlocks.Deallocate(fcb);
	return ret;
}


Status waio_Preallocate(int fd, off_t size)
{
	WinScopedPreserveLastError s;

	HANDLE hFile;	// from FileControlBlock OR lowio
	{
		FileControlBlock* fcb = fileControlBlocks.FromDescriptor(fd);
		if(fcb)
			hFile = fcb->hFile;
		else
		{
			hFile = (HANDLE)_get_osfhandle(fd);
			if(hFile == INVALID_HANDLE_VALUE)
				WARN_RETURN(ERR::INVALID_HANDLE);
		}
	}

	// Windows requires sector alignment (see discussion in header)
	const off_t alignedSize = round_up(size, (off_t)maxSectorSize);	// (Align<> cannot compute off_t)

	// allocate all space up front to reduce fragmentation
	LARGE_INTEGER size64; size64.QuadPart = alignedSize;
	WARN_IF_FALSE(SetFilePointerEx(hFile, size64, 0, FILE_BEGIN));
	if(!SetEndOfFile(hFile))
	{
		if(GetLastError() == ERROR_DISK_FULL)
			SetLastError(0);
		else
			debug_printf("waio_Preallocate(%lld) failed: %d\n", size, GetLastError());
		return ERR::FAIL;	// NOWARN (either out of disk space, or error was printed)
	}

	// avoid synchronous zero-fill (see discussion in header)
	if(pSetFileValidData)
	{
		// this has been reported (#849) to fail with GetLastError() == 0 for
		// both tiny and medium alignedSize, despite Administrator privileges.
		// it seems the problem is the underlying FAT filesystem. Nick Ryan:
		// "FastFat does not process the FILE_VALID_DATA_LENGTH_INFORMATION IOCTL"
		// (http://us.generation-nt.com/answer/setfilevaliddata-help-25926952.html?page=2)
		// verifying the filesystem is indeed FAT would be overkill; we'll just
		// ignore the return code. however, GetLastError can be used to check
		// whether other problems arose.
		(void)pSetFileValidData(hFile, alignedSize);
		ENSURE(GetLastError() == 0);
	}

	return INFO::OK;
}


//-----------------------------------------------------------------------------
// helper functions

// called by aio_read, aio_write, and lio_listio.
// cb->aio_lio_opcode specifies desired operation.
// @return -1 on failure (having also set errno)
static int Issue(aiocb* cb)
{
	ENSURE(IsAligned(cb->aio_offset, maxSectorSize));
	ENSURE(IsAligned(cb->aio_buf,    maxSectorSize));
	ENSURE(IsAligned(cb->aio_nbytes, maxSectorSize));

	FileControlBlock* fcb = fileControlBlocks.FromDescriptor(cb->aio_fildes);
	if(!fcb)
	{
		DEBUG_WARN_ERR(ERR::INVALID_HANDLE);
		errno = EINVAL;
		return -1;
	}

	ENSURE(!cb->ovl);	// SUSv3: aiocb must not be in use
	cb->ovl = fcb->ovl.Allocate(cb->aio_offset);
	if(!cb->ovl)
	{
		DEBUG_WARN_ERR(ERR::LIMIT);
		errno = EMFILE;
		return -1;
	}

	WinScopedPreserveLastError s;

	const HANDLE hFile = fcb->hFile;
	void* const buf = (void*)cb->aio_buf; // from volatile void*
	const DWORD size = u64_lo(cb->aio_nbytes);
	ENSURE(u64_hi(cb->aio_nbytes) == 0);
	OVERLAPPED* ovl = (OVERLAPPED*)cb->ovl;
	// (there is no point in using WriteFileGather/ReadFileScatter here
	// because the IO manager still needs to lock pages and translate them
	// into an MDL, and we'd just be increasing the number of addresses)
	const BOOL ok = (cb->aio_lio_opcode == LIO_WRITE)? WriteFile(hFile, buf, size, 0, ovl) : ReadFile(hFile, buf, size, 0, ovl);
	if(ok || GetLastError() == ERROR_IO_PENDING)
		return 0;	// success

	const Status status = StatusFromWin();
	WARN_IF_ERR(status);
	errno = ErrnoFromStatus(status);
	return -1;
}


static bool AreAnyComplete(const struct aiocb* const cbs[], int n)
{
	for(int i = 0; i < n; i++)
	{
		if(!cbs[i])	// SUSv3: must ignore NULL entries
			continue;

		if(HasOverlappedIoCompleted((OVERLAPPED*)cbs[i]->ovl))
			return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// API

int aio_read(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_READ;
	return Issue(cb);
}


int aio_write(struct aiocb* cb)
{
	cb->aio_lio_opcode = LIO_WRITE;
	return Issue(cb);
}


int lio_listio(int mode, struct aiocb* const cbs[], int n, struct sigevent* se)
{
	ENSURE(mode == LIO_WAIT || mode == LIO_NOWAIT);
	UNUSED2(se);	// signaling is not implemented.

	for(int i = 0; i < n; i++)
	{
		if(cbs[i] == 0 || cbs[i]->aio_lio_opcode == LIO_NOP)
			continue;

		if(Issue(cbs[i]) == -1)
			return -1;
	}

	if(mode == LIO_WAIT)
		return aio_suspend(cbs, n, 0);

	return 0;
}


int aio_suspend(const struct aiocb* const cbs[], int n, const struct timespec* timeout)
{
	// consume all pending notifications to prevent them from piling up if
	// requests are always complete by the time we're called
	DWORD bytesTransferred; ULONG_PTR key; OVERLAPPED* ovl;
	while(PollCompletionPort(hIOCP, 0, bytesTransferred, key, ovl) == INFO::OK) {}

	// avoid blocking if already complete (synchronous requests don't post notifications)
	if(AreAnyComplete(cbs, n))
		return 0;

	// caller doesn't want to block, and no requests are complete
	if(timeout && timeout->tv_sec == 0 && timeout->tv_nsec == 0)
	{
		errno = EAGAIN;
		return -1;
	}

	// reduce CPU usage by blocking until a notification arrives or a
	// brief timeout elapses (necessary because other threads - or even
	// the above poll - might have consumed our notification). note that
	// re-posting notifications that don't concern the respective requests
	// is not desirable because POSIX doesn't require aio_suspend to be
	// called, which means notifications might pile up.
	const DWORD milliseconds = 1;	// as short as possible (don't oversleep)
	const Status ret = PollCompletionPort(hIOCP, milliseconds, bytesTransferred, key, ovl);
	if(ret != INFO::OK && ret != ERR::AGAIN)	// failed
	{
		DEBUG_WARN_ERR(ERR::LOGIC);
		return -1;
	}

	// scan again (even if we got a notification, it might not concern THESE requests)
	if(AreAnyComplete(cbs, n))
		return 0;

	// none completed, must repeat the above steps. provoke being called again by
	// claiming to have been interrupted by a signal.
	errno = EINTR;
	return -1;
}


int aio_error(const struct aiocb* cb)
{
	const OVERLAPPED* ovl = (const OVERLAPPED*)cb->ovl;
	if(!ovl)	// called after aio_return
		return EINVAL;
	if(!HasOverlappedIoCompleted(ovl))
		return EINPROGRESS;
	if(ovl->Internal != ERROR_SUCCESS)
		return EIO;
	return 0;
}


ssize_t aio_return(struct aiocb* cb)
{
	FileControlBlock* fcb = fileControlBlocks.FromDescriptor(cb->aio_fildes);
	OVERLAPPED* ovl = (OVERLAPPED*)cb->ovl;
	if(!fcb || !ovl)
	{
		errno = EINVAL;
		return -1;
	}

	const ULONG_PTR status = ovl->Internal;
	const ULONG_PTR bytesTransferred = ovl->InternalHigh;

	cb->ovl = 0;	// prevent further calls to aio_error/aio_return
	COMPILER_FENCE;
	fcb->ovl.Deallocate(ovl);

	return (status == ERROR_SUCCESS)? bytesTransferred : -1;
}


// Win32 limitation: cancel all I/Os this thread issued for the given file
// (CancelIoEx can cancel individual operations, but is only
// available starting with Vista)
int aio_cancel(int fd, struct aiocb* UNUSED(cb))
{
	FileControlBlock* fcb = fileControlBlocks.FromDescriptor(fd);
	if(!fcb)
	{
		WARN_IF_ERR(ERR::INVALID_HANDLE);
		errno = EINVAL;
		return -1;
	}

	WARN_IF_FALSE(CancelIo(fcb->hFile));

	return AIO_CANCELED;
}


int aio_fsync(int, struct aiocb*)
{
	WARN_IF_ERR(ERR::NOT_SUPPORTED);
	errno = ENOSYS;
	return -1;
}
