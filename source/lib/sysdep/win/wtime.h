/**
 * =========================================================================
 * File        : wtime.h
 * Project     : 0 A.D.
 * Description : emulate POSIX high resolution timer on Windows.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef WTIME_H__
#define WTIME_H__

// advertise support for the timer routines we emulate; used by timer.cpp.
// #undef to avoid macro redefinition warning.
#undef HAVE_CLOCK_GETTIME
#define HAVE_CLOCK_GETTIME 1
#undef HAVE_GETTIMEOFDAY
#define HAVE_GETTIMEOFDAY 1


//
// <sys/types.h>
//

typedef unsigned long useconds_t;
typedef long suseconds_t;


//
// <unistd.h>
//

extern unsigned int sleep(unsigned int sec);
extern int usleep(useconds_t us);


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


// HACK: on Windows, the HRT makes its final implementation choice
// in the first calibrate call where cpu_freq is available.
// provide a routine that makes the choice when called,
// so app code isn't surprised by a timer change, although the HRT
// does try to keep the timer continuous.
extern void wtime_reset_impl(void);


#endif	// #ifndef WTIME_H__
