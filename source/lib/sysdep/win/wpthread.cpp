// partial pthread implementation for Win32
//
// Copyright (c) 2003-2005 Jan Wassenberg
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

#include <new>

#include <process.h>

#include "lib.h"
#include "posix.h"
#include "win_internal.h"


static HANDLE pthread_t_to_HANDLE(pthread_t p)
{
	return (HANDLE)((char*)0 + p);
}

static pthread_t HANDLE_to_pthread_t(HANDLE h)
{
	return (pthread_t)(uintptr_t)h;
}


pthread_t pthread_self(void)
{
	return HANDLE_to_pthread_t(GetCurrentThread());
}


int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param)
{
	if(policy)
	{
		DWORD pc = GetPriorityClass(GetCurrentProcess());
		*policy = (pc >= HIGH_PRIORITY_CLASS)? SCHED_FIFO : SCHED_RR;
	}
	if(param)
	{
		const HANDLE hThread = pthread_t_to_HANDLE(thread);
		param->sched_priority = GetThreadPriority(hThread);
	}

	return 0;
}

int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param)
{
	const int pri = param->sched_priority;

	// additional boost for policy == SCHED_FIFO
	DWORD pri_class = NORMAL_PRIORITY_CLASS;
	if(policy == SCHED_FIFO)
	{
		pri_class = HIGH_PRIORITY_CLASS;
		if(pri == 2)
			pri_class = REALTIME_PRIORITY_CLASS;
	}
	SetPriorityClass(GetCurrentProcess(), pri_class);

	// choose fixed Windows values from pri
	const HANDLE hThread = pthread_t_to_HANDLE(thread);
	SetThreadPriority(hThread, pri);
	return 0;
}


struct ThreadParam
{
	void*(*func)(void*);
	void* user_arg;
	ThreadParam(void*(*_func)(void*), void* _user_arg)
		: func(_func), user_arg(_user_arg) {}
};


// trampoline to switch calling convention.
// param points to a heap-allocated ThreadParam (see pthread_create).
static unsigned __stdcall thread_start(void* param)
{
	ThreadParam* f = (ThreadParam*)param;
	void*(*func)(void*) = f->func;
	void* user_arg      = f->user_arg;
	delete f;

	// workaround for stupid "void* -> unsigned cast" warning
	union { void* p; unsigned u; } v;
	v.p = func(user_arg);
	return v.u;
}


int pthread_create(pthread_t* thread, const void* attr, void*(*func)(void*), void* user_arg)
{
	UNUSED(attr);

	// notes:
	// - don't stack-allocate param: thread_start might not be called
	//   in the new thread before we exit this stack frame.
	// - _beginthreadex has more overhead and no value added vs. CreateThread,
	//   but the following problem is documented: when using the
	//   statically-linked CRT, ExitThread leaks memory when CreateThread is
	//   used instead of _beginthread(..ex also?).
	ThreadParam* param = new ThreadParam(func, user_arg);
	*thread = (pthread_t)_beginthreadex(0, 0, thread_start, (void*)param, 0, 0);
	return 0;
}


int pthread_cancel(pthread_t thread)
{
	HANDLE hThread = pthread_t_to_HANDLE(thread);
	TerminateThread(hThread, 0);
	debug_out("WARNING: pthread_cancel is unsafe\n");
	return 0;
}


int pthread_join(pthread_t thread, void** value_ptr)
{
	HANDLE hThread = pthread_t_to_HANDLE(thread);

	// note: pthread_join doesn't call for a timeout. if this wait
	// locks up the process, at least it'll be easy to see why.
	DWORD ret = WaitForSingleObject(hThread, INFINITE);
	if(ret != WAIT_OBJECT_0)
	{
		debug_warn("pthread_join: WaitForSingleObject failed");
		return -1;
	}

	// pass back the code that was passed to pthread_exit.
	// SUS says <*value_ptr> need only be set on success!
	if(value_ptr)
		GetExitCodeThread(hThread, (LPDWORD)value_ptr);

	CloseHandle(hThread);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////


// rationale: CRITICAL_SECTIONS have less overhead than Win32 Mutex.
// disadvantage is that pthread_mutex_timedlock isn't supported, but
// the user can switch to semaphores if this facility is important.

// DeleteCriticalSection currently doesn't complain if we double-free
// (e.g. user calls destroy() and static initializer atexit runs),
// and dox are ambiguous.

// note: pthread_mutex_t must not be an opaque struct, because the
// initializer returns pthread_mutex_t directly and CRITICAL_SECTIONS
// shouldn't be copied.
//
// note: must not use new/malloc to allocate the critical section
// because mmgr.cpp uses a mutex and must not be called to allocate
// anything before it is initialized.

pthread_mutex_t pthread_mutex_initializer()
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)win_alloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(cs);
	return (pthread_mutex_t)cs;
}

