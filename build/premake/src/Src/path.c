/**********************************************************************
 * Premake - path.c
 * Path handling routines.
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

#include <stdlib.h>
#include <string.h>
#include "premake.h"
#include "os.h"
#include "platform.h"

static char working[8192];
static char forpart[8192];


/************************************************************************
 * Return the full path from a relative path. Contains some extra 
 * logic to handle the case where the directory doesn't exist
 ***********************************************************************/

const char* path_absolute(const char* path)
{
	char  relative[8192];
	char* ptr;

	strcpy(relative, path);
	if (strlen(relative) == 0)
		strcpy(relative, ".");
	path_translateInPlace(relative, "posix");

	/* If the directory is already absolute I don't have to do anything */
	if (platform_isAbsolutePath(relative))
		return path;

	/* Figure out where I am currently */
	platform_getcwd(working, 8192);
	path_translateInPlace(working, "posix");

	/* Split the target path and add it in piece by piece */
	ptr = relative;
	while (ptr != NULL)
	{
		char* end = strchr(ptr, '/');
		if (end != NULL)
			*end = '\0';

		if (matches(ptr, ".."))
		{
			char* sep = strrchr(working, '/');
			if (sep != NULL)
				*sep = '\0';
		}
		else if (!matches(ptr, "."))
		{
			strcat(working, "/");
			strcat(working, ptr);
		}

		ptr = (end != NULL) ? end + 1 : NULL;
	}

	return working;
}


/************************************************************************
 * Build a path to get from `from` to `to`
 ***********************************************************************/

const char* path_build(const char* from, const char* to)
{
	char fromFull[8192];
	char toFull[8192];
	int start, i;

	/* Retrieve the full path to both locations */
	strcpy(fromFull, path_absolute(from));
	strcpy(toFull,   path_absolute(to));

	/* Append a separator to both */
	strcat(fromFull, "/");
	strcat(toFull,   "/");

	/* Trim off the common directories from the front */
	start = 0;
	i = 0;
	while (fromFull[i] != '\0' && toFull[i] != '\0' && fromFull[i] == toFull[i])
	{
		if (fromFull[i] == '/')
			start = i + 1;
		i++;
	}

	if (fromFull[i] == '\0' && toFull[i] == '\0')
		return ".";

	/* Build the connecting path */
	if (strlen(fromFull) - start > 0)
	{
		strcpy(working, "../");
		for (i = start; fromFull[i] != '\0'; ++i)
		{
			if (fromFull[i] == '/' && fromFull[i + 1] != '\0')
				strcat(working, "../");
		}
	}
	else
	{
		strcpy(working, "");
	}

	if (strlen(toFull) - start > 0)
	{
		strcat(working, toFull + start);
	}

	/* Remove the trailing slash */
	working[strlen(working) - 1] = '\0';

	/* Make sure I return something */
	if (strlen(working) == 0)
		strcpy(working, ".");

	return working;
}


/************************************************************************
 * Merges two paths
 ***********************************************************************/

const char* path_combine(const char* path0, const char* path1)
{
	strcpy(working, "");

	if (!matches(path0, ".") && !matches(path0, "./"))
		strcat(working, path0);

	path_translateInPlace(working, "posix");

	if (!matches(path1, "") && !matches(path1, ".") && !matches(path1, "./"))
	{
		if (strlen(working) > 0 && working[strlen(working) - 1] != '/')
			strcat(working, "/");
		strcat(working, path1);
	}

	path_translateInPlace(working, "posix");
	return working;
}


/************************************************************************
 * Compare two paths for equality, regardless of path structure.
 ***********************************************************************/

int path_compare(const char* path0, const char* path1)
{
	char abs0[8192];
	char abs1[8192];

	strcpy(abs0, path_absolute(path0));
	strcpy(abs1, path_absolute(path1));
	return matches(abs0, abs1);
}


/************************************************************************
 * Retrieve the portions of a path
 ***********************************************************************/

char path_getseparator(const char* type)
{
	if (type == NULL)
		type = os_get();

	if (matches(type, "windows"))
		return '\\';
	else
		return '/';
}

const char* path_getbasename(const char* path)
{
	const char* name = path_getname(path);
	char* ptr = strrchr(name, '.');
	if (ptr != NULL)
		*ptr = '\0';
	return name;
}

const char* path_getextension(const char* path)
{
	const char* ptr = strrchr(path, '.');
	return ptr;
}

const char* path_getdir(const char* path)
{
	char* ptr;

	if (path != NULL)
	{
		/* Convert path to neutral separators */
		strcpy(forpart, path);
		path_translateInPlace(forpart, "posix");

		/* Now split at last separator */
		ptr = strrchr(forpart, '/');
		if (ptr != NULL)
		{
			*ptr = '\0';
			return forpart;
		}
	}

	return "";
}

const char* path_getname(const char* path)
{
	char* ptr;

	if (path == NULL)
		return NULL;

	strcpy(forpart, path);
	path_translateInPlace(forpart, "posix");

	ptr = strrchr(forpart, '/');
	ptr = (ptr != NULL) ? ptr+1 : forpart;
	return ptr;
}


/************************************************************************
 * Build a path from a directory, a file name, and a file extension
 ***********************************************************************/

const char* path_join(const char* dir, const char* name, const char* ext)
{
	if (dir != NULL)
		strcpy(working, dir);
	else
		strcpy(working, "");

	if (strlen(working) > 0)
		strcat(working, "/");
	
	strcat(working, name);
	
	if (ext != NULL && strlen(ext) > 0)
	{
		strcat(working, ".");
		strcat(working, ext);
	}
	
	return working;
}


/************************************************************************
 * Swap one file extension for another
 ***********************************************************************/

const char* path_swapextension(const char* path, const char* from, const char* to)
{
	strcpy(working, path);
	working[strlen(path) - strlen(from)] = '\0';
	strcat(working, to);
	return working;
}


/************************************************************************
 * Translate the separators used in a path
 ***********************************************************************/

const char* path_translate(const char* path, const char* type)
{
	strcpy(working, path);
	path_translateInPlace(working, type);
	return working;
}

void path_translateInPlace(char* buffer, const char* type)
{
	char* ptr;
	for (ptr = buffer; *ptr != '\0'; ++ptr)
	{
		if (*ptr == '\\' || *ptr == '/')
			*ptr = path_getseparator(type);
	}
}
