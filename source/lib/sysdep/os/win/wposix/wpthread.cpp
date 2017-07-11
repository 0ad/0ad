/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * emulate pthreads on Windows.
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/wposix/wpthread.h"

#include <new>
#include <process.h>

#include "lib/sysdep/cpu.h"	// cpu_CAS
#include "lib/posix/posix_filesystem.h"	// O_CREAT
#include "lib/sysdep/os/win/wposix/wposix_internal.h"
#include "lib/sysdep/os/win/wposix/wtime.h"			// timespec
#include "lib/sysdep/os/win/wseh.h"		// wseh_ExceptionFilter
#include "lib/sysdep/os/win/winit.h"

WINIT_REGISTER_CRITICAL_INIT(wpthread_Init);


static HANDLE HANDLE_from_pthread(pthread_t p)
{
	return (HANDLE)((char*)0 + p);
}

static pthread_t pthread_from_HANDLE(HANDLE h)
{
	return (pthread_t)(uintptr_t)h;
}


//-----------------------------------------------------------------------------
// misc
//-----------------------------------------------------------------------------

// non-pseudo handle so that pthread_self value is unique for each thread
static __declspec(thread) HANDLE hCurrentThread;

static void NotifyCurrentThread()
{
	// (we leave it to the OS to clean these up at process exit - threads are not created often)
	WARN_IF_FALSE(DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hCurrentThread, 0, FALSE, DUPLICATE_SAME_ACCESS));
}



int pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1 == t2;
}

pthread_t pthread_self()
{
	return pthread_from_HANDLE(hCurrentThread);
}


int pthread_once(pthread_once_t* once, void (*init_routine)())
{
	if(cpu_CAS((volatile intptr_t*)once, 0, 1))
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


//-----------------------------------------------------------------------------
// thread-local storage
//-----------------------------------------------------------------------------

// minimum amount of TLS slots every Windows version provides;
// used to validate indices.
static const size_t TLS_LIMIT = 64;

// rationale: don't use an array of dtors for every possible TLS slot.
// other DLLs may allocate any number of them in their DllMain, so the
// array would have to be quite large. instead, store both key and dtor -
// we are thus limited only by pthread_key_create calls (which we control).
static const size_t MAX_DTORS = 4;
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

	ENSURE(idx < TLS_LIMIT);
	*key = (pthread_key_t)idx;

	// acquire a free dtor slot
	size_t i;
	for(i = 0; i < MAX_DTORS; i++)
	{
		if(cpu_CAS((volatile intptr_t*)&dtors[i].dtor, (intptr_t)0, (intptr_t)dtor))
			goto have_slot;
	}

	// not enough slots; we have a valid key, but its dtor won't be called.
	WARN_IF_ERR(ERR::LIMIT);
	return -1;

have_slot:
	dtors[i].key = *key;
	return 0;
}


int pthread_key_delete(pthread_key_t key)
{
	DWORD idx = (DWORD)key;
	ENSURE(idx < TLS_LIMIT);

	BOOL ret = TlsFree(idx);
	ENSURE(ret != 0);
	return 0;
}


void* pthread_getspecific(pthread_key_t key)
{
	DWORD idx = (DWORD)key;
	ENSURE(idx < TLS_LIMIT);

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
		WARN_IF_ERR(ERR::FAIL);

	return data;
}


int pthread_setspecific(pthread_key_t key, const void* value)
{
	DWORD idx = (DWORD)key;
	ENSURE(idx < TLS_LIMIT);

	BOOL ret = TlsSetValue(idx, (void*)value);
	ENSURE(ret != 0);
	return 0;
}


static void call_tls_dtors()
{
again:
	bool had_valid_tls = false;

	// for each registered dtor: (call order unspecified by SUSv3)
	for(size_t i = 0; i < MAX_DTORS; i++)
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
			WARN_IF_ERR(pthread_setspecific(key, 0));

			dtor(tls);
			had_valid_tls = true;
		}
	}

	// rationale: SUSv3 says we're allowed to loop infinitely. we do so to
	// expose any dtor bugs - this shouldn't normally happen.
	if(had_valid_tls)
		goto again;
}


//-----------------------------------------------------------------------------
// mutex
//-----------------------------------------------------------------------------

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
// note: we use wutil_Allocate instead of new because the (no longer extant)
// memory manager used a pthread_mutex.


