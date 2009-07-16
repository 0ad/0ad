/**********************************************************************
 * Premake - os.h
 * Manage the list of supported OSes.
 *
 * Copyright (c) 2002-2005 Jason Perkins and the Premake project
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License in the file LICENSE.txt for details.
 **********************************************************************/

/* Determine the current OS. I'm not sure how to reliably detect Windows
 * but since it is the most command I use is as the default */
#if defined(__linux__)
#define PLATFORM_POSIX 1
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define PLATFORM_POSIX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_POSIX 1
#else
#define PLATFORM_WINDOWS 1
#endif

void        os_detect();
const char* os_get();
int         os_is(const char* name);
int         os_set(const char* name);
