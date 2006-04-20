/**********************************************************************
 * Premake - os.c
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

#include <string.h>
#include "util.h"
#include "os.h"

static const char* os;


/* Determine the current operating system. I'm not sure how to 
 * consistently detect the Windows platform, but since that is the 
 * most common I use it as a default */
void os_detect()
{
#if defined(__linux__)
	os = "linux";
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	os = "bsd";
#elif defined(__APPLE__) && defined(__MACH__)
	os = "macosx";
#else
	os = "windows";
#endif
}


/* Retrieve the identification string */
const char* os_get()
{
	return os;
}


/* Does this identifier match the selected OS? */
int os_is(const char* name)
{
	return matches(name, os);
}


/* Does this symbol represent a valid OS identifier string? */
int os_set(const char* name)
{
	if (matches(name, "bsd")    ||
	    matches(name, "linux")  ||
	    matches(name, "macosx") ||
	    matches(name, "windows"))
	{
		os = name;
		return 1;
	}
	else
	{
		return 0;
	}
}

