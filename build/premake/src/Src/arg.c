/**********************************************************************
 * Premake - arg.c
 * Command-line argument handling.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "premake.h"
#include "arg.h"


static int    my_argc;
static char** my_argv;
static int    my_next;



void arg_set(int argc, char** argv)
{
	my_argc = argc;
	my_argv = argv;
	my_next = 1;
}



void arg_reset()
{
	my_next = 1;
}



const char* arg_getflag()
{
	if (my_next == my_argc)
		return NULL;

	return my_argv[my_next++];
}



const char* arg_getflagarg()
{
	if (my_next == my_argc)
		return NULL;

	if (strncmp(my_argv[my_next], "--", 2) == 0)
		return NULL;

	return my_argv[my_next++];
}
