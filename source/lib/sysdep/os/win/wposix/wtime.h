/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * emulate POSIX high resolution timer on Windows.
 */

#ifndef INCLUDED_WTIME
#define INCLUDED_WTIME


//
// <sys/types.h>
//

typedef unsigned long useconds_t;
typedef long suseconds_t;


//
// <unistd.h>
//

LIB_API unsigned sleep(unsigned sec);
LIB_API int usleep(useconds_t us);


//
// <time.h>
//

typedef enum
{
	// in our implementation, these actually do the same thing
	// (a timer that latches the system time at startup and uses the
	// monotonic HRT to add elapsed time since then)
	CLOCK_REALTIME,
	CLOCK_MONOTONIC
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

LIB_API char* strptime(const char* buf, const char* format, struct tm* timeptr);

#endif	// #ifndef INCLUDED_WTIME