int pthread_mutexattr_init(pthread_mutexattr_t* UNUSED(attr))
{
	return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t* UNUSED(attr))
{
	return 0;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t* UNUSED(attr), int* type)
{
	*type = PTHREAD_MUTEX_RECURSIVE;
	return 0;
}

int pthread_mutexattr_settype(pthread_mutexattr_t* UNUSED(attr), int type)
{
	return (type == PTHREAD_MUTEX_RECURSIVE)? 0 : -ENOSYS;
}


static CRITICAL_SECTION* CRITICAL_SECTION_from_pthread_mutex_t(pthread_mutex_t* m)
{
	if(!m)
	{
		DEBUG_WARN_ERR(ERR::LOGIC);
		return 0;
	}
	return (CRITICAL_SECTION*)*m;
}

pthread_mutex_t pthread_mutex_initializer()
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)wutil_Allocate(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(cs);
	return (pthread_mutex_t)cs;
}

int pthread_mutex_destroy(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = CRITICAL_SECTION_from_pthread_mutex_t(m);
	if(!cs)
		return -1;
	DeleteCriticalSection(cs);
	wutil_Free(cs);
	*m = 0;	// cause double-frees to be noticed
	return 0;
}

int pthread_mutex_init(pthread_mutex_t* m, const pthread_mutexattr_t*)
{
	*m = pthread_mutex_initializer();
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = CRITICAL_SECTION_from_pthread_mutex_t(m);
	if(!cs)
		return -1;
	EnterCriticalSection(cs);
	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = CRITICAL_SECTION_from_pthread_mutex_t(m);
	if(!cs)
		return -1;
	const BOOL successfullyEnteredOrAlreadyOwns = TryEnterCriticalSection(cs);
	return successfullyEnteredOrAlreadyOwns? 0 : -1;
}

int pthread_mutex_unlock(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = CRITICAL_SECTION_from_pthread_mutex_t(m);
	if(!cs)
		return -1;
	LeaveCriticalSection(cs);
	return 0;
}

// not implemented - pthread_mutex is based on CRITICAL_SECTION,
// which doesn't support timeouts. use sem_timedwait instead.
int pthread_mutex_timedlock(pthread_mutex_t* UNUSED(m), const struct timespec* UNUSED(abs_timeout))
{
	return -ENOSYS;
}


//-----------------------------------------------------------------------------
// semaphore
//-----------------------------------------------------------------------------

static HANDLE HANDLE_from_sem_t(sem_t* sem)
{
	return (HANDLE)*sem;
}

sem_t* sem_open(const char* name, int oflag, ...)
{
	WinScopedPreserveLastError s;

	const bool create = (oflag & O_CREAT) != 0;
	const bool exclusive = (oflag & O_EXCL) != 0;

	// SUSv3 parameter requirements:
	ENSURE(name[0] == '/');
	ENSURE((oflag & ~(O_CREAT|O_EXCL)) == 0);	// no other bits
	ENSURE(!exclusive || create);	// excl implies creat

	// if creating, get additional parameters
	unsigned initialValue = 0;
	if(create)
	{
		va_list args;
		va_start(args, oflag);
		const mode_t mode = va_arg(args, mode_t);
		initialValue = va_arg(args, unsigned);
		va_end(args);
		ENSURE(mode == 0700 && "this implementation ignores mode_t");
	}

	// create or open
	SetLastError(0);
	const LONG maxValue = 0x7fffffff;
	const HANDLE hSemaphore = CreateSemaphore(0, (LONG)initialValue, maxValue, 0);
	if(hSemaphore == 0)
		return SEM_FAILED;
	const bool existed = (GetLastError() == ERROR_ALREADY_EXISTS);

	// caller insisted on creating anew, but it already existed
	if(exclusive && existed)
	{
		CloseHandle(hSemaphore);
		errno = EEXIST;
		return SEM_FAILED;
	}

	// caller wanted to open semaphore, but it didn't exist
	if(!create && !existed)
	{
		CloseHandle(hSemaphore);
		errno = ENOENT;
		return SEM_FAILED;
	}

	// we have to return a pointer to sem_t, and the sem_init interface
	// requires sem_t to be usable as the handle, so we'll have to
	// allocate memory here (ugh).
	sem_t* sem = new sem_t;
	*sem = (sem_t)hSemaphore;
	return sem;
}

int sem_close(sem_t* sem)
{
	// jw: not sure if this is correct according to SUSv3, but it's
	// certainly enough for MessagePasserImpl's needs.
	sem_destroy(sem);
	delete sem;
	return 0;	// success
}

