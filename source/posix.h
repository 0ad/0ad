// POSIX emulation for Win32
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

#ifndef __POSIX_H__
#define __POSIX_H__

#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif


#define IMP(ret, name, param) extern __declspec(dllimport) ret __stdcall name param;



//
// <inttypes.h>
//

typedef unsigned short u16_t;


//
// <sys/types.h>
//

typedef int ssize_t;
typedef unsigned int size_t;


//
// <limits.h>
//

#define PATH_MAX 260


//
// <errno.h>
//

enum
{
EINPROGRESS = 1000,   // Operation in progress. 
ENOMEM        // Not enough space. 
/*
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
EDOM,          // Mathematics argument out of domain of IMPtion. 
EEXIST,        // File exists. 
EFAULT,        // Bad address. 
EHOSTUNREACH,  // Host is unreachable. 
EINTR,         // Interrupted IMPtion. 
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
ENOSYS,        // IMPtion not supported. 
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
*/
};


//
// <time.h>
//

typedef long time_t;

typedef enum
{
	CLOCK_REALTIME
}
clockid_t;

struct timespec
{
	time_t tv_sec;
	long   tv_nsec;
};

extern time_t time(time_t*);
extern int clock_gettime(clockid_t clock_id, struct timespec* tp);
extern int nanosleep(const struct timespec* rqtp, struct timespec* rmtp);


//
// sys/stat.h
//

typedef unsigned short ino_t;
typedef unsigned int mode_t;
typedef long off_t;
typedef unsigned int dev_t;

// struct stat defined in VC sys/stat.h

#define stat _stat

extern int mkdir(const char*, mode_t);


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

#define F_OK 0
#define R_OK 1
#define W_OK 2
#define X_OK 4

#define read _read
#define write _write

extern int close(int);
extern int access(const char*, int);
extern int chdir(const char*);

extern unsigned int sleep(unsigned int sec);
IMP(int, gethostname, (char* name, size_t namelen))



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
extern int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param);
extern int pthread_create(pthread_t* thread, const void* attr, void*(*IMP)(void*), void* arg);


//
// <sys/socket.h>
//

typedef unsigned long socklen_t;
typedef unsigned short sa_family_t;

// Win32 values - do not change
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define AF_INET 2

struct sockaddr;


//
// <netinet/in.h>
//

typedef unsigned long in_addr_t;
typedef unsigned short in_port_t;

struct in_addr
{
	in_addr_t s_addr;
};

struct sockaddr_in
{
	sa_family_t    sin_family;
	in_port_t      sin_port;
	struct in_addr sin_addr;
	unsigned char  sin_zero[8];
};

#define INADDR_ANY 0


//
// <netdb.h>
//

struct hostent
{
	char* h_name;       // Official name of the host. 
	char** h_aliases;   // A pointer to an array of pointers to 
	                    // alternative host names, terminated by a
	                    // null pointer. 
	short h_addrtype;   // Address type. 
	short h_length;     // The length, in bytes, of the address. 
	char** h_addr_list; // A pointer to an array of pointers to network 
	                    // addresses (in network byte order) for the host, 
	                    // terminated by a null pointer. 
};

IMP(struct hostent*, gethostbyname, (const char *name))


//
// <arpa/inet.h>
//

extern u16_t htons(u16_t hostshort);
#define ntohs htons

IMP(in_addr_t, inet_addr, (const char*))
IMP(char*, inet_ntoa, (in_addr))
IMP(int, accept, (int, struct sockaddr*, socklen_t*))
IMP(int, bind, (int, const struct sockaddr*, socklen_t))
IMP(int, connect, (int, const struct sockaddr*, socklen_t))
IMP(int, listen, (int, int))
IMP(ssize_t, recv, (int, void*, size_t, int))
IMP(ssize_t, send, (int, const void*, size_t, int))

IMP(int, socket, (int, int, int))


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


extern void _get_console();
extern void _hide_console();


#include "posix/aio.h"


#ifdef __cplusplus
}
#endif


#endif	// #ifndef __POSIX_H__
