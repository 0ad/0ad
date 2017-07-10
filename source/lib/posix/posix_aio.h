/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_POSIX_AIO
#define INCLUDED_POSIX_AIO

// despite the comment in wposix.h about not using Windows headers for
// POSIX declarations, this one is harmless (no incompatible declarations)
// and can safely be used on Windows as well.
#include <fcntl.h>

#if OS_WIN
# include "lib/sysdep/os/win/wposix/waio.h"
#elif OS_ANDROID || OS_OPENBSD
// Android doesn't provide aio.h. We don't actually use aio on Linuxes (see
// CONFIG2_FILE_ENABLE_AIO) but we use its symbols and structs, so define
// them here
# if OS_OPENBSD
// OpenBSD 5.1 (latest version at time of writing) has no struct sigevent defined,
// so we do this here.
struct sigevent
{
	int sigev_notify;
	int sigev_signo;
	union sigval sigev_value;
	void (*sigev_notify_function)(union sigval);
	pthread_attr_t *sigev_notify_attributes;
};
# endif
# define LIO_READ 0
# define LIO_WRITE 1
# define LIO_NOP 2
struct aiocb
{
	int aio_fildes;
	off_t aio_offset;
	volatile void* aio_buf;
	size_t aio_nbytes;
	int aio_reqprio;
	struct sigevent aio_sigevent;
	int aio_lio_opcode;
};
#else
# include <aio.h>
#endif

#include "lib/posix/posix_errno.h"	// for user convenience

#endif	// #ifndef INCLUDED_POSIX_AIO
