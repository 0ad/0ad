// misc. POSIX routines for Win32
//
// Copyright (c) 2004 Jan Wassenberg
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

// collection of hacks :P

#include "precompiled.h"

#include "lib.h"
#include "posix.h"
#include "win_internal.h"


#include <stdio.h>
#include <stdlib.h>


// cast intptr_t to HANDLE; centralized for easier changing, e.g. avoiding
// warnings. i = -1 converts to INVALID_HANDLE_VALUE (same value).
static HANDLE cast_to_HANDLE(intptr_t i)
{
	return (HANDLE)((char*)0 + i);
}


//////////////////////////////////////////////////////////////////////////////
//
// file
//
//////////////////////////////////////////////////////////////////////////////


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

#ifdef PARANOIA
debug_printf("open %s = %d\n", fn, fd);
#endif

	// cases when we don't want to open a second AIO-capable handle:
	// .. stdin/stdout/stderr
	if(fd <= 2)
		goto no_aio;
	// .. COM port - we don't currently need AIO access for those, and
	//    aio_reopen's CreateFile would fail with "access denied".
	if(is_com_port)
		goto no_aio;
	// .. caller is requesting we skip it (see file_open)
	if(oflag & O_NO_AIO_NP)
		goto no_aio;

	// none of the above apply; now re-open the file.
	// note: this is possible because _open defaults to DENY_NONE sharing.
	aio_reopen(fd, fn, oflag);

no_aio:

	// CRT doesn't like more than 255 files open.
	// warn now, so that we notice why so many are open.
#ifndef NDEBUG
	if(fd > 256)
		debug_warn("wposix: too many files open (CRT limitation)");
#endif

	return fd;
}


int close(int fd)
{
#ifdef PARANOIA
debug_printf("close %d\n", fd);
#endif

	debug_assert(3 <= fd && fd < 256);

	// note: there's no good way to notify us that <fd> wasn't opened for
	// AIO, so we could skip aio_close. storing a bit in the fd is evil and
	// a fd -> info map is redundant (waio already has one).
	// therefore, we require aio_close to fail gracefully.
	aio_close(fd);

	return _close(fd);
}


// we don't want to #define read to _read, since that's a fairly common
// identifier. therefore, translate from MS CRT names via thunk functions.
// efficiency is less important, and the overhead could be optimized away.

int read(int fd, void* buf, size_t nbytes)
{
	return _read(fd, buf, nbytes);
}

int write(int fd, void* buf, size_t nbytes)
{
	return _write(fd, buf, nbytes);
}



int ioctl(int fd, int op, int* data)
{
	const HANDLE h = cast_to_HANDLE(_get_osfhandle(fd));

	switch(op)
	{
	case TIOCMGET:
		/* TIOCM_* mapped directly to MS_*_ON */
		GetCommModemStatus(h, (DWORD*)data);
		break;

	case TIOCMBIS:
		/* only RTS supported */
		if(*data & TIOCM_RTS)
			EscapeCommFunction(h, SETRTS);
		else
			EscapeCommFunction(h, CLRRTS);
		break;

	case TIOCMIWAIT:
		static DWORD mask;
		DWORD new_mask = 0;
		if(*data & TIOCM_CD)
			new_mask |= EV_RLSD;
		if(*data & TIOCM_CTS)
			new_mask |= EV_CTS;
		if(new_mask != mask)
			SetCommMask(h, mask = new_mask);
		WaitCommEvent(h, &mask, 0);
		break;
	}

	return 0;
}




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

	char fs_name[32];
	BOOL ret = GetVolumeInformation(root_path, 0,0,0,0,0, fs_name, sizeof(fs_name));
	debug_assert(ret != 0);
		// if this fails, no problem - we really only care if fs is FAT,
		// and will assume that's not the case (since fs_name != "FAT").

	filesystem = FS_UNKNOWN;

	if(!strncmp(fs_name, "FAT", 3))	// e.g. FAT32
		filesystem = FS_FAT;
	else if(!strcmp(fs_name, "NTFS"))
		filesystem = FS_NTFS;
}


// from wtime
extern time_t local_filetime_to_time_t(FILETIME* ft);
extern time_t utc_filetime_to_time_t(FILETIME* ft);

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
		return local_filetime_to_time_t(&local_ft);
	}

	return utc_filetime_to_time_t(ft);
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


