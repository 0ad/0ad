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


#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


#define IMP(ret, name, param) extern "C" __declspec(dllimport) ret __stdcall name param;

// for functions actually implemented in the CRT
#ifdef _DLL
#define _CRTIMP __declspec(dllimport)
#else
#define _CRTIMP
#endif



//
// <inttypes.h>
//

typedef unsigned short u16_t;


//
// <sys/types.h>
//

typedef unsigned long useconds_t;
typedef long suseconds_t;
typedef long ssize_t;


//
// <limits.h>
//

#define PATH_MAX 260


//
// <errno.h>
//

#define EINPROGRESS 100000

#include <errno.h>

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

typedef unsigned int mode_t;

// VC libc includes stat, but it's quite slow.
// we implement our own, but use the CRT struct definition.
#include <sys/stat.h>

extern int mkdir(const char*, mode_t);

// currently only sets st_mode (file or dir) and st_size.
extern int stat(const char*, struct stat*);


//
// dirent.h
//

typedef void DIR;

struct dirent
{
	ino_t d_ino;
	char* d_name;
};

extern DIR* opendir(const char* name);
extern struct dirent* readdir(DIR*);
extern int closedir(DIR*);


//
// <sys/mman.h>
//

#define PROT_READ	0x01	// page can be read
#define PROT_WRITE	0x02	// page can be written

#define MAP_SHARED	0x01	// share changes across processes
#define MAP_PRIVATE	0x02	// private copy on write mapping
#define MAP_FIXED	0x04

#define MAP_FAILED 0

extern void* mmap(void* start, unsigned int len, int prot, int flags, int fd, long offset);
extern int munmap(void* start, unsigned int len);


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
#define O_BINARY       0x8000  // file mode is binary (untranslated)

#define O_NONBLOCK     0x1000000

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

#define read _read
#define write _write

extern int close(int);
extern int access(const char*, int);

extern int chdir(const char*);
#define getcwd _getcwd


extern unsigned int sleep(unsigned int sec);
extern int usleep(useconds_t us);

// user tests if available via #ifdef; can't use enum.
#define _SC_PAGESIZE      1
#define _SC_PAGE_SIZE     1
#define _SC_PHYS_PAGES    2
#define _SC_AVPHYS_PAGES  3

extern long sysconf(int name);

#ifndef _WINSOCKAPI_

IMP(int, gethostname, (char* name, size_t namelen))

#endif

//
// <stdlib.h>
//

extern char* realpath(const char*, char*);


//
// <signal.h>
//

union sigval
{
    int sival_int;				// Integer signal value.
    void* sival_ptr;			// Pointer signal value.
};

struct sigevent
{
    int sigev_notify;			// notification mode
    int sigev_signo;			// signal number
    union sigval sigev_value;	// signal value
	void(*sigev_notify_function)(union sigval);
};


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
// <sched.h>
//

struct sched_param
{
	int sched_priority;
};

enum
{
	SCHED_RR,
	SCHED_FIFO,
	SCHED_OTHER
};

#define sched_get_priority_max(policy) 15		// TIME_CRITICAL
#define sched_get_priority_min(policy) -15		// IDLE


//
// <pthread.h>
//

typedef unsigned int pthread_t;

extern pthread_t pthread_self();
extern int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param);
extern int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param);
extern int pthread_create(pthread_t* thread, const void* attr, void*(*func)(void*), void* arg);

typedef void* pthread_mutex_t;
typedef void pthread_mutexattr_t;

extern pthread_mutex_t pthread_mutex_initializer();
#define PTHREAD_MUTEX_INITIALIZER pthread_mutex_initializer()

extern int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
extern int pthread_mutex_destroy(pthread_mutex_t*);

extern int pthread_mutex_lock(pthread_mutex_t*);
extern int pthread_mutex_trylock(pthread_mutex_t*);
extern int pthread_mutex_unlock(pthread_mutex_t*);
extern int pthread_mutex_timedlock(pthread_mutex_t*, const struct timespec*);


//
// <time.h>
//

typedef enum
{
	CLOCK_REALTIME
}
clockid_t;

// BSD gettimeofday
struct timeval
{
	time_t tv_sec;
	suseconds_t tv_usec;
};

// POSIX realtime clock_*
struct timespec
{
	time_t tv_sec;
	long   tv_nsec;
};

extern int gettimeofday(struct timeval* tv, void* tzp);

extern int nanosleep(const struct timespec* rqtp, struct timespec* rmtp);
extern int clock_gettime(clockid_t clock, struct timespec* ts);
extern int clock_getres(clockid_t clock, struct timespec* res);




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


extern void _get_console();
extern void _hide_console();


// split out of this module
#include "waio.h"
#include "wsock.h"


#ifdef __cplusplus
}
#endif


#endif	// #ifndef __WPOSIX_H__
