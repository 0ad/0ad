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
#include "../cpu.h"	// CAS


static HANDLE HANDLE_from_pthread(pthread_t p)
{
	return (HANDLE)((char*)0 + p);
}

static pthread_t pthread_from_HANDLE(HANDLE h)
{
	return (pthread_t)(uintptr_t)h;
}


//////////////////////////////////////////////////////////////////////////////
//
// misc
//
//////////////////////////////////////////////////////////////////////////////

pthread_t pthread_self(void)
{
	return pthread_from_HANDLE(GetCurrentThread());
}


int pthread_once(pthread_once_t* once, void (*init_routine)(void))
{
	if(CAS(once, 0, 1))
		init_routine();
	return 0;
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
		const HANDLE hThread = HANDLE_from_pthread(thread);
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
	const HANDLE hThread = HANDLE_from_pthread(thread);
	SetThreadPriority(hThread, pri);
	return 0;
}


//////////////////////////////////////////////////////////////////////////////
//
// thread-local storage
//
//////////////////////////////////////////////////////////////////////////////

// minimum amount of TLS slots every Windows version provides;
// used to validate indices.
static const uint TLS_LIMIT = 64;

// rationale: don't use an array of dtors for every possible TLS slot.
// other DLLs may allocate any number of them in their DllMain, so the
// array would have to be quite large. instead, store both key and dtor -
// we are thus limited only by pthread_key_create calls (which we control).
static const uint MAX_DTORS = 4;
static struct
{
	pthread_key_t key;
	void (*dtor)(void*);
}
dtors[MAX_DTORS];


int pthread_key_create(pthread_key_t* key, void (*dtor)(void*))
{
	DWORD idx = TlsAlloc();
	if(idx == TLS_OUT_OF_INDEXES)
		return -ENOMEM;

	debug_assert(idx < TLS_LIMIT);
	*key = (pthread_key_t)idx;

	// acquire a free dtor slot
	uint i;
	for(i = 0; i < MAX_DTORS; i++)
	{
		if(CAS(&dtors[i].dtor, 0, dtor))
			goto have_slot;
	}

	// not enough slots; we have a valid key, but its dtor won't be called.
	debug_warn("increase pthread MAX_DTORS");
	return -1;

have_slot:
	dtors[i].key = *key;
	return 0;
}


int pthread_key_delete(pthread_key_t key)
{
	DWORD idx = (DWORD)key;
	debug_assert(idx < TLS_LIMIT);

	BOOL ret = TlsFree(idx);
	debug_assert(ret != 0);
	return 0;
}


void* pthread_getspecific(pthread_key_t key)
{
	DWORD idx = (DWORD)key;
	debug_assert(idx < TLS_LIMIT);

	// TlsGetValue sets last error to 0 on success (boo).
	// we don't want this to hide previous errors, so it's restored below.
	DWORD last_err = GetLastError();

	void* data = TlsGetValue(idx);

	// no error
	if(GetLastError() == 0)
	{
		// we care about performance here. SetLastError is low overhead,
		// but last error = 0 is expected.
		if(last_err != 0)
			SetLastError(last_err);
	}
	else
		debug_warn("pthread_getspecific: TlsGetValue failed");

	return data;
}


int pthread_setspecific(pthread_key_t key, const void* value)
{
	DWORD idx = (DWORD)key;
	debug_assert(idx < TLS_LIMIT);

	BOOL ret = TlsSetValue(idx, (void*)value);
	debug_assert(ret != 0);
	return 0;
}


static void call_tls_dtors()
{
again:
	bool had_valid_tls = false;

	// for each registered dtor: (call order unspecified by SUSv3)
	for(uint i = 0; i < MAX_DTORS; i++)
	{
		// is slot #i in use?
		void (*dtor)(void*) = dtors[i].dtor;
		if(!dtor)
			continue;

		// clear slot and call dtor with its previous value.
		const pthread_key_t key = dtors[i].key;
		void* tls = pthread_getspecific(key);
		if(tls)
		{
			WARN_ERR(pthread_setspecific(key, 0));

			dtor(tls);
			had_valid_tls = true;
		}
	}

	// rationale: SUSv3 says we're allowed to loop infinitely. we do so to
	// expose any dtor bugs - this shouldn't normally happen.
	if(had_valid_tls)
		goto again;
}


//////////////////////////////////////////////////////////////////////////////
//
// threads
//
//////////////////////////////////////////////////////////////////////////////

// _beginthreadex cannot call the user's thread function directly due to
// differences in calling convention; we need to pass its address and
// the user-specified data pointer to our trampoline.
// a) a local variable in pthread_create isn't safe because the
//    new thread might not start before pthread_create returns.
// b) allocating on the heap would work but we're trying to keep that
//    to a minimum.
// c) we therefore use static data protected by a critical section.
static struct FuncAndArg
{
	void* (*func)(void*);
	void* arg;
}
func_and_arg;


