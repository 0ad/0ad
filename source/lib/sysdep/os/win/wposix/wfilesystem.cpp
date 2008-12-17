#include "precompiled.h"
#include "wfilesystem.h"

#include "lib/allocators/allocators.h"		// single_calloc
#include "wposix_internal.h"
#include "wtime_internal.h"		// wtime_utc_filetime_to_time_t
#include "crt_posix.h"			// _rmdir, _access

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
	char root_path[MAX_PATH] = "c:\\";	// default in case GCD fails
	DWORD gcd_ret = GetCurrentDirectory(sizeof(root_path), root_path);
	debug_assert(gcd_ret != 0);
		// if this fails, no problem - we have the default from above.
	root_path[3] = '\0';	// cut off after "c:\"

	char fs_name[32] = {0};
	BOOL ret = GetVolumeInformation(root_path, 0,0,0,0,0, fs_name, sizeof(fs_name));
	fs_name[ARRAY_SIZE(fs_name)-1] = '\0';
	debug_assert(ret != 0);
		// if this fails, no problem - we really only care if fs is FAT,
		// and will assume that's not the case (since fs_name != "FAT").

	filesystem = FS_UNKNOWN;

	if(!strncmp(fs_name, "FAT", 3))	// e.g. FAT32
		filesystem = FS_FAT;
	else if(!strcmp(fs_name, "NTFS"))
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


/*
// currently only sets st_mode (file or dir) and st_size.
int stat(const char* fn, struct stat* s)
{
	memset(s, 0, sizeof(struct stat));

	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(!GetFileAttributesEx(fn, GetFileExInfoStandard, &fad))
		return -1;

	s->st_mtime = filetime_to_time_t(fad.ftLastAccessTime)

	// dir
	if(fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		s->st_mode = S_IFDIR;
	else
	{
		s->st_mode = S_IFREG;
		s->st_size = (off_t)((((u64)fad.nFileSizeHigh) << 32) | fad.nFileSizeLow);
	}

	return 0;
}
*/


int access(const char* path, int mode)
{
	return _access(path, mode);
}


#ifndef HAVE_MKDIR
int mkdir(const char* path, mode_t UNUSED(mode))
{
	if(!CreateDirectory(path, (LPSECURITY_ATTRIBUTES)NULL))
	{
		return -1;
	}

	return 0;
}
#endif


int rmdir(const char* path)
{
	return _rmdir(path);
}


//-----------------------------------------------------------------------------
// readdir
//-----------------------------------------------------------------------------

// note: we avoid opening directories or returning entries that have
// hidden or system attributes set. this is to prevent returning something
// like "\System Volume Information", which raises an error upon opening.

// 0-initialized by wdir_alloc for safety; this is required for
// num_entries_scanned.
struct WDIR
{
	HANDLE hFind;

	// the dirent returned by readdir.
	// note: having only one global instance is not possible because
	// multiple independent opendir/readdir sequences must be supported.
	struct dirent ent;

	WIN32_FIND_DATA fd;

	// since opendir calls FindFirstFile, we need a means of telling the
	// first call to readdir that we already have a file.
	// that's the case iff this is == 0; we use a counter rather than a
	// flag because that allows keeping statistics.
	int num_entries_scanned;
};


// suballocator - satisfies most requests with a reusable static instance,
// thus speeding up allocation and avoiding heap fragmentation.
// thread-safe.

static WDIR global_wdir;
static uintptr_t global_wdir_is_in_use;

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
static bool is_normal_dir(const char* path)
{
	const DWORD fa = GetFileAttributes(path);

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


DIR* opendir(const char* path)
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

	// build search path for FindFirstFile. note: "path\\dir" only returns
	// information about that directory; trailing slashes aren't allowed.
	// for dir entries to be returned, we have to append "\\*".
	char search_path[PATH_MAX];
	snprintf(search_path, ARRAY_SIZE(search_path), "%s\\*", path);

	// note: we could store search_path and defer FindFirstFile until
	// readdir. this way is a bit more complex but required for
	// correctness (we must return a valid DIR iff <path> is valid).
	d->hFind = FindFirstFileA(search_path, &d->fd);
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


struct dirent* readdir(DIR* d_)
{
	WDIR* const d = (WDIR*)d_;

	// avoid polluting the last error.
	DWORD prev_err = GetLastError();

	// first call - skip FindNextFile (see opendir).
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
		if(!FindNextFileA(d->hFind, &d->fd))
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
	// FindNextFile failed; determine why and bail.
	// .. legit, end of dir reached. don't pollute last error code.
	if(GetLastError() == ERROR_NO_MORE_FILES)
		SetLastError(prev_err);
	else
		debug_assert(0);	// readdir: FindNextFile failed
	return 0;
}


// return status for the dirent returned by the last successful
// readdir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by dir_ForEachSortedEntry.
int readdir_stat_np(DIR* d_, struct stat* s)
{
	WDIR* d = (WDIR*)d_;

	memset(s, 0, sizeof(*s));
	s->st_size  = (off_t)u64_from_u32(d->fd.nFileSizeHigh, d->fd.nFileSizeLow);
	s->st_mode  = (unsigned short)((d->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? S_IFDIR : S_IFREG);
	s->st_mtime = filetime_to_time_t(&d->fd.ftLastWriteTime);
	return 0;
}


int closedir(DIR* d_)
{
	WDIR* const d = (WDIR*)d_;

	FindClose(d->hFind);

	wdir_free(d);
	return 0;
}


//-----------------------------------------------------------------------------

char* realpath(const char* fn, char* path)
{
	if(!GetFullPathName(fn, PATH_MAX, path, 0))
		return 0;
	return path;
}
