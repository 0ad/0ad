/* Copyright (C) 2022 Wildfire Games.
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

unsigned sleep(unsigned sec);
int usleep(useconds_t us);


//
// <time.h>
//

extern int nanosleep(const struct timespec* rqtp, struct timespec* rmtp);

char* strptime(const char* buf, const char* format, struct tm* timeptr);

#endif	// #ifndef INCLUDED_WTIME