int pthread_mutex_destroy(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)(*m);
	DeleteCriticalSection(cs);
	win_free(cs);
	return 0;
}

int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t*)
{
	*m = pthread_mutex_initializer();
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)(*m);
	EnterCriticalSection(cs);
	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)(*m);
	BOOL got_it = TryEnterCriticalSection(cs);
	return got_it? 0 : -1;
}

int pthread_mutex_unlock(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)(*m);
	LeaveCriticalSection(cs);
	return 0;
}

// not implemented - pthread_mutex is based on CRITICAL_SECTION,
// which doesn't support timeouts. use sem_timedwait instead.
int pthread_mutex_timedlock(pthread_mutex_t* m, const struct timespec* abs_timeout)
{
	UNUSED(m);
	UNUSED(abs_timeout);
	return -ENOSYS;
}


//////////////////////////////////////////////////////////////////////////////


HANDLE sem_t_to_HANDLE(sem_t* sem)
{
	return (HANDLE)*sem;
}

int sem_init(sem_t* sem, int pshared, unsigned value)
{
	UNUSED(pshared);
	*sem = (uintptr_t)CreateSemaphore(0, (LONG)value, 0x7fffffff, 0);
	return 0;
}

int sem_post(sem_t* sem)
{
	HANDLE h = sem_t_to_HANDLE(sem);
	ReleaseSemaphore(h, 1, 0);
	return 0;
}

int sem_wait(sem_t* sem)
{
	HANDLE h = sem_t_to_HANDLE(sem);
	WaitForSingleObject(h, INFINITE);
	return 0;
}

int sem_destroy(sem_t* sem)
{
	HANDLE h = sem_t_to_HANDLE(sem);
	CloseHandle(h);
	return 0;
}


// helper function for sem_timedwait - multiple return is convenient.
// converts an absolute timeout deadline into a relative length for use with
// WaitForSingleObject with the following peculiarity: if the semaphore
// could be locked immediately, abs_timeout must be ignored (see SUS).
// to that end, we return a timeout of 0 and pass back <valid> = false if
// abs_timeout is invalid.
static DWORD calc_timeout_length_ms(const struct timespec* abs_timeout,
	bool& timeout_is_valid)
{
	timeout_is_valid = false;

	if(!abs_timeout)
		return 0;

	// SUS requires we fail if not normalized
	if(abs_timeout->tv_nsec >= 1000000000)
		return 0;

	struct timespec cur_time;
	if(clock_gettime(CLOCK_REALTIME, &cur_time) != 0)
		return 0;

	timeout_is_valid = true;

	// convert absolute deadline to relative length, rounding up to [ms].
	// note: use i64 to avoid overflow in multiply.
	const i64  ds = abs_timeout->tv_sec  - cur_time.tv_sec;
	const long dn = abs_timeout->tv_nsec - cur_time.tv_nsec;
	i64 length_ms = ds*1000 + (dn+500000)/1000000;
	// .. deadline already reached; we'll still attempt to lock once
	if(length_ms < 0)
		return 0;
	// .. length > 49 days => result won't fit in 32 bits. most likely bogus.
	//    note: we're careful to avoid returning exactly -1 since
	//          that's the Win32 INFINITE value.
	if(length_ms >= 0xffffffff)
	{
		debug_warn("calc_timeout_length_ms: 32-bit overflow");
		length_ms = 0xfffffffe;
	}
	return (DWORD)(length_ms & 0xffffffff);
}

int sem_timedwait(sem_t* sem, const struct timespec* abs_timeout)
{
	bool timeout_is_valid;
	DWORD timeout_ms = calc_timeout_length_ms(abs_timeout, timeout_is_valid);

	HANDLE h = sem_t_to_HANDLE(sem);
	DWORD ret = WaitForSingleObject(h, timeout_ms);
	// successfully decremented semaphore; bail.
	if(ret == WAIT_OBJECT_0)
		return 0;

	// we're going to return -1. decide what happened:
	// .. abs_timeout was invalid (must not check this before trying to lock)
	if(!timeout_is_valid)
		errno = EINVAL;
	// .. timeout reached (not a failure)
	else if(ret == WAIT_TIMEOUT)
		errno = ETIMEDOUT;

	return -1;
}
