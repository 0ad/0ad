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

#ifndef INCLUDED_WDLFCN
#define INCLUDED_WDLFCN

//
// <dlfcn.h>
//

// these have no meaning for the Windows GetProcAddress implementation,
// so they are ignored but provided for completeness.
#define RTLD_LAZY   0x01
#define RTLD_NOW    0x02
#define RTLD_GLOBAL 0x04	// semantics are unsupported, so complain if set.
#define RTLD_LOCAL  0x08

extern int dlclose(void* handle);
extern char* dlerror(void);
extern void* dlopen(const char* so_name, int flags);
extern void* dlsym(void* handle, const char* sym_name);

#endif	// #ifndef INCLUDED_WDLFCN
