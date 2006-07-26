/**
 * =========================================================================
 * File        : posix.h
 * Project     : 0 A.D.
 * Description : definitions for a subset of POSIX.
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

/*

[KEEP IN SYNC WITH WIKI]

this header makes available commonly used POSIX (Portable Operating System
Interface) definitions, e.g. thread, file I/O and socket APIs.
on Linux and OS X we just include the requisite headers; Win32 doesn't really
support POSIX (*), so we have to implement everything ourselves.

rationale: this is preferable to a wrapper for several reasons:
- less code (implementation is only needed on Win32)
- no lock-in (the abstraction may prevent not-designed-for operations
  that the POSIX interface would have allowed)
- familiarity (many coders already know POSIX)

if a useful definition is missing, feel free to add it!

implementation reference is the "Single Unix Specification v3"
(http://www.unix.org/online.html) - it's similar to the POSIX standard
(superset?) and freely available.


* Win32 does have a POSIX subsystem (mandated by a government contract),
but it is crippled. only apps with the PE header 'subsystem' field set to
"POSIX" can use the appendant DLL, and then they can't call the regular
Windows APIs. this is obviously unacceptable - GDI is needed to set up OpenGL.

we therefore need to emulate POSIX functions using the Win32 API.
fortunately, many POSIX functions are already implemented in the VC CRT and
need only be renamed (e.g. _open, _stat).

*/

#if OS_WIN

#include "lib/sysdep/win/wposix.h"
#include "lib/sysdep/win/win.h"

#else

// unix/linux/glibc/gcc says that this macro has to be defined when including
// stdint.h from C++ for stdint.h to define SIZE_MAX and friends
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <aio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <dlfcn.h>

#include <sys/stat.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#endif	// #if OS_WIN
