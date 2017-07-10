/* Copyright (C) 2010 Wildfire Games.
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

#include "precompiled.h"
#include "lib/sysdep/filesystem.h"

#include "lib/sysdep/cpu.h"	// cpu_CAS
#include "lib/sysdep/os/win/wutil.h"	// StatusFromWin
#include "lib/sysdep/os/win/wposix/waio.h"	// waio_reopen
#include "lib/sysdep/os/win/wposix/wtime_internal.h"	// wtime_utc_filetime_to_time_t
#include "lib/sysdep/os/win/wposix/crt_posix.h"			// _close, _lseeki64 etc.


//-----------------------------------------------------------------------------
// WDIR suballocator
//-----------------------------------------------------------------------------

// most applications only need a single WDIR at a time. we avoid expensive
// heap allocations by reusing a single static instance. if it is already
// in use, we allocate further instances dynamically.
// NB: this is thread-safe due to CAS.

struct WDIR	// POD
{
	HANDLE hFind;

	WIN32_FIND_DATAW findData;	// indeterminate if hFind == INVALID_HANDLE_VALUE

	// wreaddir will return the address of this member.
	// (must be stored in WDIR to allow multiple independent
	// wopendir/wreaddir sequences).
	struct wdirent ent;

	// used by wreaddir to skip the first FindNextFileW. (a counter is
	// easy to test/update and also provides useful information.)
	size_t numCalls;
};

static WDIR wdir_storage;
static volatile intptr_t wdir_in_use;

static inline WDIR* wdir_alloc()
{
	if(cpu_CAS(&wdir_in_use, 0, 1))	// gained ownership
		return &wdir_storage;

	// already in use (rare) - allocate from heap
	return new WDIR;
}

static inline void wdir_free(WDIR* d)
{
	if(d == &wdir_storage)
	{
		const bool ok = cpu_CAS(&wdir_in_use, 1, 0);	// relinquish ownership
		ENSURE(ok);	// ensure it wasn't double-freed
	}
	else	// allocated from heap
		delete d;
}


//-----------------------------------------------------------------------------
// dirent.h
//-----------------------------------------------------------------------------

static bool IsValidDirectory(const OsPath& path)
{
	const DWORD fileAttributes = GetFileAttributesW(OsString(path).c_str());

	// path not found
	if(fileAttributes == INVALID_FILE_ATTRIBUTES)
		return false;

	// not a directory
	if((fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return false;

	// NB: no longer reject hidden or system attributes since
	// wsnd's add_oal_dlls_in_dir opens the Windows system directory,
	// which sometimes has these attributes set.

	return true;
}


WDIR* wopendir(const OsPath& path)
{
	WinScopedPreserveLastError s;

	if(!IsValidDirectory(path))
	{
		errno = ENOENT;
		return 0;
	}

	WDIR* d = wdir_alloc();
	d->numCalls = 0;

	// NB: "c:\\path" only returns information about that directory;
	// trailing slashes aren't allowed. append "\\*" to retrieve its entries.
	OsPath searchPath = path/"*";

	// (we don't defer FindFirstFileW until wreaddir because callers
	// expect us to return 0 if directory reading will/did fail.)
	d->hFind = FindFirstFileW(OsString(searchPath).c_str(), &d->findData);
	if(d->hFind != INVALID_HANDLE_VALUE)
		return d;	// success
	if(GetLastError() == ERROR_NO_MORE_FILES)
		return d;	// success, but directory is empty

	Status status = StatusFromWin();

	// release the WDIR allocated above (this is preferable to
	// always copying the large WDIR or findData from a temporary)
	wdir_free(d);

	WARN_IF_ERR(status);
	errno = ErrnoFromStatus(status);

	return 0;
}


struct wdirent* wreaddir(WDIR* d)
{
	// directory is empty and d->findData is indeterminate
	if(d->hFind == INVALID_HANDLE_VALUE)
		return 0;

	WinScopedPreserveLastError s;

	// until end of directory or a valid entry was found:
	for(;;)
	{
		if(d->numCalls++ != 0)	// (skip first call to FindNextFileW - see wopendir)
		{
			if(!FindNextFileW(d->hFind, &d->findData))
			{
				if(GetLastError() == ERROR_NO_MORE_FILES)
					SetLastError(0);
				else	// unexpected error
					DEBUG_WARN_ERR(StatusFromWin());
				return 0;	// end of directory or error
			}
		}

		// only accept non-hidden and non-system entries - otherwise,
		// callers might encounter errors when attempting to open them.
		if((d->findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_SYSTEM)) == 0)
		{
			d->ent.d_name = d->findData.cFileName;	// (NB: d_name is a pointer)
			return &d->ent;
		}
	}
}


int wreaddir_stat_np(WDIR* d, struct stat* s)
{
	// NTFS stores UTC but FAT stores local times, which are incorrectly
	// translated to UTC based on the _current_ DST settings. we no longer
	// bother checking the filesystem, since that's either unreliable or
	// expensive. timestamps may therefore be off after a DST transition,
	// which means our cached files would be regenerated.
	FILETIME* filetime = &d->findData.ftLastWriteTime;

	memset(s, 0, sizeof(*s));
	s->st_size  = (off_t)u64_from_u32(d->findData.nFileSizeHigh, d->findData.nFileSizeLow);
	s->st_mode  = (unsigned short)((d->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? S_IFDIR : S_IFREG);
	s->st_mtime = wtime_utc_filetime_to_time_t(filetime);
	return 0;
}


int wclosedir(WDIR* d)
{
	FindClose(d->hFind);

	wdir_free(d);
	return 0;
}


//-----------------------------------------------------------------------------
// fcntl.h
//-----------------------------------------------------------------------------

int wopen(const OsPath& pathname, int oflag)
{
	ENSURE(!(oflag & O_CREAT));	// must specify mode_arg if O_CREAT
	return wopen(OsString(pathname).c_str(), oflag, _S_IREAD|_S_IWRITE);
}


int wopen(const OsPath& pathname, int oflag, mode_t mode)
{
	if(oflag & O_DIRECT)
	{
		Status ret = waio_open(pathname, oflag);
		if(ret < 0)
		{
			errno = ErrnoFromStatus(ret);
			return -1;
		}
		return (int)ret;	// file descriptor
	}
	else
	{
		WinScopedPreserveLastError s;	// _wsopen_s's CreateFileW
		int fd;
		oflag |= _O_BINARY;
		if(oflag & O_WRONLY)
			oflag |= O_CREAT|O_TRUNC;
		// NB: _wsopen_s ignores mode unless oflag & O_CREAT
		errno_t ret = _wsopen_s(&fd, OsString(pathname).c_str(), oflag, _SH_DENYRD, mode);
		if(ret != 0)
		{
			errno = ret;
			return -1;	// NOWARN
		}
		return fd;
	}
}


int wclose(int fd)
{
	ENSURE(fd >= 3);	// not invalid nor stdin/out/err

	if(waio_close(fd) != 0)
		return _close(fd);
	return 0;
}


//-----------------------------------------------------------------------------
// unistd.h
//-----------------------------------------------------------------------------

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


int wtruncate(const OsPath& pathname, off_t length)
{
	// (re-open the file to avoid the FILE_FLAG_NO_BUFFERING
	// sector-alignment restriction)
	HANDLE hFile = CreateFileW(OsString(pathname).c_str(), GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	ENSURE(hFile != INVALID_HANDLE_VALUE);
	LARGE_INTEGER ofs; ofs.QuadPart = length;
	WARN_IF_FALSE(SetFilePointerEx(hFile, ofs, 0, FILE_BEGIN));
	WARN_IF_FALSE(SetEndOfFile(hFile));
	WARN_IF_FALSE(CloseHandle(hFile));
	return 0;
}


int wunlink(const OsPath& pathname)
{
	return _wunlink(OsString(pathname).c_str());
}


int wrmdir(const OsPath& path)
{
	return _wrmdir(OsString(path).c_str());
}


int wrename(const OsPath& pathnameOld, const OsPath& pathnameNew)
{
	return _wrename(OsString(pathnameOld).c_str(), OsString(pathnameNew).c_str());
}


OsPath wrealpath(const OsPath& pathname)
{
	wchar_t resolved[PATH_MAX];
	if(!GetFullPathNameW(OsString(pathname).c_str(), PATH_MAX, resolved, 0))
		return OsPath();
	return resolved;
}


static int ErrnoFromCreateDirectory()
{
	switch(GetLastError())
	{
	case ERROR_ALREADY_EXISTS:
		return EEXIST;
	case ERROR_PATH_NOT_FOUND:
		return ENOENT;
	case ERROR_ACCESS_DENIED:
		return EACCES;
	case ERROR_WRITE_PROTECT:
		return EROFS;
	case ERROR_DIRECTORY:
		return ENOTDIR;
	default:
		return 0;
	}
}

int wmkdir(const OsPath& path, mode_t UNUSED(mode))
{
	if(!CreateDirectoryW(OsString(path).c_str(), (LPSECURITY_ATTRIBUTES)NULL))
	{
		errno = ErrnoFromCreateDirectory();
		return -1;
	}

	return 0;
}


int wstat(const OsPath& pathname, struct stat* buf)
{
	return _wstat64(OsString(pathname).c_str(), buf);
}
