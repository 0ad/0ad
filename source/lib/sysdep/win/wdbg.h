/**
 * =========================================================================
 * File        : wdbg.h
 * Project     : 0 A.D.
 * Description : Win32 debug support code and exception handler.
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

#ifndef WDBG_H__
#define WDBG_H__

#if HAVE_MS_ASM
# define debug_break() __asm { int 3 }
#else
# error "port this or define to implementation function"
#endif

// internal use only:
extern void wdbg_set_thread_name(const char* name);

#endif	// #ifndef WDBG_H__
