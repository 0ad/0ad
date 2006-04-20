/**********************************************************************
 * Premake - util.h
 * Support functions.
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

#define ALLOCT(T)   (T*)malloc(sizeof(T))

enum { WST_OPENGROUP, WST_CLOSEGROUP, WST_SOURCEFILE };

extern char g_buffer[];

int         endsWith(const char* haystack, const char* needle);
void        generateUUID(char* uuid);
int         is_cpp(const char* name);
int         matches(const char* str0, const char* str1);
void        print_list(const char** list, const char* prefix, const char* postfix, const char* infix, const char* (*func)(const char*));
void        print_source_tree(const char* path, void (*cb)(const char*, int));
