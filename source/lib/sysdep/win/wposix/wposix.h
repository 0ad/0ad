/**
 * =========================================================================
 * File        : wposix.h
 * Project     : 0 A.D.
 * Description : emulate a subset of POSIX on Win32.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef INCLUDED_WPOSIX
#define INCLUDED_WPOSIX

/**
 * rationale: the Windows headers declare many POSIX functions (e.g. read).
 * unfortunately, these are often slightly incorrect (size_t vs. uint).
 * to avert trouble in user code caused by these differences, we declare
 * all functions ourselves according to SUSv3 and do not use the headers.
 *
 * however, it does not end there. some other libraries (e.g. wxWidgets)
 * will want to pull in these headers, which would conflict with our
 * declarations. also, our implementation uses the actual CRT code,
 * so we want those functions (e.g. _read) to be declared correctly even
 * if switching compiler/CRT version.
 *
 * how can these conflicting requirements be reconciled? our headers #include
 * "no_crt_posix.h" to #define the CRT headers' include guards and thus
 * prevent them from declaring anything. the implementation files #include
 * "crt_posix.h", which pulls in the CRT headers (even if "no_crt_posix.h"
 * was previously included, e.g. in the PCH). note that the CRT headers
 * would still cause conflicts with the POSIX function declarations,
 * but we are able to prevent this via __STDC__.
 **/


// misc routines

extern char* getcwd(char*, size_t);

// user tests if available via #ifdef; can't use enum.
#define _SC_PAGESIZE      1
#define _SC_PAGE_SIZE     1
#define _SC_PHYS_PAGES    2
#define _SC_AVPHYS_PAGES  3

extern long sysconf(int name);

#endif	// #ifndef INCLUDED_WPOSIX
