#include "precompiled.h"

#include <new>

#include <process.h>

#include "lib.h"
#include "win_internal.h"
#include "wpthread.h"

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
	// - don't call via asm: _beginthreadex might be a func ptr (if DLL CRT).
	// - don't stack-allocate param: thread_start might not be called
	//   in the new thread before we exit this stack frame.
	ThreadParam* param = new ThreadParam(func, user_arg);
	*thread = (pthread_t)_beginthreadex(0, 0, thread_start, (void*)param, 0, 0);
	return 0;
}


void pthread_cancel(pthread_t thread)
{
	HANDLE hThread = pthread_t_to_HANDLE(thread);
	TerminateThread(hThread, 0);
}


void pthread_join(pthread_t thread, void** value_ptr)
{
	HANDLE hThread = pthread_t_to_HANDLE(thread);

	// clean exit
	if(WaitForSingleObject(hThread, 100) == WAIT_OBJECT_0)
	{
		if(value_ptr)
			GetExitCodeThread(hThread, (LPDWORD)value_ptr);
	}
	// force close
	else
		TerminateThread(hThread, 0);
	if(value_ptr)
		*value_ptr = (void*)-1;
	CloseHandle(hThread);	
}


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
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)HeapAlloc(GetProcessHeap(), 0, sizeof(CRITICAL_SECTION));
	InitializeCriticalSection(cs);
	return (pthread_mutex_t)cs;
}

int pthread_mutex_destroy(pthread_mutex_t* m)
{
	CRITICAL_SECTION* cs = (CRITICAL_SECTION*)(*m);
	DeleteCriticalSection(cs);
	HeapFree(GetProcessHeap(), 0, cs);
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

int pthread_mutex_timedlock(pthread_mutex_t* m, const struct timespec* abs_timeout)
{
	UNUSED(m);
	UNUSED(abs_timeout);
	return -ENOSYS;
}




int sem_init(sem_t* sem, int pshared, unsigned value)
{
	UNUSED(pshared);
	*sem = (uintptr_t)CreateSemaphore(0, (LONG)value, 0x7fffffff, 0);
	return 0;
}

int sem_post(sem_t* sem)
{
	ReleaseSemaphore((HANDLE)*sem, 1, 0);
	return 0;
}

int sem_wait(sem_t* sem)
{
	WaitForSingleObject((HANDLE)*sem, INFINITE);
	return 0;
}

int sem_destroy(sem_t* sem)
{
	CloseHandle((HANDLE)*sem);
	return 0;
}
