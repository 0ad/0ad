// misc. POSIX routines for Win32
//
// Copyright (c) 2003 Jan Wassenberg
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

// note: try to avoid redefining CRT functions - if building against the
// DLL CRT, the declarations will be incompatible. adding _CRTIMP to the decl
// is a last resort (e.g. if the regular CRT headers would conflict).


#ifndef __WPOSIX_H__
#define __WPOSIX_H__


// split out of this module.
#include "wposix_types.h"
#include "waio.h"
#include "wsock.h"
#include "wtime.h"
#include "wpthread.h"


#ifdef __cplusplus
extern "C" {
#endif


// we define some CRT functions (e.g. access), because they're otherwise
// only brought in by win-specific headers (here, <io.h>).
// define correctly for static or DLL CRT in case the original header
// is included, to avoid conflict warnings.
#ifndef _CRTIMP
# ifdef  _DLL
#  define _CRTIMP __declspec(dllimport)
# else
#  define _CRTIMP
# endif 
#endif


//
// <limits.h>
//

#define PATH_MAX 255
// Win32 MAX_PATH is 260

#ifdef _WIN32
# ifndef SIZE_MAX // VC2005 already defines this in limits.h
#  define SIZE_MAX 0xffffffff
# endif
#else
# define SIZE_MAX 0xffffffffffffffff
#endif


//
// <errno.h>
//

#include <errno.h>

// not defined there:
#define EINPROGRESS 100000
#define ETIMEDOUT            (60)	// matches NetworkInternal.h def

/*
enum
{
EINPROGRESS = 1000,   // Operation in progress. 
ENOMEM        // Not enough space. 
EACCES,        // Permission denied. 
EADDRINUSE,    // Address in use. 
EADDRNOTAVAIL, // Address not available. 
EAGAIN,        // Resource unavailable, try again (may be the same value as EWOULDBLOCK]). 
EALREADY,      // Connection already in progress. 
EBADF,         // Bad file descriptor. 
ECANCELED,     // Operation canceled. 
ECONNABORTED,  // Connection aborted. 
ECONNREFUSED,  // Connection refused. 
ECONNRESET,    // Connection reset. 
EDOM,          // Mathematics argument out of domain of function. 
EEXIST,        // File exists. 
EFAULT,        // Bad address. 
EHOSTUNREACH,  // Host is unreachable. 
EINTR,         // Interrupted function. 
EINVAL,        // Invalid argument. 
EISCONN,       // Socket is connected. 
EISDIR,        // Is a directory. 
ENAMETOOLONG,  // Filename too long. 
ENETDOWN,      // Network is down. 
ENETRESET,     // Connection aborted by network. 
ENETUNREACH,   // Network unreachable. 
ENOENT,        // No such file or directory. 
ENOEXEC,       // Executable file format error. 

ENOSPC,        // No space left on device. 
ENOSYS,        // Function not supported. 
ENOTCONN,      // The socket is not connected. 
ENOTDIR,       // Not a directory. 
ENOTEMPTY,     // Directory not empty. 
ENOTSOCK,      // Not a socket. 
ENOTSUP,       // Not supported. 
EOVERFLOW,     // Value too large to be stored in data type. 
EPERM,         // Operation not permitted. 
EPIPE,         // Broken pipe. 
EPROTO,        // Protocol error. 
ERANGE,        // Result too large. 
ETIMEDOUT,     // Connection timed out. 
EWOULDBLOCK    // Operation would block (may be the same value as EAGAIN]). 

};
*/


//
// sys/stat.h
//

// already defined by MinGW
#ifdef _MSC_VER
typedef unsigned int mode_t;
#endif

// VC libc includes stat, but it's quite slow.
// we implement our own, but use the CRT struct definition.
// rename the VC function definition to avoid conflict.
/*
#define stat vc_stat
//
// Extra hack for VC++ 2005, since it defines inline stat/fstat
// functions inside stat.h (which get confused by the
// macro-renaming of "stat")
# if _MSC_VER >= 1400
#  define RC_INVOKED // stat.h only includes stat.inl if "!defined(RC_INVOKED) && !defined(__midl)"
#  include <sys/stat.h>
#  undef RC_INVOKED
# else
#  include <sys/stat.h>
# endif
#undef stat
*/
#  include <sys/stat.h>

extern int mkdir(const char*, mode_t);

// currently only sets st_mode (file or dir) and st_size.
//extern int stat(const char*, struct stat*);

#define S_IRWXO 0xffff
#define S_IRWXU 0xffff
#define S_IRWXG 0xffff
	// stat.h _S_* values are wrong! disassembly shows _S_IWRITE is 0x80,
	// instead of 0x100. define christmas-tree value to be safe.

#define S_ISDIR(m) (m & S_IFDIR)
#define S_ISREG(m) (m & S_IFREG)


//
// dirent.h
//

typedef void DIR;

struct dirent
{
	ino_t d_ino;
	char d_name[PATH_MAX+1];
};

extern DIR* opendir(const char* name);
extern struct dirent* readdir(DIR*);
extern int closedir(DIR*);

// return status for the file returned by the last successful
// readdir call from the given directory stream.
// currently sets st_size, st_mode, and st_mtime; the rest are zeroed.
// non-portable, but considerably faster than stat(). used by file_enum.
extern int readdir_stat_np(DIR*, struct stat*);


//
// <sys/mman.h>
//

// mmap prot flags
#define PROT_NONE   0x00
#define PROT_READ	0x01
#define PROT_WRITE	0x02
#define PROT_EXEC   0x04

// mmap flags
#define MAP_SHARED	0x01	// share changes across processes
#define MAP_PRIVATE	0x02	// private copy-on-write mapping
#define MAP_FIXED	0x04

#define MAP_FAILED 0

extern void* mmap(void* start, size_t len, int prot, int flags, int fd, off_t offset);
extern int munmap(void* start, size_t len);

extern int mprotect(void* addr, size_t len, int prot); 

//
// <fcntl.h>
//

// values from MS _open - do not change!
#define O_RDONLY       0x0000  // open for reading only
#define O_WRONLY       0x0001  // open for writing only
#define O_RDWR         0x0002  // open for reading and writing
#define O_APPEND       0x0008  // writes done at eof
#define O_CREAT        0x0100  // create and open file
#define O_TRUNC        0x0200  // open and truncate
#define O_EXCL         0x0400  // open only if file doesn't already exist

// .. Win32-only (not specified by POSIX)
#define O_TEXT_NP      0x4000  // file mode is text (translated)
#define O_BINARY_NP    0x8000  // file mode is binary (untranslated)

// .. wposix.cpp only (bit values not used by MS _open)
#define O_NO_AIO_NP    0x20000
	// wposix-specific: do not open a separate AIO-capable handle.
	// this is used for small files where AIO overhead isn't worth it,
	// thus speeding up loading and reducing resource usage.

// .. not supported by Win32 (bit values not used by MS _open)
#define O_NONBLOCK     0x1000000


// redefinition error here => io.h is getting included somewhere.
// we implement this function, so the io.h definition conflicts if
// compiling against the DLL CRT. either rename the io.h def
// (as with vc_stat), or don't include io.h.
extern int open(const char* fn, int mode, ...);


//
// <unistd.h>
//

// values from MS _access() implementation. do not change.
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 0
	// MS implementation doesn't support this distinction.
	// hence, the file is reported executable if it exists.

extern int read (int fd, void* buf, size_t nbytes);	// thunk
extern int write(int fd, void* buf, size_t nbytes);	// thunk
extern _CRTIMP off_t lseek(int fd, off_t ofs, int whence);


// redefinition error here => io.h is getting included somewhere.
// we implement this function, so the io.h definition conflicts if
// compiling against the DLL CRT. either rename the io.h def
// (as with vc_stat), or don't include io.h.
extern int close(int);
extern _CRTIMP int access(const char*, int);

extern int chdir(const char*);
#undef getcwd
extern char* getcwd(char*, size_t);

// user tests if available via #ifdef; can't use enum.
#define _SC_PAGESIZE      1
#define _SC_PAGE_SIZE     1
#define _SC_PHYS_PAGES    2
#define _SC_AVPHYS_PAGES  3

extern long sysconf(int name);


//
// <stdlib.h>
//

extern char* realpath(const char*, char*);



//
// <termios.h>
//

#define TCSANOW 0

struct termios
{
	long c_lflag;
};

#define ICANON 2	// do not change - correspond to ENABLE_LINE_INPUT / ENABLE_ECHO_INPUT
#define ECHO   4

extern int tcgetattr(int fd, struct termios* termios_p);
extern int tcsetattr(int fd, int optional_actions, const struct termios* termios_p);


//
// <poll.h>
//

struct pollfd
{
	int fd;
	short int events, revents;
};

#define POLLIN 1

extern int poll(struct pollfd[], int, int);


//
// <sys/utsname.h>
//

struct utsname
{
	char sysname[9];   // Name of this implementation of the operating system. 
	char nodename[16]; // Name of this node within an implementation-defined 
					   // communications network. 
	                   // Win9x requires this minimum buffer size.
	char release[9];   // Current release level of this implementation. 
	char version[16];  // Current version level of this release. 
	char machine[9];   // Name of the hardware type on which the system is running. 
};

extern int uname(struct utsname*);


//
// serial port IOCTL
//

// use with TIOCMBIS
#define TIOCM_RTS 1

// use with TIOCMGET or TIOCMIWAIT
#define TIOCM_CD  0x80	// MS_RLSD_ON
#define TIOCM_CTS 0x10	// MS_CTS_ON

enum
{
	TIOCMBIS,		// set control line
	TIOCMGET,		// get line state
	TIOCMIWAIT		// wait for status change
};

extern int ioctl(int fd, int op, int* data);

#ifndef _WINSOCKAPI_
#define FIONREAD 0
#endif


extern void _get_console(void);
extern void _hide_console(void);


#ifdef __cplusplus
}
#endif


#endif	// #ifndef __WPOSIX_H__
