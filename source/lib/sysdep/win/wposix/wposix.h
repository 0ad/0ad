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

// misc routines

extern int chdir(const char*);
#undef getcwd
extern char* getcwd(char*, size_t);

// user tests if available via #ifdef; can't use enum.
#define _SC_PAGESIZE      1
#define _SC_PAGE_SIZE     1
#define _SC_PHYS_PAGES    2
#define _SC_AVPHYS_PAGES  3

extern long sysconf(int name);

#endif	// #ifndef INCLUDED_WPOSIX