//////////////////////////////////////////////////////////////////////////////
//
// dir
//
//////////////////////////////////////////////////////////////////////////////

#undef getcwd

char* getcwd(char* buf, size_t buf_size)
{
	return _getcwd(buf, buf_size);
}

char* realpath(const char* fn, char* path)
{
	if(!GetFullPathName(fn, PATH_MAX, path, 0))
		return 0;
	return path;
}


int mkdir(const char* path, mode_t)
{
	return CreateDirectory(path, 0)? 0 : -1;
}


// opendir/readdir/closedir
//
// implementation rationale:
//
// opendir only performs minimal error checks (does directory exist?);
// readdir calls FindFirstFile. this is to ensure correct handling
// of empty directories. we need to store the path in WDIR anyway
// for filetime_to_time_t.
// 
// we avoid opening directories or returning files that have hidden or system
// attributes set. this is to prevent returning something like
// "\system volume information", which raises an error upon opening.

struct WDIR
{
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	struct dirent ent;
		// can't be global - must not be overwritten
		// by calls from different DIRs.

	char path[PATH_MAX+1];
		// can't be stored in fd or ent's path fields -
		// needed by each readdir_stat_np (for filetime_to_time_t).
};


static const DWORD hs = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	// convenience

DIR* opendir(const char* path)
{
	// make sure path exists and is a normal directory (see rationale above).
	// note: this is the only error check we can do here -
	// FindFirstFile is called in readdir (see rationale above).
	DWORD fa = GetFileAttributes(path);
	if((fa == INVALID_FILE_ATTRIBUTES) || !(fa & FILE_ATTRIBUTE_DIRECTORY) || (fa & hs))
		return 0;

	// note: zero-initializes everything (required).
	WDIR* d = (WDIR*)calloc(1, sizeof(WDIR));
	if(!d)
		return 0;

	// note: "path\\dir" only returns information about that directory;
	// trailing slashes aren't allowed. we have to append "\\*" to find files.
	snprintf(d->path, sizeof(d->path)-1, "%s\\*", path);

	return d;
}


struct dirent* readdir(DIR* d_)
{
	WDIR* const d = (WDIR*)d_;

	DWORD prev_err = GetLastError();

	// bails if end of dir reached or error.
	// called (again) if entry was rejected below.
get_another_entry:

	// first time
	if(d->hFind == 0)
	{
		d->hFind = FindFirstFile(d->path, &d->fd);
		if(d->hFind != INVALID_HANDLE_VALUE)    // success
			goto have_entry;
	}
	else
		if(FindNextFile(d->hFind, &d->fd))      // success
			goto have_entry;

	// Find*File failed; determine why and bail.
	// .. legit, end of dir reached. don't pollute last error code.
	if(GetLastError() == ERROR_NO_MORE_FILES)
		SetLastError(prev_err);
	else
		debug_warn("readdir: Find*File failed");
	return 0;


	// d->fd holds a valid entry, but we may have to get another below.
have_entry:

	// we must not return hidden or system entries, so get another.
	// (see rationale above).
	if(d->fd.dwFileAttributes & hs)
		goto get_another_entry;


	// this entry has passed all checks; return information about it.
	// .. d_ino zero-initialized by opendir
	// .. POSIX requires d_name to be an array, so we copy there.
	strcpy_s(d->ent.d_name, sizeof(d->ent.d_name), d->fd.cFileName);
	return &d->ent;
}


