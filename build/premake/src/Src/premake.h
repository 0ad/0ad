/**********************************************************************
 * Premake - premake.h
 * Application globals and functions.
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

#include "io.h"
#include "path.h"
#include "project.h"
#include "util.h"

extern const char* VERSION;
extern const char* COPYRIGHT;
extern const char* HELP_MSG;

extern const char* g_cc;
extern const char* g_dotnet;
extern int         g_verbose;

int onCommand(const char* cmd, const char* arg);
