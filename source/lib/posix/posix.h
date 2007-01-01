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

#ifndef INCLUDED_POSIX
#define INCLUDED_POSIX

#if OS_WIN
# include "lib/sysdep/win/wposix/wposix.h"
# include "lib/sysdep/win/win.h"
#endif

#include "posix_types.h"
#include "posix_aio.h"
#include "posix_dlfcn.h"
#include "posix_filesystem.h"
#include "posix_mman.h"
#include "posix_pthread.h"
#include "posix_sock.h"
#include "posix_terminal.h"
#include "posix_time.h"
#include "posix_utsname.h"

#endif	// #ifndef INCLUDED_POSIX
