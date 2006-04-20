/**********************************************************************
 * Premake - io.h
 * File and directory I/O routines.
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

struct PlatformMaskData;
typedef struct PlatformMaskData* MaskHandle;

int         io_chdir(const char* path);
int         io_closefile();
int         io_copyfile(const char* src, const char* dst);
int         io_fileexists(const char* path);
const char* io_findlib(const char* name);
const char* io_getcwd();
int         io_mask_close(MaskHandle data);
const char* io_mask_getname(MaskHandle data);
int         io_mask_getnext(MaskHandle data);
int         io_mask_isfile(MaskHandle data);
MaskHandle  io_mask_open(const char* mask);
int         io_openfile(const char* path);
void        io_print(const char* format, ...);
int         io_remove(const char* path);
int         io_rmdir(const char* path, const char* dir);