// return status for the dirent returned by the last successful
// readdir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by file_enum.
int readdir_stat_np(DIR* d_, struct stat* s)
{
	WDIR* d = (WDIR*)d_;

	memset(s, 0, sizeof(struct stat));
	s->st_size  = (off_t)((((u64)d->fd.nFileSizeHigh) << 32) | d->fd.nFileSizeLow);
	s->st_mode  = (d->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? S_IFDIR : S_IFREG;
	s->st_mtime = filetime_to_time_t(&d->fd.ftLastWriteTime);
	return 0;
}


int closedir(DIR* d_)
{
	WDIR* const d = (WDIR*)d_;

	FindClose(d->hFind);

	memset(d, 0, sizeof(WDIR));	// safety
	free(d);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// terminal
//
//////////////////////////////////////////////////////////////////////////////


static HANDLE std_h[2] = { (HANDLE)((char*)0 + 3), (HANDLE)((char*)0 + 7) };


void _get_console()
{
	AllocConsole();
}

void _hide_console()
{
	FreeConsole();
}


int tcgetattr(int fd, struct termios* termios_p)
{
	if(fd > 2)
		return -1;
	HANDLE h = std_h[fd];

	DWORD mode;
	GetConsoleMode(h, &mode);
	termios_p->c_lflag = mode & (ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT);

	return 0;
}


int tcsetattr(int fd, int /* optional_actions */, const struct termios* termios_p)
{
	if(fd > 2)
		return -1;
	HANDLE h = std_h[fd];
	SetConsoleMode(h, (DWORD)termios_p->c_lflag);
	FlushConsoleInputBuffer(h);

	return 0;
}


int poll(struct pollfd /* fds */[], int /* nfds */, int /* timeout */)
{
	return -1;
}


//////////////////////////////////////////////////////////////////////////////
//
// memory mapping
//
//////////////////////////////////////////////////////////////////////////////


void* mmap(void* user_start, size_t len, int prot, int flags, int fd, off_t offset)
{
	{
	WIN_SAVE_LAST_ERROR;

	// assume fd = -1 (requesting mapping backed by page file),
	// so that we notice invalid file handles below.
	HANDLE hFile = INVALID_HANDLE_VALUE;
	if(fd != -1)
	{
		hFile = cast_to_HANDLE(_get_osfhandle(fd));
		if(hFile == INVALID_HANDLE_VALUE)
		{
			debug_warn("mmap: invalid file handle");
			goto fail;
		}
	}

	// MapView.. will choose start address unless MAP_FIXED was specified.
	void* start = 0;
	if(flags & MAP_FIXED)
	{
		start = user_start;
		if(start == 0)	// debug_assert below would fire
			goto fail;
	}

	// figure out access rights.
	// note: reads are always allowed (Win32 limitation).

	SECURITY_ATTRIBUTES sec = { sizeof(SECURITY_ATTRIBUTES), (void*)0, FALSE };
	DWORD flProtect = PAGE_READONLY;
	DWORD dwAccess = FILE_MAP_READ;

	// .. no access: not possible on Win32.
	if(prot == PROT_NONE)
		goto fail;
	// .. write or read/write (Win32 doesn't support write-only)
	if(prot & PROT_WRITE)
	{
		flProtect = PAGE_READWRITE;

		const bool shared = (flags & MAP_SHARED ) != 0;
		const bool priv   = (flags & MAP_PRIVATE) != 0;
		// .. both aren't allowed
		if(shared && priv)
			goto fail;
		// .. changes are shared & written to file
		else if(shared)
		{
			sec.bInheritHandle = TRUE;
			dwAccess = FILE_MAP_ALL_ACCESS;
		}
		// .. private copy-on-write mapping
		else if(priv)
		{
			flProtect = PAGE_WRITECOPY;
			dwAccess = FILE_MAP_COPY;
		}
	}

	// now actually map.
	const DWORD len_hi = (DWORD)((u64)len >> 32);
		// careful! language doesn't allow shifting 32-bit types by 32 bits.
	const DWORD len_lo = (DWORD)len & 0xffffffff;
	const HANDLE hMap = CreateFileMapping(hFile, &sec, flProtect, len_hi, len_lo, (LPCSTR)0);
	if(hMap == INVALID_HANDLE_VALUE)
		// bail now so that MapView.. doesn't overwrite the last error value.
		goto fail;
	void* ptr = MapViewOfFileEx(hMap, dwAccess, len_hi, offset, len_lo, start);

	// free the mapping object now, so that we don't have to hold on to its
	// handle until munmap(). it's not actually released yet due to the
	// reference held by MapViewOfFileEx (if it succeeded).
	if(hMap != INVALID_HANDLE_VALUE)	// avoid "invalid handle" error
		CloseHandle(hMap);

	if(!ptr)
		// bail now, before the last error value is restored,
		// but after freeing the mapping object.
		goto fail;

	debug_assert(!(flags & MAP_FIXED) || (ptr == start));
		// fixed => ptr = start

	WIN_RESTORE_LAST_ERROR;

	return ptr;
	}
fail:
	return MAP_FAILED;
}


int munmap(void* start, size_t len)
{
	UNUSED(len);
	BOOL ok = UnmapViewOfFile(start);
	return ok? 0 : -1;
}




int uname(struct utsname* un)
{
	static OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&vi);

	// OS implementation name
	const char* family = "??";
	int ver = (vi.dwMajorVersion << 8) | vi.dwMinorVersion;
	if(vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		family = (ver == 0x045a)? "ME" : "9x";
	if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		if(ver == 0x0500)
			family = "2k";
		else if(ver == 0x0501)
			family = "XP";
		else
			family = "NT";
	}
	sprintf(un->sysname, "Win%s", family);

	// release info
	const char* vs = vi.szCSDVersion;
	int sp;
	if(sscanf(vs, "Service Pack %d", &sp) == 1)
		sprintf(un->release, "SP %d", sp);
	else
	{
		const char* release = "";
		if(vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		{
			if(!strcmp(vs, " C"))
				release = "OSR2";
			else if(!strcmp(vs, " A"))
				release = "SE";
		}
		strcpy(un->release, release);	// safe
	}

	// version
	sprintf(un->version, "%lu.%02lu.%lu", vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber & 0xffff);

	// node name
	DWORD buf_size = sizeof(un->nodename);
	DWORD last_err = GetLastError();
	BOOL ok = GetComputerName(un->nodename, &buf_size);
	// GetComputerName sets last error even on success - suppress.
	if(ok)
		SetLastError(last_err);
	else
		debug_warn("GetComputerName failed");

	// hardware type
	static SYSTEM_INFO si;
	GetSystemInfo(&si);
	if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		strcpy(un->machine, "AMD64");	// safe
	else
		strcpy(un->machine, "IA-32");	// safe

	return 0;
}


 




long sysconf(int name)
{
	// used by _SC_*_PAGES
	static DWORD page_size;
	static BOOL (WINAPI *pGlobalMemoryStatusEx)(MEMORYSTATUSEX*);  

	ONCE(
	{
		// get page size
		// (used by _SC_PAGESIZE and _SC_*_PAGES)
		SYSTEM_INFO si;
		GetSystemInfo(&si);		// can't fail => page_size always > 0.
		page_size = si.dwPageSize;

		// import GlobalMemoryStatusEx - it's not defined by the VC6 PSDK.
		// used by _SC_*_PAGES if available (provides better results).
		const HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");  
		*(void**)&pGlobalMemoryStatusEx = GetProcAddress(hKernel32Dll, "GlobalMemoryStatusEx"); 
		FreeLibrary(hKernel32Dll);
			// make sure the reference is released so BoundsChecker
			// doesn't complain. it won't actually be unloaded anyway -
			// there is at least one other reference.
	}
	);


	switch(name)
	{
	case _SC_PAGESIZE:
		return page_size;

	case _SC_PHYS_PAGES:
	case _SC_AVPHYS_PAGES:
		{
		u64 total_phys_mem;
		u64 avail_phys_mem;

		// first try GlobalMemoryStatus - cannot fail.
		// override its results if GlobalMemoryStatusEx is available.
		MEMORYSTATUS ms;
		GlobalMemoryStatus(&ms);
			// can't fail.
		total_phys_mem = ms.dwTotalPhys;
		avail_phys_mem = ms.dwAvailPhys;

		// newer API is available: use it to report correct results
		// (no overflow or wraparound) on systems with > 4 GB of memory.
		MEMORYSTATUSEX mse = { sizeof(mse) };
		if(pGlobalMemoryStatusEx && pGlobalMemoryStatusEx(&mse))
		{
			total_phys_mem = mse.ullTotalPhys;
			avail_phys_mem = mse.ullAvailPhys;
		}
		// else: not an error, since this isn't available before Win2k / XP.
		// we have results from GlobalMemoryStatus anyway.

		if(name == _SC_PHYS_PAGES)
			return (long)(round_up((uintptr_t)total_phys_mem, 2*MiB) / page_size);
				// Richter, "Programming Applications for Windows":
				// reported value doesn't include non-paged pool reserved
				// during boot; it's not considered available to kernel.
				// it's 528 KiB on my 512 MiB machine (WinXP and Win2k).
 		else
			return (long)(avail_phys_mem / page_size);
		}

	default:
		return -1;
	}
}
