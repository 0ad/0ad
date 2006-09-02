/**
 * =========================================================================
 * File        : posix_types.h
 * Project     : 0 A.D.
 * Description : lightweight header that defines POSIX types.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2002-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

// this header defines e.g. ssize_t and int8_t without pulling in all
// POSIX declarations.
// included from lib/types.h in place of posix.h; this helps avoid conflicts
// due to incompatible winsock definitions.

#include <wchar.h>
#include <sys/types.h>
#include <stddef.h>

#if OS_WIN
# include "lib/sysdep/win/wposix_types.h"
#else
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
#endif	// #if OS_WIN
