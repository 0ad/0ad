// stack trace, improved assert and exception handler for Win32
// Copyright (c) 2002-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef WDBG_H__
#define WDBG_H__

#include "lib/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
# define debug_break() __asm { int 3 }
#else
# error "port this or define to implementation function"
#endif


// TODO: remove this from here, and make all the exception debugging stuff nicer
struct _EXCEPTION_POINTERS;
typedef struct _EXCEPTION_POINTERS* PEXCEPTION_POINTERS;
extern int debug_main_exception_filter(unsigned int code, PEXCEPTION_POINTERS ep);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef WDBG_H__
