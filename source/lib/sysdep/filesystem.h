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

/*
 * wchar_t versions of POSIX filesystem functions
 */

#ifndef INCLUDED_FILESYSTEM
#define INCLUDED_FILESYSTEM

#include "lib/os_path.h"
#include "lib/posix/posix_filesystem.h"	// mode_t


//
// dirent.h
//

struct WDIR;

struct wdirent
{
	// note: SUSv3 describes this as a "char array" but of unspecified size.
	// we declare as a pointer to avoid having to copy the string.
	wchar_t* d_name;
};

extern WDIR* wopendir(const OsPath& path);

extern wdirent* wreaddir(WDIR*);

// return status for the file returned by the last successful
// wreaddir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by dir_ForEachSortedEntry.
extern int wreaddir_stat_np(WDIR*, struct stat*);

extern int wclosedir(WDIR*);


//
// fcntl.h
//

// Win32 _wsopen_s flags not specified by POSIX:
#define O_TEXT_NP      0x4000  // file mode is text (translated)
#define O_BINARY_NP    0x8000  // file mode is binary (untranslated)

// waio flags not specified by POSIX nor implemented by Win32 _wsopen_s:
// do not open a separate AIO-capable handle.
// (this can be used for small files where AIO overhead isn't worthwhile,
// thus speeding up loading and reducing resource usage.)
#define O_NO_AIO_NP    0x20000

extern int wopen(const OsPath& pathname, int oflag);
extern int wopen(const OsPath& pathname, int oflag, mode_t mode);
extern int wclose(int fd);


//
// unistd.h
//

// waio requires offsets and sizes to be multiples of the sector size.
// to allow arbitrarily sized files, we truncate them after I/O.
// however, ftruncate cannot be used since it is also subject to the
// sector-alignment requirement. instead, the file must be closed and
// this function called.
LIB_API int wtruncate(const OsPath& pathname, off_t length);

LIB_API int wunlink(const OsPath& pathname);

LIB_API int wrmdir(const OsPath& path);


//
// stdio.h
//

LIB_API int wrename(const OsPath& pathnameOld, const OsPath& pathnameNew);


//
// stdlib.h
//

LIB_API OsPath wrealpath(const OsPath& pathname);


//
// sys/stat.h
//

LIB_API int wstat(const OsPath& pathname, struct stat* buf);

LIB_API int wmkdir(const OsPath& path, mode_t mode);

#endif	// #ifndef INCLUDED_FILESYSTEM
