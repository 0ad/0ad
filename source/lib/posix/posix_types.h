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
 * lightweight header that defines POSIX types.
 */

#ifndef INCLUDED_POSIX_TYPES
#define INCLUDED_POSIX_TYPES

// this header defines e.g. ssize_t and int8_t without pulling in all
// POSIX declarations.
// included from lib/types.h in place of posix.h; this helps avoid conflicts
// due to incompatible winsock definitions.

#include "lib/sysdep/os.h"	// OS_WIN

// (must come before any system headers because it fixes off_t)
#if OS_WIN
# include "lib/sysdep/os/win/wposix/wposix_types.h"
#else

// unix/linux/glibc/gcc says that this macro has to be defined when including
// stdint.h from C++ for stdint.h to define SIZE_MAX and friends
# ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
# endif

# include <math.h>
# include <wchar.h>
# include <sys/types.h>
# include <stddef.h>
# include <limits.h>
# include <stdint.h>

// but sometimes it still doesn't get defined, so define it ourselves
# ifndef SIZE_MAX
#  define SIZE_MAX ((size_t)-1)
# endif

# include <unistd.h>

#endif	// #if !OS_WIN

#endif	// #ifndef INCLUDED_POSIX_TYPES
