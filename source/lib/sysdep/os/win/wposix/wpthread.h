/* Copyright (c) 2010 Wildfire Games
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

#ifndef INCLUDED_WPTHREAD
#define INCLUDED_WPTHREAD


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

// changing will break pthread_setschedparam:
#define sched_get_priority_max(policy) +2
#define sched_get_priority_min(policy) -2


//
// <pthread.h>
//

// one-time init
typedef intptr_t pthread_once_t;	// required for cpu_CAS
#define PTHREAD_ONCE_INIT 0	// static pthread_once_t x = PTHREAD_ONCE_INIT;

LIB_API int pthread_once(pthread_once_t*, void (*init_routine)());

// thread
typedef uintptr_t pthread_t;

LIB_API int pthread_equal(pthread_t t1, pthread_t t2);
LIB_API pthread_t pthread_self();
LIB_API int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param);
LIB_API int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param);
LIB_API int pthread_create(pthread_t* thread, const void* attr, void* (*func)(void*), void* arg);
LIB_API int pthread_cancel(pthread_t thread);
LIB_API int pthread_join(pthread_t thread, void** value_ptr);

// mutex

typedef void* pthread_mutexattr_t;
LIB_API int pthread_mutexattr_init(pthread_mutexattr_t* attr);
LIB_API int pthread_mutexattr_destroy(pthread_mutexattr_t* attr);
enum { PTHREAD_MUTEX_RECURSIVE }; // the only one we support
LIB_API int pthread_mutexattr_gettype(const pthread_mutexattr_t* attr, int* type);
LIB_API int pthread_mutexattr_settype(pthread_mutexattr_t* attr, int type);

typedef void* pthread_mutex_t;	// pointer to critical section
LIB_API pthread_mutex_t pthread_mutex_initializer();
#define PTHREAD_MUTEX_INITIALIZER pthread_mutex_initializer()
LIB_API int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
LIB_API int pthread_mutex_destroy(pthread_mutex_t*);
LIB_API int pthread_mutex_lock(pthread_mutex_t*);
LIB_API int pthread_mutex_trylock(pthread_mutex_t*);
LIB_API int pthread_mutex_unlock(pthread_mutex_t*);
LIB_API int pthread_mutex_timedlock(pthread_mutex_t*, const struct timespec*);

// thread-local storage
typedef unsigned int pthread_key_t;

LIB_API int pthread_key_create(pthread_key_t*, void (*dtor)(void*));
LIB_API int pthread_key_delete(pthread_key_t);
LIB_API void* pthread_getspecific(pthread_key_t);
LIB_API int   pthread_setspecific(pthread_key_t, const void* value);


//
// <semaphore.h>
//

typedef uintptr_t sem_t;

#define SEM_FAILED 0

LIB_API sem_t* sem_open(const char* name, int oflag, ...);
LIB_API int sem_close(sem_t* sem);
LIB_API int sem_unlink(const char* name);
LIB_API int sem_init(sem_t*, int pshared, unsigned value);
LIB_API int sem_destroy(sem_t*);
LIB_API int sem_post(sem_t*);
LIB_API int sem_wait(sem_t*);
LIB_API int sem_timedwait(sem_t*, const struct timespec*);


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
LIB_API int sem_msgwait_np(sem_t* sem);

#endif	// #ifndef INCLUDED_WPTHREAD
