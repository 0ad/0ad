/**
 * =========================================================================
 * File        : wposix.h
 * Project     : 0 A.D.
 * Description : emulate a subset of POSIX on Win32.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

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

// Win32 MAX_PATH is 260; our number may be a bit more efficient.
#define PATH_MAX 256

#if OS_WIN
# ifndef SIZE_MAX // VC2005 already defines this in limits.h
#  define SIZE_MAX 0xffffffff
# endif
#else
# define SIZE_MAX 0xffffffffffffffff
#endif


//
// sys/stat.h
//

// already defined by MinGW
#if MSC_VERSION
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
# if MSC_VERSION >= 1400
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
#define MAP_SHARED	0x01	// writes change the underlying file
#define MAP_PRIVATE	0x02	// writes do not affect the file (copy-on-write)
#define MAP_FIXED	0x04
// .. non-portable
#define MAP_ANONYMOUS 0x10 
#define MAP_NORESERVE 0x20

// note: we need a means of only "reserving" virtual address ranges
// for the fixed-address expandable array mechanism. the non-portable
// MAP_NORESERVE flag says that no space in the page file need be reserved.
// the caller can still try to access the entire mapping, but might get
// SIGBUS if there isn't enough room to commit a page. Linux currently
// doesn't commit mmap-ed regions anyway, but we specify this flag to
// make sure of that in the future.

#define MAP_FAILED ((void*)-1L)

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
// <dlfcn.h>
//

// these have no meaning for the Windows GetProcAddress implementation,
// so they are ignored but provided for completeness.
#define RTLD_LAZY   0x01
#define RTLD_NOW    0x02
#define RTLD_GLOBAL 0x04	// semantics are unsupported, so complain if set.
#define RTLD_LOCAL  0x08

extern int dlclose(void* handle);
extern char* dlerror(void);
extern void* dlopen(const char* so_name, int flags);
extern void* dlsym(void* handle, const char* sym_name);


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
// <errno.h>
//

#include <errno.h>

// this is an exhaustive list of SUSv3 error codes;
// see http://www.opengroup.org/onlinepubs/009695399/basedefs/errno.h.html .
// .. the following are already correctly defined by VC errno.h:
#if 0
#define EPERM            1      // Operation not permitted
#define ENOENT           2      // No such file or directory
#define ESRCH            3      // No such process
#define EINTR            4      // Interrupted system call
#define EIO              5      // I/O error
#define ENXIO            6      // No such device or address
#define E2BIG            7      // Argument list too long
#define ENOEXEC          8      // Exec format error
#define EBADF            9      // Bad file number
#define ECHILD          10      // No child processes
#define EAGAIN          11      // Try again
#define ENOMEM          12      // Out of memory
#define EACCES          13      // Permission denied
#define EFAULT          14      // Bad address
#define EBUSY           16      // Device or resource busy
#define EEXIST          17      // File exists
#define ENODEV          19      // No such device
#define ENOTDIR         20      // Not a directory
#define EISDIR          21      // Is a directory
#define EINVAL          22      // Invalid argument
#define ENFILE          23      // File table overflow
#define EMFILE          24      // Too many open files
#define ENOTTY          25      // Not a typewriter
#define EFBIG           27      // File too large
#define ENOSPC          28      // No space left on device
#define ESPIPE          29      // Illegal seek
#define EMLINK          31      // Too many links
#define EPIPE           32      // Broken pipe
#define EDOM            33      // Math argument out of domain of func
#define ERANGE          34      // Math result not representable
#endif
// .. the following are unfortunately defined differently by VC errno.h;
//    we have to stick with those interpretations because they are set
//    by already-compiled CRT code.
#if 0
#define EDEADLK         35      // Resource deadlock would occur
#define ENAMETOOLONG    36      // File name too long
#define ENOLCK          37      // No record locks available
#define ENOSYS          38      // Function not implemented
#define ENOTEMPTY       39      // Directory not empty
#define EILSEQ          84      // Illegal byte sequence
#endif
// .. the following aren't yet defined; we take on the Linux values for
//    simplicity (why assign different values?)
#define ELOOP           40      // Too many symbolic links encountered
#define ENOMSG          42      // No message of desired type
#define EIDRM           43      // Identifier removed
#define EWOULDBLOCK     EAGAIN  // Operation would block
#define ENOLINK         67      // Reserved
#define EPROTO          71      // Protocol error
#define EMULTIHOP       72      // Reserved
#define EBADMSG         74      // Not a data message
#define EOVERFLOW       75      // Value too large for defined data type
#define ENOTSOCK        88      // Socket operation on non-socket
#define EDESTADDRREQ    89      // Destination address required
#define EMSGSIZE        90      // Message too long
#define EPROTOTYPE      91      // Protocol wrong type for socket
#define ENOPROTOOPT     92      // Protocol not available
#define EPROTONOSUPPORT 93      // Protocol not supported
#define EOPNOTSUPP      95      // Operation not supported on transport endpoint
#define EAFNOSUPPORT    97      // Address family not supported by protocol
#define EADDRINUSE      98      // Address already in use
#define EADDRNOTAVAIL   99      // Cannot assign requested address
#define ENETDOWN        100     // Network is down
#define ENETUNREACH     101     // Network is unreachable
#define ENETRESET       102     // Network dropped connection because of reset
#define ECONNABORTED    103     // Software caused connection abort
#define ECONNRESET      104     // Connection reset by peer
#define ENOBUFS         105     // No buffer space available
#define EISCONN         106     // Transport endpoint is already connected
#define ENOTCONN        107     // Transport endpoint is not connected
#define ETIMEDOUT       110     // Connection timed out
#define ECONNREFUSED    111     // Connection refused
#define EHOSTUNREACH    113     // No route to host
#define EALREADY        114     // Operation already in progress
#define EINPROGRESS     115     // Operation now in progress
#define ESTALE          116     // Reserved
#define EDQUOT          122     // Reserved
#define ECANCELED       125     // Operation Canceled


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
