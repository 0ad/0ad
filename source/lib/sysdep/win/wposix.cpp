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

// collection of hacks :P

#include "precompiled.h"

#include <process.h>

#include "lib.h"
#include "win_internal.h"
#include "hrt.h"


//////////////////////////////////////////////////////////////////////////////
//
// file
//
//////////////////////////////////////////////////////////////////////////////

/*
extern int aio_open(const char*, int, int);
extern int aio_close(int);
*/

int open(const char* fn, int mode, ...)
{
	bool is_com_port = strncmp(fn, "/dev/tty", 8) == 0;

	// /dev/tty? => COM?
	if(is_com_port)
	{
		static char port[] = "COM ";
		port[3] = (char)(fn[8]+1);
		fn = port;
	}

	int fd = _open(fn, mode);

	// open it for async I/O as well (_open defaults to deny_none sharing)
	if(fd > 2)
	{
		// .. unless it's a COM port. don't currently need aio access for those;
		// also, aio_open's CreateFile reports access denied when trying to open.
		if(!is_com_port)
			aio_open(fn, mode, fd);
	}

	return fd;
}


int close(int fd)
{
	aio_close(fd);
	return _close(fd);
}


int ioctl(int fd, int op, int* data)
{
	HANDLE h = (HANDLE)((char*)0 + _get_osfhandle(fd));

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


// currently only sets st_mode (file or dir) and st_size.
int stat(const char* fn, struct stat* s)
{
	memset(s, 0, sizeof(struct stat));

	WIN32_FILE_ATTRIBUTE_DATA fad;
	if(!GetFileAttributesEx(fn, GetFileExInfoStandard, &fad))
		return -1;

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

	size_t size = round_up(sizeof(_DIR), 32);
		// be nice to allocator.
	_DIR* d = (_DIR*)calloc(size, 1);

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


static HANDLE std_h[2] = { (HANDLE)(((char*)0) + 3), (HANDLE)(((char*)0) + 7) };


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
{ __asm jmp		dword ptr [GetCurrentThreadId] }


int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param)
{
	if(policy)
	{
		DWORD pc = GetPriorityClass(GetCurrentProcess());
		*policy = (pc >= HIGH_PRIORITY_CLASS)? SCHED_FIFO : SCHED_RR;
	}
	if(param)
	{
		HANDLE hThread = (HANDLE)((char*)0 + thread);
		param->sched_priority = GetThreadPriority(hThread);
	}

	return 0;
}

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param)
{
	if(policy == SCHED_FIFO)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	HANDLE hThread = (HANDLE)((char*)0 + thread);

	SetThreadPriority(hThread, param->sched_priority);
	return 0;
}


int pthread_create(pthread_t* /* thread */, const void* /* attr */, void*(*func)(void*), void* arg)
{
	/* can't use asm 'cause _beginthread might be a func ptr (with libc) */
	return (int)_beginthread((void(*)(void*))func, 0, arg);
}


pthread_mutex_t pthread_mutex_initializer()
{
	HANDLE h = CreateMutex(0, 0, 0);
	if(h == INVALID_HANDLE_VALUE)
		return 0;
	atexit2(CloseHandle, (uintptr_t)h, CC_STDCALL_1);
	return h;
}

int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t*)
{
	if(!m)
		return -1;
	*m = pthread_mutex_initializer();
	return *m? 0 : -1;
}

int pthread_mutex_lock(pthread_mutex_t* m)
{
	return WaitForSingleObject(*m, INFINITE) == WAIT_OBJECT_0? 0 : -1;
}

int pthread_mutex_trylock(pthread_mutex_t* m)
{
	return WaitForSingleObject(*m, 0) == WAIT_OBJECT_0? 0 : -1;
}

int pthread_mutex_unlock(pthread_mutex_t* m)
{
	return ReleaseMutex(*m)? 0 : -1;
}

int pthread_mutex_timedlock(pthread_mutex_t* m, const struct timespec* abs_timeout)
{
	DWORD ms_timeout = 0;
	if(abs_timeout)
	{
		struct timespec cur_ts;
		clock_gettime(CLOCK_REALTIME, &cur_ts);
		ms_timeout = DWORD((cur_ts.tv_sec  - abs_timeout->tv_sec ) * 1000 +
		                   (cur_ts.tv_nsec - abs_timeout->tv_nsec) / 1000000);
	}

	return WaitForSingleObject(*m, ms_timeout) == WAIT_OBJECT_0? 0 : -1;
}

