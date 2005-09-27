// misc. POSIX routines for Win32
//
// Copyright (c) 2004-2005 Jan Wassenberg
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

#include <stdio.h>
#include <stdlib.h>

#include "lib.h"
#include "posix.h"
#include "win_internal.h"
#include "sysdep/cpu.h"


// cast intptr_t to HANDLE; centralized for easier changing, e.g. avoiding
// warnings. i = -1 converts to INVALID_HANDLE_VALUE (same value).
static HANDLE HANDLE_from_intptr(intptr_t i)
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
	WARN_ERR(aio_reopen(fd, fn, oflag));

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
	return _read(fd, buf, nbytes);
}

int write(int fd, void* buf, size_t nbytes)
{
	return _write(fd, buf, nbytes);
}



int ioctl(int fd, int op, int* data)
{
	const HANDLE h = HANDLE_from_intptr(_get_osfhandle(fd));

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


// from wtime
extern time_t time_t_from_local_filetime(FILETIME* ft);
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
		return time_t_from_local_filetime(&local_ft);
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


// suballocator - satisfies most requests with a reusable static instance.
// this avoids hundreds of alloc/free which would fragment the heap.
// to guarantee thread-safety, we fall back to malloc if the instance is
// already in use. (it's important to avoid suprises since this is such a
// low-level routine).

static WDIR global_wdir;
static uintptr_t global_wdir_is_in_use;

// zero-initializes the WDIR (code below relies on this)
static inline WDIR* wdir_alloc()
{
	WDIR* d;

	// successfully reserved the global instance
	if(CAS(&global_wdir_is_in_use, 0, 1))
	{
		d = &global_wdir;
		memset(d, 0, sizeof(*d));
	}
	else
		d = (WDIR*)calloc(1, sizeof(WDIR));

	return d;
}

static inline void wdir_free(WDIR* d)
{
	if(d == &global_wdir)
		global_wdir_is_in_use = 0;
	else
		free(d);
}


static const DWORD hs = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;

// make sure path exists and is a normal (according to attributes) directory.
static bool is_normal_dir(const char* path)
{
	const DWORD fa = GetFileAttributes(path);
	// .. path not found
	if(fa == INVALID_FILE_ATTRIBUTES)
		return false;
	// .. not a directory
	if((fa & FILE_ATTRIBUTE_DIRECTORY) == 0)
		return false;
	// .. hidden or system attribute(s) set
	if((fa & hs) != 0)
		return false;
	return true;
}


DIR* opendir(const char* path)
{
	if(!is_normal_dir(path))
	{
		errno = ENOENT;
fail:
		debug_warn("opendir failed");
		return 0;
	}

	WDIR* d = wdir_alloc();
	if(!d)
	{
		errno = ENOMEM;
		goto fail;
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
		switch(GetLastError())
		{
		// not an error - the directory is just empty.
		case ERROR_NO_MORE_FILES:
			goto success;
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			errno = ENOENT;
			break;
		case ERROR_NOT_ENOUGH_MEMORY:
			errno = ENOMEM;
			break;
		default:
			errno = EINVAL;
			break;
		}

		// unfortunately there's no way around this; we need to allocate
		// d before FindFirstFile because it uses d->fd. copying from a
		// temporary isn't nice either (this free doesn't happen often)
		wdir_free(d);
		goto fail;
	}

success:
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
		debug_warn("readdir: FindNextFile failed");
	return 0;
}


// return status for the dirent returned by the last successful
// readdir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by file_enum.
int readdir_stat_np(DIR* d_, struct stat* s)
{
	WDIR* d = (WDIR*)d_;

	memset(s, 0, sizeof(struct stat));
	s->st_size  = (off_t)u64_from_u32(d->fd.nFileSizeHigh, d->fd.nFileSizeLow);
	s->st_mode  = (d->fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)? S_IFDIR : S_IFREG;
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
	if(fd >= 2)
		return -1;
	HANDLE h = std_h[fd];

	DWORD mode;
	GetConsoleMode(h, &mode);
	termios_p->c_lflag = mode & (ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT);

	return 0;
}


int tcsetattr(int fd, int /* optional_actions */, const struct termios* termios_p)
{
	if(fd >= 2)
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



// convert POSIX PROT_* flags to their Win32 PAGE_* enumeration equivalents.
// used by mprotect.
static DWORD win32_prot(int prot)
{
	// this covers all 8 combinations of read|write|exec
	// (note that "none" means all flags are 0).
	switch(prot & (PROT_READ|PROT_WRITE|PROT_EXEC))
	{
	case PROT_NONE:
		return PAGE_NOACCESS;
	case PROT_READ:
		return PAGE_READONLY;
	case PROT_WRITE:
		// not supported by Win32; POSIX allows us to also grant read access.
		return PAGE_READWRITE;
	case PROT_EXEC:
		return PAGE_EXECUTE;
	case PROT_READ|PROT_WRITE:
		return PAGE_READWRITE;
	case PROT_READ|PROT_EXEC:
		return PAGE_EXECUTE_READ;
	case PROT_WRITE|PROT_EXEC:
		// not supported by Win32; POSIX allows us to also grant read access.
		return PAGE_EXECUTE_READWRITE;
	case PROT_READ|PROT_WRITE|PROT_EXEC:
		return PAGE_EXECUTE_READWRITE;
	default:
		UNREACHABLE;
	}
}


int mprotect(void* addr, size_t len, int prot)
{
	const DWORD flNewProtect = win32_prot(prot);
	DWORD flOldProtect;	// required by VirtualProtect
	BOOL ok = VirtualProtect(addr, len, flNewProtect, &flOldProtect);
	WARN_RETURN_IF_FALSE(ok);
	return 0;
}


// called when flags & MAP_ANONYMOUS
static int mmap_mem(void* start, size_t len, int prot, int flags, int fd, void** pp)
{
	// sanity checks. we don't care about these but enforce them to
	// ensure callers are compatible with mmap.
	// .. MAP_ANONYMOUS is documented to require this.
	debug_assert(fd == -1);
	// .. if MAP_SHARED, writes are to change "the underlying [mapped]
	//    object", but there is none here (we're backed by the page file).
	debug_assert(flags & MAP_PRIVATE);

	// see explanation at MAP_NORESERVE definition.
	bool want_commit = (prot != PROT_NONE && !(flags & MAP_NORESERVE));

	if(!want_commit && start != 0 && flags & MAP_FIXED)
	{
		MEMORY_BASIC_INFORMATION mbi;
		BOOL ok = VirtualQuery(start, &mbi, len);
debug_assert(ok);	// todo4
		DWORD state = mbi.State;
		if(state == MEM_COMMIT)
		{
			ok = VirtualFree(start, len, MEM_DECOMMIT);
debug_assert(ok);
			*pp = 0;
			return 0;
		}
	}

	DWORD op = want_commit? MEM_COMMIT : MEM_RESERVE;

	DWORD flProtect = win32_prot(prot);
	void* p = VirtualAlloc(start, len, op, flProtect);
	if(!p)
		return ERR_NO_MEM;
	*pp = p;
	return 0;
}


// given mmap prot and flags, output protection/access values for use with
// CreateFileMapping / MapViewOfFile. they only support read-only,
// read/write and copy-on-write, so we dumb it down to that and later
// set the correct (and more restrictive) permission via mprotect.
static int mmap_file_access(int prot, int flags, DWORD& flProtect, DWORD& dwAccess)
{
	// assume read-only; other cases handled below.
	flProtect = PAGE_READONLY;
	dwAccess  = FILE_MAP_READ;

	if(prot & PROT_WRITE)
	{
		// determine write behavior: (whether they change the underlying file)
		switch(flags & (MAP_SHARED|MAP_PRIVATE))
		{
		// .. changes are written to file.
		case MAP_SHARED:
			flProtect = PAGE_READWRITE;
			dwAccess  = FILE_MAP_WRITE;	// read and write
			break;
		// .. copy-on-write mapping; writes do not affect the file.
		case MAP_PRIVATE:
			flProtect = PAGE_WRITECOPY;
			dwAccess  = FILE_MAP_COPY;
			break;
		// .. either none or both of the flags are set. the latter is
		//    definitely illegal according to POSIX and some man pages
		//    say exactly one must be set, so abort.
		default:
			return ERR_INVALID_PARAM;
		}
	}

	return 0;
}


static int mmap_file(void* start, size_t len, int prot, int flags,
	int fd, off_t ofs, void** pp)
{
	debug_assert(fd != -1);	// handled by mmap_mem

	WIN_SAVE_LAST_ERROR;

	HANDLE hFile = HANDLE_from_intptr(_get_osfhandle(fd));
	if(hFile == INVALID_HANDLE_VALUE)
		return ERR_INVALID_PARAM;

	// MapViewOfFileEx will fail if the "suggested" base address is
	// nonzero but cannot be honored, so wipe out <start> unless MAP_FIXED.
	if(!(flags & MAP_FIXED))
		start = 0;

	// choose protection and access rights for CreateFileMapping /
	// MapViewOfFile. these are weaker than what PROT_* allows and
	// are augmented below by subsequently mprotect-ing.
	DWORD flProtect; DWORD dwAccess;
	RETURN_ERR(mmap_file_access(prot, flags, flProtect, dwAccess));

	// enough foreplay; now actually map.
	const HANDLE hMap = CreateFileMapping(hFile, 0, flProtect, 0, 0, (LPCSTR)0);
	// .. create failed; bail now to avoid overwriting the last error value.
	if(!hMap)
		return ERR_NO_MEM;
	const DWORD ofs_hi = u64_hi(ofs), ofs_lo = u64_lo(ofs);
	void* p = MapViewOfFileEx(hMap, dwAccess, ofs_hi, ofs_lo, (SIZE_T)len, start);
	// .. make sure we got the requested address if MAP_FIXED was passed.
	debug_assert(!(flags & MAP_FIXED) || (p == start));
	// .. free the mapping object now, so that we don't have to hold on to its
	//    handle until munmap(). it's not actually released yet due to the
	//    reference held by MapViewOfFileEx (if it succeeded).
	CloseHandle(hMap);
	// .. map failed; bail now to avoid "restoring" the last error value.
	if(!p)
		return ERR_NO_MEM;

	// slap on correct (more restrictive) permissions. 
	(void)mprotect(p, len, prot);

	WIN_RESTORE_LAST_ERROR;
	*pp = p;
	return 0;
}


void* mmap(void* start, size_t len, int prot, int flags, int fd, off_t ofs)
{
	void* p;
	int err;
	if(flags & MAP_ANONYMOUS)
		err = mmap_mem(start, len, prot, flags, fd, &p);
	else
		err = mmap_file(start, len, prot, flags, fd, ofs, &p);
	if(err < 0)
	{
		WARN_ERR(err);
		return MAP_FAILED;
	}

	return p;
}


int munmap(void* start, size_t UNUSED(len))
{
	// UnmapViewOfFile checks if start was returned by MapViewOfFile*;
	// if not, it will fail.
	BOOL ok = UnmapViewOfFile(start);
	if(!ok)
		// VirtualFree requires dwSize to be 0 (entire region is released).
		ok = VirtualFree(start, 0, MEM_RELEASE);

	WARN_RETURN_IF_FALSE(ok);	// both failed
	return 0;
}


//-----------------------------------------------------------------------------
// DLL
//-----------------------------------------------------------------------------

static HMODULE HMODULE_from_void(void* handle)
{
	return (HMODULE)handle;
}

static void* void_from_HMODULE(HMODULE hModule)
{
	return (void*)hModule;
}


int dlclose(void* handle)
{
	BOOL ok = FreeLibrary(HMODULE_from_void(handle));
	WARN_RETURN_IF_FALSE(ok);
	return 0;
}


char* dlerror(void)
{
	return 0;
}


void* dlopen(const char* so_name, int flags)
{
	if(flags & RTLD_GLOBAL)
		debug_warn("dlopen: unsupported flag(s)");

	// if present, strip .so extension; add .dll extension
	char dll_name[MAX_PATH];
	strcpy_s(dll_name, ARRAY_SIZE(dll_name)-4, so_name);
	char* ext = strrchr(dll_name, '.');
	if(!ext)
		ext = dll_name + strlen(dll_name);
	strcpy(ext, ".dll");	// safe

	HMODULE hModule = LoadLibrary(dll_name);
	if(!hModule)
		debug_warn("dlopen failed");
	return void_from_HMODULE(hModule);
}


void* dlsym(void* handle, const char* sym_name)
{
	HMODULE hModule = HMODULE_from_void(handle);
	void* sym = GetProcAddress(hModule, sym_name);
	if(!sym)
		debug_warn("dlsym failed");
	return sym;
}


//-----------------------------------------------------------------------------


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
	// note: don't add _SC_PAGE_SIZE - they are different names but
	// have the same value.
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
