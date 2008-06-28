/**********************************************************************
 * Premake - vs.h
 * Common code for Visual Studio 2002-2008 targets.
 *
 * Copyright (c) 2002-2006 Jason Perkins and the Premake project
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

enum VsVer
{
	VS2002,
	VS2003,
	VS2005,
	VS2008
};

typedef struct tagVsPkgData
{
	char projGuid[38];
	char toolGuid[38];
	char projExt[8];
	char projType[8];
	int  numDependencies;
} VsPkgData;

void vs_setversion(int version);
int  vs_getversion();

void        vs_assign_guids();
int         vs_write_cpp();
const char* vs_list_refpaths(const char* name);
const char* vs_list_pkgdeps(const char* name);
void        vs_list_files(const char* path, int stage);
const char* vs_filter_links(const char* name);