int pthread_mutex_destroy(pthread_mutex_t* m)
{
	CloseHandle(*m);
	return 0;
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

	DWORD len_hi = (DWORD)((u64)len >> 32), len_lo = (DWORD)len & 0xffffffff;

	HANDLE hFile = (HANDLE)((char*)0 + _get_osfhandle(fd));
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


//////////////////////////////////////////////////////////////////////////////
//
// time
//
//////////////////////////////////////////////////////////////////////////////


static const long _1e6 = 1000000;
static const i64 _1e9 = 1000000000;


// return nanoseconds since posix epoch
// currently only 10 or 15 ms resolution! use HRT for finer timing
static i64 st_time_ns()
{
	union
	{
		FILETIME ft;
		i64 i;
	}
	t;
	GetSystemTimeAsFileTime(&t.ft);
	// Windows system time is hectonanoseconds since Jan. 1, 1601
	return (t.i - 0x019DB1DED53E8000) * 100;
}


static inline long st_res_ns()
{
	DWORD adjust, interval;
	BOOL adj_disabled;
	GetSystemTimeAdjustment(&adjust, &interval, &adj_disabled);
	return interval * 100;	// hns -> ns
}


static void sleep_ns(i64 ns)
{
	DWORD ms = DWORD(ns / _1e6);
	if(ms != 0)
		Sleep(ms);
	else
	{
		i64 t0 = hrt_ticks(), t1;
		do
			t1 = hrt_ticks();
		while(hrt_delta_s(t0, t1) * _1e9 < ns);
	}
}


int clock_gettime(clockid_t clock, struct timespec* t)
{
#ifndef NDEBUG
	if(clock != CLOCK_REALTIME || !t)
	{
		debug_warn("clock_gettime: invalid clock or t param");
		return -1;
	}
#endif

	const i64 ns = st_time_ns();
	t->tv_sec  = (time_t)(ns / _1e9);
	t->tv_nsec = (long)  (ns % _1e9);
	return 0;
}


int clock_getres(clockid_t clock, struct timespec* res)
{
#ifndef NDEBUG
	if(clock != CLOCK_REALTIME || !res)
	{
		debug_warn("clock_getres: invalid clock or res param");
		return -1;
	}
#endif

	res->tv_sec  = 0;
	res->tv_nsec = st_res_ns();;
	return 0;
}


int nanosleep(const struct timespec* rqtp, struct timespec* /* rmtp */)
{
	i64 ns = rqtp->tv_sec;	// make sure we don't overflow
	ns *= _1e9;
	ns += rqtp->tv_nsec;
	sleep_ns(ns);
	return 0;
}



int gettimeofday(struct timeval* tv, void* tzp)
{
	UNUSED(tzp);

#ifndef NDEBUG
	if(!tv)
	{
		debug_warn("gettimeofday: invalid t param");
		return -1;
	}
#endif

	const i64 us = st_time_ns() / 1000;
	tv->tv_sec  = (time_t)     (us / _1e6);
	tv->tv_usec = (suseconds_t)(us % _1e6);
	return 0;
}


uint sleep(uint sec)
{
	Sleep(sec * 1000);
	return sec;
}


int usleep(useconds_t us)
{
	// can't overflow, because us < 1e6
	sleep_ns(us * 1000);
	return 0;
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
	DWORD buf_size = sizeof(un->nodename);
	GetComputerName(un->nodename, &buf_size);

	// hardware type
	static SYSTEM_INFO si;
	GetSystemInfo(&si);
	if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		strcpy(un->machine, "AMD64");
	else
		strcpy(un->machine, "IA-32");

	return 0;
}


long sysconf(int name)
{
	// used by _SC_*_PAGES
	static DWORD page_size;

	switch(name)
	{
	case _SC_PAGESIZE:
		if(page_size)
			return page_size;
		SYSTEM_INFO si;
		GetSystemInfo(&si);		// can't fail => page_size always > 0.
		return page_size = si.dwPageSize;

	case _SC_PHYS_PAGES:
	case _SC_AVPHYS_PAGES:
		if(!page_size)
			sysconf(_SC_PAGESIZE);	// sets page_size

		{
		MEMORYSTATUSEX ms = { sizeof(ms) };
		GlobalMemoryStatusEx(&ms);
		if(name == _SC_PHYS_PAGES)
			return (long)(round_up((uintptr_t)ms.ullTotalPhys, 2*MB) / page_size);
				// Richter, "Programming Applications for Windows":
				// reported value doesn't include non-paged pool reserved
				// during boot; it's not considered available to kernel.
				// it's 528 KB on my 512 MB machine (WinXP and Win2k).
 		else
			return (long)(ms.ullAvailPhys / page_size);
		}

	default:
		return -1;
	}
}


u16_t htons(u16_t s)
{
	return (s >> 8) | ((s & 0xff) << 8);
}




/******************************************************************/
/* socket dynamic functions */
fp_getnameinfo_t getnameinfo;
fp_getaddrinfo_t getaddrinfo;
fp_freeaddrinfo_t freeaddrinfo;

/* IPv6 globals
These are included in the linux C libraries, and in newer platform SDK's, so
should only be needed in VC++6 or earlier.
*/
//#if _MSC_VER <= 1200 /* VC++6 or earlier */
const struct in6_addr in6addr_any=IN6ADDR_ANY_INIT;        /* :: */
const struct in6_addr in6addr_loopback=IN6ADDR_LOOPBACK_INIT;   /* ::1 */
//#endif
/*
void entry(void)
{
	// note: winsock header is also removed by this define
#ifndef NO_WINSOCK
	char d[1024];
	WSAStartup(0x0002, d);	// want 2.0
#endif

	HMODULE h=LoadLibrary("ws2_32.dll");
	if (h)
	{
		getaddrinfo=(fp_getaddrinfo_t)GetProcAddress(h, "getaddrinfo");
		getnameinfo=(fp_getnameinfo_t)GetProcAddress(h, "getnameinfo");
		freeaddrinfo=(fp_freeaddrinfo_t)GetProcAddress(h, "freeaddrinfo");
	}
	else
	{
		getaddrinfo=NULL;
		getnameinfo=NULL;
		freeaddrinfo=NULL;
	}

	mainCRTStartup();
}
*/