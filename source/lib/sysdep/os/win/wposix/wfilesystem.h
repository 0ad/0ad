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

#ifndef INCLUDED_WFILESYSTEM
#define INCLUDED_WFILESYSTEM

#include "no_crt_posix.h"


//
// sys/stat.h
//

// (use VC8's stat because it defines helpful inline macros)
#include <sys/stat.h>

#if MSC_VERSION
// defined by MinGW but not VC
typedef unsigned int mode_t;

// we need 64-bit st_size and time_t currently defaults to 64-bit
#define stat _stat64
#endif

// permission masks when creating files (_wsopen_s doesn't distinguish
// between owner/user/group)
#define S_IRWXO _S_IREAD|_S_IWRITE
#define S_IRWXU _S_IREAD|_S_IWRITE
#define S_IRWXG _S_IREAD|_S_IWRITE

#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISREG(m) (m & S_IFREG)

// we need to emulate this on VC7 (not included) and VC8 (deprecated)
#if MSC_VERSION
# define EMULATE_MKDIR 1
#else
# define EMULATE_MKDIR 0
#endif

#if EMULATE_MKDIR
extern int mkdir(const char* path, mode_t mode);
#endif


//
// dirent.h
//

typedef void DIR;

struct dirent
{
	// note: SUSv3 describes this as a "char array" but of unspecified size.
	// since that precludes using sizeof(), we may as well declare as a
	// pointer to avoid copying in the implementation.
	char* d_name;
};

extern DIR* opendir(const char* name);
extern struct dirent* readdir(DIR*);
extern int closedir(DIR*);

// return status for the file returned by the last successful
// readdir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by dir_ForEachSortedEntry.
extern int readdir_stat_np(DIR*, struct stat*);


//
// <stdlib.h>
//

extern char* realpath(const char*, char*);


//
// <unistd.h>
//

// values from MS _access() implementation. do not change.
#define F_OK 0
#define R_OK 4
#define W_OK 2
// .. MS implementation doesn't support this distinction.
//    hence, the file is reported executable if it exists.
#define X_OK 0

extern int access(const char* path, int mode);
extern int rmdir(const char* path);

#endif	// #ifndef INCLUDED_WFILESYSTEM
