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

// included by wposix.h and wpthread.cpp

#ifndef WPTHREAD_H__
#define WPTHREAD_H__

#include "wtime.h"	// sem_timedwait


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
typedef uintptr_t pthread_once_t;
#define PTHREAD_ONCE_INIT 0	// static pthread_once_t x = PTHREAD_ONCE_INIT;

extern int pthread_once(pthread_once_t*, void (*init_routine)(void));

// thread
typedef unsigned int pthread_t;

extern pthread_t pthread_self(void);
extern int pthread_getschedparam(pthread_t thread, int* policy, struct sched_param* param);
extern int pthread_setschedparam(pthread_t thread, int policy, const struct sched_param* param);
extern int pthread_create(pthread_t* thread, const void* attr, void*(*func)(void*), void* arg);
extern int pthread_cancel(pthread_t thread);
extern int pthread_join(pthread_t thread, void** value_ptr);

// mutex
typedef void* pthread_mutex_t;	// pointer to critical section
typedef void pthread_mutexattr_t;
#define PTHREAD_MUTEX_INITIALIZER pthread_mutex_initializer()

extern pthread_mutex_t pthread_mutex_initializer(void);
extern int pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
extern int pthread_mutex_destroy(pthread_mutex_t*);
extern int pthread_mutex_lock(pthread_mutex_t*);
extern int pthread_mutex_trylock(pthread_mutex_t*);
extern int pthread_mutex_unlock(pthread_mutex_t*);
extern int pthread_mutex_timedlock(pthread_mutex_t*, const struct timespec*);

// thread-local storage
typedef unsigned int pthread_key_t;

extern int pthread_key_create(pthread_key_t*, void (*dtor)(void*));
extern int pthread_key_delete(pthread_key_t);
extern void* pthread_getspecific(pthread_key_t);
extern int   pthread_setspecific(pthread_key_t, const void* value);


//
// <semaphore.h>
//

typedef uintptr_t sem_t;

extern int sem_init(sem_t*, int pshared, unsigned value);
extern int sem_post(sem_t*);
extern int sem_wait(sem_t*);
extern int sem_timedwait(sem_t*, const struct timespec*);
extern int sem_destroy(sem_t*);

#endif	// #ifndef WPTHREAD_H__
