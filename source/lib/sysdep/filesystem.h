/* Copyright (C) 2010 Wildfire Games.
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
 * wchar_t versions of POSIX filesystem functions
 */

#ifndef INCLUDED_FILESYSTEM
#define INCLUDED_FILESYSTEM


//
// dirent.h
//

struct WDIR;

struct wdirent
{
	// note: SUSv3 describes this as a "char array" but of unspecified size.
	// since that precludes using sizeof(), we may as well declare as a
	// pointer to avoid copying in the implementation.
	wchar_t* d_name;
};

extern WDIR* wopendir(const wchar_t* path);

extern struct wdirent* wreaddir(WDIR*);

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

// POSIX flags not supported by the underlying Win32 _wsopen_s:
#if OS_WIN
#define O_NONBLOCK     0x1000000
#endif

extern int wopen(const wchar_t* pathname, int oflag, ...);
extern int wclose(int fd);


//
// unistd.h
//

LIB_API int wtruncate(const wchar_t* pathname, off_t length);

LIB_API int wunlink(const wchar_t* pathname);

LIB_API int wrmdir(const wchar_t* path);


//
// stdlib.h
//

LIB_API wchar_t* wrealpath(const wchar_t* pathname, wchar_t* resolved);


//
// sys/stat.h
//

LIB_API int wstat(const wchar_t* pathname, struct stat* buf);

LIB_API int wmkdir(const wchar_t* path, mode_t mode);

#endif	// #ifndef INCLUDED_FILESYSTEM