int sem_unlink(const char* UNUSED(name))
{
	// see sem_close
	return 0;	// success
}

int sem_init(sem_t* sem, int pshared, unsigned value)
{
	SECURITY_ATTRIBUTES sec = { sizeof(SECURITY_ATTRIBUTES) };
	sec.bInheritHandle = (BOOL)pshared;
	HANDLE h = CreateSemaphore(&sec, (LONG)value, 0x7fffffff, 0);
	WARN_IF_FALSE(h);
	*sem = (sem_t)h;
	return 0;
}

int sem_destroy(sem_t* sem)
{
	HANDLE h = HANDLE_from_sem_t(sem);
	WARN_IF_FALSE(CloseHandle(h));
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
	ENSURE(ret == WAIT_OBJECT_0);
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
	if(length_ms >= 0xFFFFFFFF)
	{
		WARN_IF_ERR(ERR::LIMIT);
		length_ms = 0xfffffffe;
	}
	return (DWORD)(length_ms & 0xFFFFFFFF);
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
		WARN_IF_ERR(ERR::FAIL);
	}
	return -1;
}


//-----------------------------------------------------------------------------
// threads
//-----------------------------------------------------------------------------

// _beginthreadex cannot call the user's thread function directly due to
// differences in calling convention; we need to pass its address and
// the user-specified data pointer to our trampoline.
//
// rationale:
// - a local variable in pthread_create isn't safe because the
//   new thread might not start before pthread_create returns.
// - using one static FuncAndArg protected by critical section doesn't
//   work. wutil_Lock allows recursive locking, so if creating 2 threads,
//   the parent thread may create both without being stopped and thus
//   stomp on the first thread's func_and_arg.
// - blocking pthread_create until the trampoline has latched func_and_arg
//   would work. this is a bit easier to understand than nonrecursive CS.
//   deadlock is impossible because thread_start allows the parent to
//   continue before doing anything dangerous. however, this requires
//   initializing a semaphore, which leads to init order problems.
// - stashing func and arg in TLS would work, but it is a very limited
//   resource and __declspec(thread) is documented as possibly
//   interfering with delay loading.
// - heap allocations are the obvious safe solution. we'd like to
//   minimize them, but it's the least painful way.

struct FuncAndArg
{
	void* (*func)(void*);
	void* arg;
};

// bridge calling conventions required by _beginthreadex and POSIX.
static unsigned __stdcall thread_start(void* param)
{
	const FuncAndArg* func_and_arg = (const FuncAndArg*)param;
	void* (*func)(void*) = func_and_arg->func;
	void* arg            = func_and_arg->arg;
	wutil_Free(param);

	NotifyCurrentThread();

	void* ret = 0;
	__try
	{
		ret = func(arg);
		call_tls_dtors();
	}
	__except(wseh_ExceptionFilter(GetExceptionInformation()))
	{
		ret = 0;
	}

	return (unsigned)(uintptr_t)ret;
}


int pthread_create(pthread_t* thread_id, const void* UNUSED(attr), void* (*func)(void*), void* arg)
{
	// notes:
	// - use wutil_Allocate instead of the normal heap because we /might/
	//   potentially be called before _cinit.
	// - placement new is more trouble than it's worth
	//   (see REDEFINED_NEW), so we don't bother with a ctor.
	FuncAndArg* func_and_arg = (FuncAndArg*)wutil_Allocate(sizeof(FuncAndArg));
	if(!func_and_arg)
	{
		WARN_IF_ERR(ERR::NO_MEM);
		return -1;
	}
	func_and_arg->func = func;
	func_and_arg->arg = arg;

	// _beginthreadex has more overhead and no value added vs.
	// CreateThread, but it avoids small memory leaks in
	// ExitThread when using the statically-linked CRT (-> MSDN).
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, thread_start, func_and_arg, 0, 0);
	if(!hThread)
	{
		WARN_IF_ERR(ERR::FAIL);
		return -1;
	}

	// SUSv3 doesn't specify whether this is optional - go the safe route.
	if(thread_id)
		*thread_id = pthread_from_HANDLE(hThread);

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
		WARN_IF_ERR(ERR::FAIL);
		return -1;
	}

	// pass back the code that was passed to pthread_exit.
	// SUS says <*value_ptr> need only be set on success!
	if(value_ptr)
		GetExitCodeThread(hThread, (LPDWORD)value_ptr);

	CloseHandle(hThread);
	return 0;
}


static Status wpthread_Init()
{
	NotifyCurrentThread();
	return INFO::OK;
}
