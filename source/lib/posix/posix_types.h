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
 * lightweight header that defines POSIX types.
 */

// this header defines e.g. ssize_t and int8_t without pulling in all
// POSIX declarations.
// included from lib/types.h in place of posix.h; this helps avoid conflicts
// due to incompatible winsock definitions.

// (must come before any system headers because it fixes off_t)
#if OS_WIN
# include "lib/sysdep/os/win/wposix/wposix_types.h"
#else

#include <wchar.h>
#include <sys/types.h>
#include <stddef.h>
#include <limits.h>

// unix/linux/glibc/gcc says that this macro has to be defined when including
// stdint.h from C++ for stdint.h to define SIZE_MAX and friends
# ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
# endif
# include <stdint.h>

// but sometimes it still doesn't get defined, so define it ourselves
# ifndef SIZE_MAX
#  define SIZE_MAX ((size_t)-1)
# endif

#include <unistd.h>

#endif	// #if !OS_WIN