// bridge calling conventions required by _beginthreadex and POSIX.
static unsigned __stdcall thread_start(void* UNUSED(param))
{
	void* (*func)(void*) = func_and_arg.func;
	void* arg           = func_and_arg.arg;
	win_unlock(WPTHREAD_CS);

	void* ret = (void*)-1;
	__try
	{
		ret = func(arg);
	}
	__except(wdbg_exception_filter(GetExceptionInformation()))
	{
	}

	call_tls_dtors();

	return (unsigned)(uintptr_t)ret;
}


int pthread_create(pthread_t* thread_id, const void* UNUSED(attr), void* (*func)(void*), void* arg)
{
	win_lock(WPTHREAD_CS);
	func_and_arg.func = func;
	func_and_arg.arg  = arg;

	// _beginthreadex has more overhead and no value added vs.
	// CreateThread, but it avoids small memory leaks in
	// ExitThread when using the statically-linked CRT (-> MSDN).
	const uintptr_t id = _beginthreadex(0, 0, thread_start, 0, 0, 0);
	if(!id)
	{
		win_unlock(WPTHREAD_CS);
		debug_warn("_beginthreadex failed");
		return -1;
	}

	// SUSv3 doesn't specify whether this is optional - go the safe route.
	if(thread_id)
		*thread_id = (pthread_t)id;

	return 0;
}


int pthread_cancel(pthread_t thread)
{
	HANDLE hThread = HANDLE_from_pthread(thread);
	TerminateThread(hThread, 0);
	debug_printf("WARNING: pthread_cancel is unsafe\n");
	return 0;
}


int pthread_join(pthread_t thread, void** value_ptr)
{
	HANDLE hThread = HANDLE_from_pthread(thread);

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
//
// locks
//
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
int pthread_mutex_timedlock(pthread_mutex_t* UNUSED(m), const struct timespec* UNUSED(abs_timeout))
{
	return -ENOSYS;
}


//////////////////////////////////////////////////////////////////////////////


static HANDLE HANDLE_from_sem_t(sem_t* sem)
{
	return (HANDLE)*sem;
}

int sem_init(sem_t* sem, int pshared, unsigned value)
{
	SECURITY_ATTRIBUTES sec = { sizeof(SECURITY_ATTRIBUTES) };
	sec.bInheritHandle = (BOOL)pshared;
	HANDLE h = CreateSemaphore(&sec, (LONG)value, 0x7fffffff, 0);
	WARN_IF_FALSE(h);
	*sem = (uintptr_t)h;
	return 0;
}

int sem_post(sem_t* sem)
{
	HANDLE h = HANDLE_from_sem_t(sem);
	WARN_IF_FALSE(ReleaseSemaphore(h, 1, 0));
	return 0;
}

int sem_wait(sem_t* sem)
{
	HANDLE h = HANDLE_from_sem_t(sem);
	DWORD ret = WaitForSingleObject(h, INFINITE);
	if(ret != WAIT_OBJECT_0)
		debug_warn("unexpected WaitForSingleObject return value");
	return 0;
}

int sem_destroy(sem_t* sem)
{
	HANDLE h = HANDLE_from_sem_t(sem);
	WARN_IF_FALSE(CloseHandle(h));
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

	HANDLE h = HANDLE_from_sem_t(sem);
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


// wait until semaphore is locked or a message arrives. non-portable.
//
// background: on Win32, UI threads must periodically pump messages, or
// else deadlock may result (see WaitForSingleObject docs). that entails
// avoiding any blocking functions. when event waiting is needed,
// one cheap workaround would be to time out periodically and pump messages.
// that would work, but either wastes CPU time waiting, or introduces
// message latency. to avoid this, we provide an API similar to sem_wait and
// sem_timedwait that gives MsgWaitForMultipleObjects functionality.
//
// return value: 0 if the semaphore has been locked (SUS terminology),
// -1 otherwise. errno differentiates what happened: ETIMEDOUT if a
// message arrived (this is to ease switching between message waiting and
// periodic timeout), or an error indication.
int sem_msgwait_np(sem_t* sem)
{
	HANDLE h = HANDLE_from_sem_t(sem);
	DWORD ret = MsgWaitForMultipleObjects(1, &h, FALSE, INFINITE, QS_ALLEVENTS);
	// semaphore is signalled
	if(ret == WAIT_OBJECT_0)
		return 0;

	// something else:
	// .. message came up
	if(ret == WAIT_OBJECT_0+1)
		errno = ETIMEDOUT;
	// .. error
	else
	{
		errno = EINVAL;
		debug_warn("unexpected MsgWaitForMultipleObjects return value");
	}
	return -1;

}