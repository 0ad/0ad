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

// collection of hacks :P

#include <cassert>
#include <cstdlib>
#include <cmath>
#include <cstdio>

#include <process.h>

#include "posix.h"
#include "win.h"
#include "time.h"
#include "misc.h"


// VC6 windows.h may not have defined these
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif

extern "C"
{
// both hooked to support aio
extern int _open(const char* fn, int mode, ...);
extern int _close(int);

extern int _get_osfhandle(int);
extern void mainCRTStartup();
}


//////////////////////////////////////////////////////////////////////////////
//
// file
//
//////////////////////////////////////////////////////////////////////////////


extern int aio_open(const char*, int, int);
extern int aio_close(int);


int open(const char* fn, int mode, ...)
{
	// /dev/tty? => COM?
	if(!strncmp(fn, "/dev/tty", 8))
	{
		static char port[] = "COM ";
		port[3] = (char)(fn[8]+1);
		fn = port;
	}

	int fd = _open(fn, mode);

	// open it for async I/O as well (_open defaults to deny_none sharing)
	if(fd > 2)
		aio_open(fn, mode, fd);

	return fd;
}


int close(int fd)
{
	aio_close(fd);
	return _close(fd);
}


int ioctl(int fd, int op, int* data)
{
	HANDLE h = (HANDLE)_get_osfhandle(fd);

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


//////////////////////////////////////////////////////////////////////////////
//
// dir
//
//////////////////////////////////////////////////////////////////////////////


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


struct _DIR
{
	WIN32_FIND_DATA fd;
	HANDLE handle;
	struct dirent ent;		// must not be overwritten by calls for different dirs
	bool not_first;
};


DIR* opendir(const char* name)
{
	DWORD fa = GetFileAttributes(name);
	if(fa == INVALID_FILE_ATTRIBUTES || !(fa & FILE_ATTRIBUTE_DIRECTORY))
		return 0;

	_DIR* d = (_DIR*)calloc(sizeof(_DIR), 1);

	char path[MAX_PATH+1];
	strncpy(path, name, MAX_PATH-2);
	strcat(path, "\\*");
	d->handle = FindFirstFile(path, &d->fd);

	return d;
}


struct dirent* readdir(DIR* dir)
{
	_DIR* d = (_DIR*)dir;

	if(d->not_first)
		if(!FindNextFile(d->handle, &d->fd))
			return 0;
	d->not_first = true;

	d->ent.d_ino = 0;
	d->ent.d_name = &d->fd.cFileName[0];
	return &d->ent;
}


int closedir(DIR* dir)
{
	_DIR* d = (_DIR*)dir;

	FindClose(d->handle);

	free(dir);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// terminal
//
//////////////////////////////////////////////////////////////////////////////


static HANDLE std_h[2] = { (HANDLE)3, (HANDLE)7 };


__declspec(naked) void _get_console()
{ __asm	jmp		dword ptr [AllocConsole] }

__declspec(naked) void _hide_console()
{ __asm jmp		dword ptr [FreeConsole] }


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
// thread
//
//////////////////////////////////////////////////////////////////////////////


__declspec(naked) pthread_t pthread_self(void)
{ __asm jmp		dword ptr [GetCurrentThread] }


int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param)
{
	if(policy == SCHED_FIFO)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	HANDLE hThread = (HANDLE)thread;

	SetThreadPriority(hThread, param->sched_priority);
	return 0;
}


int pthread_create(pthread_t* /* thread */, const void* /* attr */, void*(* func)(void*), void* arg)
{
	/* can't use asm 'cause _beginthread might be a func ptr (with libc) */
	return (int)_beginthread((void(*)(void*))func, 0, arg);
}


//////////////////////////////////////////////////////////////////////////////
//
// time
//
//////////////////////////////////////////////////////////////////////////////


int clock_gettime(clockid_t clock_id, struct timespec* tp)
{
	static double start_t = -1.0;
	static time_t start_s;

	if(start_t < 0.0)
	{
		start_s = time(0);
		start_t = get_time();
	}

	if(clock_id != CLOCK_REALTIME)
	{
//		errno = EINVAL;
		return -1;
	}

	double t = get_time();
	double dt = t-start_t;
	start_t = t;

	int ds = (int)floor(dt);
	int dn = (int)floor((dt-ds) * 1.0e9);

	tp->tv_sec  = start_s + ds;
	tp->tv_nsec = 0       + dn;

	return 0;
}


int nanosleep(const struct timespec* rqtp, struct timespec* /* rmtp */)
{
	int ms = (int)rqtp->tv_sec * 1000 + rqtp->tv_nsec / 1000000;
	if(ms > 0)
		Sleep(ms);
	else
	{
		struct timespec t1, t2;
		clock_gettime(CLOCK_REALTIME, &t1);
		int d_ns;
		do
		{
			clock_gettime(CLOCK_REALTIME, &t2);
			d_ns = (int)(t2.tv_sec-t1.tv_sec)*1000000000 + (t2.tv_nsec-t1.tv_nsec);
		}
		while(d_ns < rqtp->tv_nsec);
	}

	return 0;
}


uint sleep(uint sec)
{
	Sleep(sec * 1000);

	return sec;
}


//////////////////////////////////////////////////////////////////////////////
//
// memory mapping
//
//////////////////////////////////////////////////////////////////////////////


void* mmap(void* start, unsigned int len, int prot, int flags, int fd, long offset)
{
	if(!(flags & MAP_FIXED))
		start = 0;

	/* interpret protection/mapping flags. */
	SECURITY_ATTRIBUTES sec = { sizeof(SECURITY_ATTRIBUTES), 0, 0 };
	DWORD flProtect = PAGE_READONLY;	/* mapping object permission */
	DWORD dwAccess = FILE_MAP_READ;		/* file map access permission */
	if(prot & PROT_WRITE)
	{
		flProtect = PAGE_READWRITE;

		/* changes are shared & written to file */
		if(flags & MAP_SHARED)
		{
			sec.bInheritHandle = 1;
			dwAccess = FILE_MAP_ALL_ACCESS;
		}
		/* private copy on write mapping */
		else if(flags & MAP_PRIVATE)
		{
			flProtect = PAGE_WRITECOPY;
			dwAccess = FILE_MAP_COPY;
		}
	}

	DWORD len_hi = (u32)((u64)len >> 32), len_lo = (u32)len & 0xffffffff;

	HANDLE hFile = (HANDLE)_get_osfhandle(fd);
	HANDLE hMap = CreateFileMapping(hFile, &sec, flProtect, len_hi, len_lo, 0);

	void* ptr = MapViewOfFileEx(hMap, dwAccess, len_hi, offset, len_lo, start);

	/* file mapping object will be freed when ptr is unmapped */
	CloseHandle(hMap);

	if(!ptr || (flags & MAP_FIXED && ptr != start))
		return MAP_FAILED;

	return ptr;
}


int munmap(void* start, unsigned int /* len */)
{
	return UnmapViewOfFile(start) - 1;	/* 0: success; -1: fail */
}


int uname(struct utsname* un)
{
	if(!un)
		return -1;

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
		strcpy(un->release, release);
	}

	// version
	sprintf(un->version, "%lu.%02lu.%lu", vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber & 0xffff);

	// node name
	u32 buf_size = sizeof(un->nodename);
	GetComputerName(un->nodename, &buf_size);

	// hardware type
	static SYSTEM_INFO si;
	GetSystemInfo(&si);

	if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		strcpy(un->machine, "AMD64");
	else
		strcpy(un->machine, "x86");

	return 0;
}




#ifndef NO_WINSOCK
#ifdef _MSC_VER
#pragma comment(lib, "wsock32.lib")
#endif
extern "C" {
IMP(int, WSAStartup, (WORD, char*))
}
#endif


extern void entry(void);

void entry(void)
{
// alloc __pio, __iob?
// replace CRT init?

// header also removed to prevent calling Winsock functions

#ifndef NO_WINSOCK
	char d[1024];
	WSAStartup(0x0101, d);
#endif

	mainCRTStartup();
}
