/* Copyright (c) 2010 Wildfire Games
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
#include "lib/sysdep/os/win/wposix/wfilesystem.h"

#include "lib/allocators/allocators.h"		// single_calloc
#include "lib/sysdep/os/win/wposix/wposix_internal.h"
#include "lib/sysdep/os/win/wposix/waio.h"
#include "lib/sysdep/os/win/wposix/wtime_internal.h"	// wtime_utc_filetime_to_time_t
#include "lib/sysdep/os/win/wposix/crt_posix.h"			// _rmdir, _access

//
// determine file system type on the current drive -
// needed to work around incorrect FAT time translation.
//

static enum Filesystem
{
	FS_INVALID,	// detect_filesystem() not yet called
	FS_FAT,		// FAT12, FAT16, or FAT32
	FS_NTFS,	// (most common)
	FS_UNKNOWN	// newer FS we don't know about
}
filesystem;


// rationale: the previous method of checking every path was way too slow
// (taking ~800ms total during init). instead, we only determine the FS once.
// this is quite a bit easier than intercepting chdir() calls and/or
// caching FS type per drive letter, but not foolproof.
//
// if some data files are on a different volume that is set up as FAT,
// the workaround below won't be triggered (=> timestamps may be off by
// 1 hour when DST is in effect). oh well, that is not a supported.
//
// the common case (everything is on a single NTFS volume) is more important
// and must run without penalty.


// called from the first filetime_to_time_t() call, not win.cpp init;
// this means we can rely on the current directory having been set to
// the app's directory (and therefore its appendant volume - see above).
static void detect_filesystem()
{
	wchar_t root_path[MAX_PATH] = L"c:\\";	// default in case GCD fails
	DWORD gcd_ret = GetCurrentDirectoryW(ARRAY_SIZE(root_path), root_path);
	debug_assert(gcd_ret != 0);
		// if this fails, no problem - we have the default from above.
	root_path[3] = '\0';	// cut off after "c:\"

	wchar_t fs_name[32] = {0};
	BOOL ret = GetVolumeInformationW(root_path, 0,0,0,0,0, fs_name, sizeof(fs_name));
	fs_name[ARRAY_SIZE(fs_name)-1] = '\0';
	debug_assert(ret != 0);
		// if this fails, no problem - we really only care if fs is FAT,
		// and will assume that's not the case (since fs_name != "FAT").

	filesystem = FS_UNKNOWN;

	if(!wcsncmp(fs_name, L"FAT", 3))	// e.g. FAT32
		filesystem = FS_FAT;
	else if(!wcscmp(fs_name, L"NTFS"))
		filesystem = FS_NTFS;
}


// convert local FILETIME (includes timezone bias and possibly DST bias)
// to seconds-since-1970 UTC.
//
// note: splitting into month, year etc. is inefficient,
//   but much easier than determining whether ft lies in DST,
//   and ourselves adding the appropriate bias.
//
// called for FAT file times; see wposix filetime_to_time_t.
time_t time_t_from_local_filetime(FILETIME* ft)
{
	SYSTEMTIME st;
	FileTimeToSystemTime(ft, &st);

	struct tm t;
	t.tm_sec   = st.wSecond;
	t.tm_min   = st.wMinute;
	t.tm_hour  = st.wHour;
	t.tm_mday  = st.wDay;
	t.tm_mon   = st.wMonth-1;
	t.tm_year  = st.wYear-1900;
	t.tm_isdst = -1;
	// let the CRT determine whether this local time
	// falls under DST by the US rules.
	return mktime(&t);
}


// convert Windows FILETIME to POSIX time_t (seconds-since-1970 UTC);
// used by stat and readdir_stat_np for st_mtime.
//
// works around a documented Windows bug in converting FAT file times
// (correct results are desired since VFS mount logic considers
// files 'equal' if their mtime and size are the same).
static time_t filetime_to_time_t(FILETIME* ft)
{
	ONCE(detect_filesystem());

	// the FAT file system stores local file times, while
	// NTFS records UTC. Windows does convert automatically,
	// but uses the current DST settings. (boo!)
	// we go back to local time, and convert properly.
	if(filesystem == FS_FAT)
	{
		FILETIME local_ft;
		FileTimeToLocalFileTime(ft, &local_ft);
		return time_t_from_local_filetime(&local_ft);
	}

	return wtime_utc_filetime_to_time_t(ft);
}





//-----------------------------------------------------------------------------
// dirent.h
//-----------------------------------------------------------------------------

// note: we avoid opening directories or returning entries that have
// hidden or system attributes set. this is to prevent returning something
// like "\System Volume Information", which raises an error upon opening.

// 0-initialized by wdir_alloc for safety; this is required for
// num_entries_scanned.
struct WDIR
{
	HANDLE hFind;

	// the wdirent returned by readdir.
	// note: having only one global instance is not possible because
	// multiple independent wopendir/wreaddir sequences must be supported.
	struct wdirent ent;

	WIN32_FIND_DATAW fd;

	// since wopendir calls FindFirstFileW, we need a means of telling the
	// first call to wreaddir that we already have a file.
	// that's the case iff this is == 0; we use a counter rather than a
	// flag because that allows keeping statistics.
	int num_entries_scanned;
};


// suballocator - satisfies most requests with a reusable static instance,
// thus speeding up allocation and avoiding heap fragmentation.
// thread-safe.

static WDIR global_wdir;
static intptr_t global_wdir_is_in_use;

// zero-initializes the WDIR (code below relies on this)
static inline WDIR* wdir_alloc()
{
	return (WDIR*)single_calloc(&global_wdir, &global_wdir_is_in_use, sizeof(WDIR));
}

static inline void wdir_free(WDIR* d)
{
	single_free(&global_wdir, &global_wdir_is_in_use, d);
}


static const DWORD hs = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

// make sure path exists and is a normal (according to attributes) directory.
static bool is_normal_dir(const wchar_t* path)
{
	const DWORD fa = GetFileAttributesW(path);

	// path not found
	if(fa == INVALID_FILE_ATTRIBUTES)
		return false;

	// not a directory
	if((fa & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return false;

	// hidden or system attribute(s) set
	// this check is now disabled because wsnd's add_oal_dlls_in_dir
	// needs to open the Windows system directory, which sometimes has
	// these attributes set.
	//if((fa & hs) != 0)
	//	return false;

	return true;
}


WDIR* wopendir(const wchar_t* path)
{
	if(!is_normal_dir(path))
	{
		errno = ENOENT;
		return 0;
	}

	WDIR* d = wdir_alloc();
	if(!d)
	{
		errno = ENOMEM;
		return 0;
	}

	// build search path for FindFirstFileW. note: "path\\dir" only returns
	// information about that directory; trailing slashes aren't allowed.
	// for dir entries to be returned, we have to append "\\*".
	wchar_t search_path[PATH_MAX];
	swprintf_s(search_path, ARRAY_SIZE(search_path), L"%ls\\*", path);

	// note: we could store search_path and defer FindFirstFileW until
	// wreaddir. this way is a bit more complex but required for
	// correctness (we must return a valid DIR iff <path> is valid).
	d->hFind = FindFirstFileW(search_path, &d->fd);
	if(d->hFind == INVALID_HANDLE_VALUE)
	{
		// not an error - the directory is just empty.
		if(GetLastError() == ERROR_NO_MORE_FILES)
			return d;

		// translate Win32 error to errno.
		LibError err = LibError_from_win32(FALSE);
		LibError_set_errno(err);

		// release the WDIR allocated above.
		// unfortunately there's no way around this; we need to allocate
		// d before FindFirstFile because it uses d->fd. copying from a
		// temporary isn't nice either (this free doesn't happen often)
		wdir_free(d);
		return 0;
	}

	return d;
}


struct wdirent* wreaddir(WDIR* d)
{
	// avoid polluting the last error.
	DWORD prev_err = GetLastError();

	// first call - skip FindNextFileW (see wopendir).
	if(d->num_entries_scanned == 0)
	{
		// this directory is empty.
		if(d->hFind == INVALID_HANDLE_VALUE)
			return 0;
		goto already_have_file;
	}

	// until end of directory or a valid entry was found:
	for(;;)
	{
		if(!FindNextFileW(d->hFind, &d->fd))
			goto fail;
already_have_file:

		d->num_entries_scanned++;

		// not a hidden or system entry -> it's valid.
		if((d->fd.dwFileAttributes & hs) == 0)
			break;
	}

	// this entry has passed all checks; return information about it.
	// (note: d_name is a pointer; see struct dirent definition)
	d->ent.d_name = d->fd.cFileName;
	return &d->ent;

fail:
	// FindNextFileW failed; determine why and bail.
	// .. legit, end of dir reached. don't pollute last error code.
	if(GetLastError() == ERROR_NO_MORE_FILES)
		SetLastError(prev_err);
	else
		WARN_ERR(LibError_from_GLE());
	return 0;
}


int wreaddir_stat_np(WDIR* d, struct stat* s)
{
	memset(s, 0, sizeof(*s));
	s->st_size  = (off_t)u64_from_u32(d->fd.nFileSizeHigh, d->fd.nFileSizeLow);
	s->st_mode  = (unsigned short)((d->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? S_IFDIR : S_IFREG);
	s->st_mtime = filetime_to_time_t(&d->fd.ftLastWriteTime);
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

int wopen(const wchar_t* pathname, int oflag)
{
	debug_assert(!(oflag & O_CREAT));
	return wopen(pathname, oflag, _S_IREAD|_S_IWRITE);
}

int wopen(const wchar_t* pathname, int oflag, mode_t mode_arg)
{
	mode_t mode = _S_IREAD|_S_IWRITE;
	if(oflag & O_CREAT)
		mode = mode_arg;

	WinScopedPreserveLastError s;	// _wsopen_s's CreateFileW
	int fd;
	errno_t ret = _wsopen_s(&fd, pathname, oflag, _SH_DENYNO, mode);
	if(ret != 0)
	{
		errno = ret;
		return -1;	// NOWARN
	}

	if(waio_reopen(fd, pathname, oflag) != INFO::OK)
		return -1;

	// CRT doesn't like more than 255 files open.
	// warn now, so that we notice why so many are open.
#ifndef NDEBUG
	if(fd > 256)
		WARN_ERR(ERR::LIMIT);
#endif

	return fd;
}


int wclose(int fd)
{
	debug_assert(3 <= fd && fd < 256);

	(void)waio_close(fd);	// no-op if fd wasn't opened for aio

	return _close(fd);
}


//-----------------------------------------------------------------------------
// unistd.h
//-----------------------------------------------------------------------------

int wtruncate(const wchar_t* pathname, off_t length)
{
	HANDLE hFile = CreateFileW(pathname, GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	debug_assert(hFile != INVALID_HANDLE_VALUE);
	LARGE_INTEGER ofs; ofs.QuadPart = length;
	WARN_IF_FALSE(SetFilePointerEx(hFile, ofs, 0, FILE_BEGIN));
	WARN_IF_FALSE(SetEndOfFile(hFile));
	WARN_IF_FALSE(CloseHandle(hFile));
	return 0;
}


int wunlink(const wchar_t* pathname)
{
	return _wunlink(pathname);
}


int wrmdir(const wchar_t* path)
{
	return _wrmdir(path);
}


int wrename(const wchar_t* pathnameOld, const wchar_t* pathnameNew)
{
	return _wrename(pathnameOld, pathnameNew);
}


wchar_t* wrealpath(const wchar_t* pathname, wchar_t* resolved)
{
	if(!GetFullPathNameW(pathname, PATH_MAX, resolved, 0))
		return 0;
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

int wmkdir(const wchar_t* path, mode_t UNUSED(mode))
{
	if(!CreateDirectoryW(path, (LPSECURITY_ATTRIBUTES)NULL))
	{
		errno = ErrnoFromCreateDirectory();
		return -1;
	}

	return 0;
}


int wstat(const wchar_t* pathname, struct stat* buf)
{
	return _wstat64(pathname, buf);
}
