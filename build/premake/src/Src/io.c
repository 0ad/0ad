/**********************************************************************
 * Premake - io.c
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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "io.h"
#include "path.h"
#include "platform.h"

static char buffer[8192];
static FILE* file;


int io_chdir(const char* path)
{
	return platform_chdir(path);
}

int io_closefile()
{
	fclose(file);
	return 1;
}


int io_copyfile(const char* src, const char* dest)
{
	return platform_copyfile(src, dest);
}


int io_fileexists(const char* path)
{
	struct stat buf;
	if (stat(path, &buf) == 0)
	{
		if (buf.st_mode & S_IFDIR)
			return 0;
		else
			return 1;
	}
	else
	{
		return 0;
	}
}


const char* io_findlib(const char* name)
{
	if (platform_findlib(name, buffer, 8192))
		return buffer;
	else
		return NULL;
}


const char* io_getcwd()
{
	platform_getcwd(buffer, 8192);
	return buffer;
}


int io_mask_close(MaskHandle data)
{
	return platform_mask_close(data);
}


const char* io_mask_getname(MaskHandle data)
{
	return platform_mask_getname(data);
}


int io_mask_getnext(MaskHandle data)
{
	return platform_mask_getnext(data);
}


int io_mask_isfile(MaskHandle data)
{
	return platform_mask_isfile(data);
}


MaskHandle io_mask_open(const char* mask)
{
	return platform_mask_open(mask);
}


int io_mkdir(const char* path)
{
	/* Remember the current directory */
	char cwd[8192];
	platform_getcwd(cwd, 8192);

	/* Split the path and check each part in turn */
	strcpy(buffer, path);
	path = buffer;

	while (path != NULL)
	{
		char* ptr = strchr(path, '/');
		if (ptr != NULL)
			*ptr = '\0';

		platform_mkdir(path);
		platform_chdir(path);

		path = (ptr != NULL) ? ptr + 1 : NULL;
	}

	/* Restore the original working directory */
	platform_chdir(cwd);
	return 1;
}


int io_openfile(const char* path)
{
	/* Make sure that all parts of the path exist */
	io_mkdir(path_getdir(path));

	/* Now I can open the file */
	file = fopen(path, "w");
	if (file == NULL)
	{
		printf("** Unable to open file '%s' for writing\n", path);
		return 0;
	}
	else
	{
		return 1;
	}
}


void io_print(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(file, format, args);
	va_end(args);
}


int io_remove(const char* path)
{
	return platform_remove(path);
}


int io_rmdir(const char* path, const char* dir)
{
	strcpy(buffer, path);
	if (strlen(buffer) > 0) 
		strcat(buffer, "/");
	strcat(buffer, dir);
	path_translateInPlace(buffer, NULL);
	return platform_rmdir(buffer);
}



