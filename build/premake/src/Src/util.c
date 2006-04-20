/**********************************************************************
 * Premake - util.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "premake.h"
#include "platform.h"

static char* CPP_EXT[] = { ".cc", ".cpp", ".cxx", ".c", ".s", ".asm", NULL };

/* Buffer for generators */
char g_buffer[8192];

/* Buffer for internal use */
static char buffer[8192];


/************************************************************************
 * Checks a pattern against the end of a string
 ***********************************************************************/

int endsWith(const char* haystack, const char* needle)
{
	if (strlen(haystack) < strlen(needle))
		return 0;

	haystack = haystack + strlen(haystack) - strlen(needle);
	return (strcmp(haystack, needle) == 0);
}


/************************************************************************
 * Create a pseudo-UUID, good enough for Premake's purposes
 ***********************************************************************/

static void stringify(char* src, char* dst, int count)
{
	char buffer[4];
	int  i;

	for (i = 0; i < count; ++i)
	{
		unsigned value = (unsigned char)src[i];
		sprintf(buffer, "%X", value);

		if (value >= 0x10)
		{
			*(dst++) = buffer[0];
			*(dst++) = buffer[1];
		}
		else
		{
			*(dst++) = '0';
			*(dst++) = buffer[0];
		}
	}
}

void generateUUID(char* uuid)
{
	platform_getuuid(buffer);

	stringify(buffer, uuid, 4);
	uuid[8] = '-';
	stringify(buffer + 4, uuid + 9, 2);
	uuid[13] = '-';
	stringify(buffer + 6, uuid + 14, 2);
	uuid[18] = '-';
	stringify(buffer + 8, uuid + 19, 2);
	uuid[23] = '-';
	stringify(buffer + 10, uuid + 24, 6);
	uuid[36] = '\0';
}


/************************************************************************
 * Checks a file name against a list of known C/C++ file extensions
 ***********************************************************************/

int is_cpp(const char* name)
{
	int i;
	const char* ext = path_getextension(name);
	if (ext == NULL) 
		return 0;

	for (i = 0; CPP_EXT[i] != NULL; ++i)
	{
		if (matches(ext, CPP_EXT[i]))
			return 1;
	}

	return 0;
}


/************************************************************************
 * Checks to see if two strings match. Just a more readable version
 * of the standard strcmp() function
 ***********************************************************************/

int matches(const char* str0, const char* str1)
{
	if (str0 == NULL || str1 == NULL)
		return (str0 == str1);

	return (strcmp(str0, str1) == 0);
}


/************************************************************************
 * Iterate through an array of string, calling a function for each
 ***********************************************************************/

void print_list(const char** list, const char* prefix, const char* postfix, const char* infix, const char* (*func)(const char*))
{
	int i = 0;
	while (*list)
	{
		const char* value = (func != NULL) ? func(*list) : *list;
		if (value != NULL)
		{
			if (i++ > 0) 
				io_print(infix);
			io_print("%s%s%s", prefix, value, postfix);
		}
		++list;
	}
}


/************************************************************************
 * Iterate through the list of files, build a tree structure
 ***********************************************************************/

void print_source_tree(const char* path, void (*cb)(const char*, int))
{
	const char** i;

	/* Open an enclosing group */
	strcpy(buffer, path);
	if (buffer[strlen(buffer) - 1] == '/')   /* Trim off trailing path separator */
		buffer[strlen(buffer) - 1] = '\0';
	cb(buffer, WST_OPENGROUP);

	for (i = prj_get_files(); *i != NULL; ++i)
	{
		const char* source = (*i);

		/* For each file in the target directory... */
		if (strlen(source) > strlen(path) && strncmp(source, path, strlen(path)) == 0)
		{
			/* Look for a subdirectory... */
			const char* ptr = strchr(source + strlen(path), '/');
			if (ptr != NULL)
			{
				const char** j;

				/* Pull out the subdirectory name */
				strncpy(buffer, source, ptr - source + 1);
				buffer[ptr - source + 1] = '\0';

				/* Have I processed this subdirectory already? Check to see if
				 * I encountered the same subdir name earlier in the list */
				for (j = prj_get_files(); *j != NULL; ++j)
				{
					if (strncmp(buffer, *j, strlen(buffer)) == 0)
						break;
				}

				if (i == j)
				{
					/* Not processed earlier, process it now. Make a local copy
					 * because 'buffer' will be reused in the next call */
					char* newpath = (char*)malloc(strlen(buffer) + 1);
					strcpy(newpath, buffer);
					print_source_tree(newpath, cb);
					free(newpath);
				}
			}
		}
	}

	/* Now send all files that live in 'path' */
	for (i = prj_get_files(); *i != NULL; ++i)
	{
		const char* source = (*i);
		const char* ptr = strrchr(source, '/');
		/* Make sure file is in path and not a subdir under path */
		if (strncmp(path, source, strlen(path)) == 0 && ptr <= source + strlen(path))
			cb(source, WST_SOURCEFILE);
	}

	/* Close the enclosing group */
	strcpy(buffer, path);
	if (buffer[strlen(buffer)-1] == '/')   /* Trim off trailing path separator */
		buffer[strlen(buffer)-1] = '\0';
	cb(buffer, WST_CLOSEGROUP);
}

