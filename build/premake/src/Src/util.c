//-----------------------------------------------------------------------------
// Premake - util.c
//
// Premake utility functions
//
// Copyright (C) 2002-2003 by Jason Perkins
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: util.c,v 1.19 2004/03/27 13:42:24 jason379 Exp $
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "util.h"

char** args;

static char buffer[4096];
static char buffer2[4096];

static char pathseps[] = { '/', '\\' };

extern int  plat_chdir(const char* path);
extern int  plat_copyFile(const char* src, const char* dest);
extern void plat_deleteDir(const char* path);
extern void plat_deleteFile(const char* path);
extern int  plat_findLib(const char* lib, char* buffer, int size);
extern void plat_generateUUID(const char* uuid);
extern void plat_getcwd(char* buffer, int size);
extern int  plat_isAbsPath(const char* path);

extern int  plat_dirOpen(const char* mask);
extern void plat_dirClose();
extern int  plat_dirGetNext();
extern const char* plat_dirGetName();
extern int plat_dirIsFile();

//-----------------------------------------------------------------------------

int copyFile(const char* src, const char* dest)
{
	translatePath(src, NATIVE);  // copies into buffer2
	strcpy(buffer, buffer2);
	translatePath(dest, NATIVE);
	return plat_copyFile(buffer, buffer2);
}

//-----------------------------------------------------------------------------

void deleteDirectory(const char* directory, const char* name)
{
	strcpy(buffer, directory);
	if (strlen(directory) > 0) strcat(buffer, "/");
	strcat(buffer, name);
	plat_deleteDir(translatePath(buffer, NATIVE));
}

//-----------------------------------------------------------------------------

void deleteFile(const char* directory, const char* name, const char* extension)
{
	strcpy(buffer, directory);
	if (strlen(buffer) > 0) strcat(buffer, "/");
	strcat(buffer, name);
	strcat(buffer, extension);
	plat_deleteFile(buffer);
}

//-----------------------------------------------------------------------------

int endsWith(const char* haystack, const char* needle)
{
	if (strlen(haystack) < strlen(needle))
		return 0;

	haystack = haystack + strlen(haystack) - strlen(needle);
	return (strcmp(haystack, needle) == 0);
}

//-----------------------------------------------------------------------------

int fileExists(const char* path, const char* name, const char* extension)
{
	FILE* file;
	char  buffer[512];

	strcpy(buffer, path);
	if (path && strlen(path) > 0)
		strcat(buffer, "/");
	strcat(buffer, name);
	strcat(buffer, extension);

	file = fopen(buffer, "r");
	if (file != NULL)
	{
		fclose(file);
		return 1;
	}

	return 0;
}

//-----------------------------------------------------------------------------

const char* findSharedLib(const char* lib)
{
	if (plat_findLib(lib, buffer, 4096))
		return buffer;
	return NULL;
}

//-----------------------------------------------------------------------------

static void stringify(unsigned char* src, char* dst, int count)
{
	char buffer[4];
	int  i;

	for (i = 0; i < count; ++i)
	{
		sprintf(buffer, "%X", (int)src[i]);

		if (src[i] >= 0x10)
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
	plat_generateUUID(buffer);

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

//-----------------------------------------------------------------------------

const char* getDirectory(const char* path)
{
	char* ptr;
	if (path != NULL)
	{
		strcpy(buffer, path);
		for (ptr = buffer; *ptr; ptr++)
		{
			if (*ptr == '\\') 
				*ptr = '/';
		}

		ptr = strrchr(buffer, '/');
		if (ptr != NULL)
		{
			*(++ptr) = '\0';
			return buffer;
		}
	}

	return ".";
}

//-----------------------------------------------------------------------------

const char* getExtension(const char* path)
{
	if (path != NULL)
	{
		const char* ptr = strrchr(path, '.');
		return ptr;
	}

	return NULL;
}

//-----------------------------------------------------------------------------

const char* getFilename(const char* path, int trim)
{
	if (path != NULL)
	{
		char* ptr = strrchr(path, '/');
		ptr = (ptr != NULL) ? ++ptr : (char*)path;
		strcpy(buffer, ptr);

		if (trim)
		{
			ptr = strrchr(buffer, '.');
			if (ptr != NULL) *ptr = '\0';
		}

		return buffer;
	}

	return NULL;
}

//-----------------------------------------------------------------------------

int hasOption(char* option)
{
	char** arg;
	for (arg = args; *arg != NULL; ++arg)
	{
		if (strlen(*arg) == strlen(option) + 2 && strcmp((*arg) + 2, option) == 0)
		{
			return 1;
		}
	}
	
	return 0;
}

//-----------------------------------------------------------------------------

int inArray(const char** list, const char* value)
{
	while (*list)
	{
		if (strcmp(*list, value) == 0)
			return 1;
		++list;
	}
	return 0;
}

//-----------------------------------------------------------------------------

void insertPath(FILE* file, const char* path, int type)
{
	if (strcmp(path, ".") != 0)
		fprintf(file, translatePath(path, type));
}

//-----------------------------------------------------------------------------

int isCppFile(const char* file)
{
	int i;
	const char* ext = getExtension(file);
	for (i = 0; CPP_EXT[i] != NULL; ++i)
	{
		if (strcmp(ext, CPP_EXT[i]) == 0)
			return 1;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// This function does some gyrations to handle the case where the requested
// directory doesn't actually exist. It tries to still give a reasonable path.

const char* makeAbsolute(const char* base, const char* path)
{
	const char* ptr;

	// If the directory is already absolute I don't have to do anything
	if (plat_isAbsPath(path))
		return translatePath(path, NATIVE);

	// chdir() to the package directory
	plat_getcwd(buffer, 4096);
	if (!plat_chdir(translatePath(base, NATIVE))) 
	{
		plat_chdir(buffer);
		return NULL;
	}

	// Back my way up the tree
	ptr = path;
	while (*ptr == '.' || *ptr == '/')
	{
		if (strncmp(ptr, "..", 2) == 0)
		{
			plat_chdir("..");
			ptr += 2;
		}
		else 
		{
			ptr++;
		}
	}


/* OBSOLETE
	while (*ptr == '.')
	{
		if (strncmp(ptr, "../", 3) == 0)
		{
			plat_chdir("..");
			ptr += 3;
		}
		else if (strncmp(ptr, "./", 2) == 0)
		{
			ptr += 2;
		}
		else if (*(ptr + 1) == '\0')
		{
			ptr++;
		}
	}
*/

	// Tack the remained of the path onto my current location
	plat_getcwd(buffer2, 4096);
	strcat(buffer2, "/");
	strcat(buffer2, ptr);

	for (ptr = buffer2; *ptr; ++ptr)
	{
		if (*ptr == '/')
			*((char*)ptr) = nativePathSep;
	}

	plat_chdir(buffer);
	return buffer2;
}

//-----------------------------------------------------------------------------

FILE* openFile(const char* path, const char* name, const char* extension)
{
	FILE* file;
	char  buffer[512];

	strcpy(buffer, path);
	if (path && strlen(path) > 0)
		strcat(buffer, "/");
	strcat(buffer, name);
	strcat(buffer, extension);

	file = fopen(buffer, "wt");
	if (file == NULL)
		printf("** Error: unable to open file '%s'\n", buffer);

	return file;
}

//-----------------------------------------------------------------------------

const char* replaceChars(const char* str, const char* replace)
{
	int i;
	for (i = 0; i < (int)strlen(str); ++i)
		buffer[i] = (strchr(replace, str[i]) == NULL) ? str[i] : '_';
	buffer[strlen(str)] = '\0';
	return buffer;
}

//-----------------------------------------------------------------------------

const char* replaceExtension(const char* path, const char* extension)
{
	char* ptr = strrchr(path, '.');
	if (ptr != NULL)
	{
		strncpy(buffer, path, ptr - path);
		buffer[ptr - path] = '\0';
	}
	else
		strcpy(buffer, path);
	strcat(buffer, extension);
	return buffer;
}

//-----------------------------------------------------------------------------

const char* reversePath(const char* path, const char* subdir, int type)
{
	char origin[2048];
	char destination[2048];
	int  start, i;

	// Get the full path to both locations
	strcpy(origin, makeAbsolute(".", path));
	strcpy(destination, makeAbsolute(".", subdir));

	// Trim off common directories
	start = 0;
	i = 0;
	while (origin[i] && destination[i] && origin[i] == destination[i])
	{
		if (origin[i] == nativePathSep)
			start = i + 1;
		++i;
	}

	// Build the return path
	if (strlen(origin) - start > 0)
	{
		int i = start;
		strcpy(buffer, "..");
		for ( ; origin[i]; ++i)
		{
			if (origin[i] == nativePathSep && origin[i+1] != '\0')
				strcat(buffer, "/..");
		}
	}
	else
	{
		strcpy(buffer, ".");
	}

	if (strlen(destination) - start > 0)
	{
		strcat(buffer, "/");
		strcat(buffer, destination + start);
	}

	// It must end with a separator
	if (buffer[strlen(buffer)-1] != '/' && buffer[strlen(buffer)-1] != '\\') strcat(buffer, "/");

	return translatePath(buffer, type);
}

//-----------------------------------------------------------------------------

const char* translatePath(const char* path, int type)
{
	char* ptr;
	char sep = (type == NATIVE) ? type = nativePathSep : pathseps[type];

	strcpy(buffer2, path);
	for (ptr = buffer2; *ptr != '\0'; ++ptr)
	{
		if (*ptr == '/' || *ptr == '\\') 
			*ptr = sep;
	}

	return buffer2;
}

//-----------------------------------------------------------------------------

void walkSourceList(FILE* file, Package* package, const char* path, void (*cb)(FILE*, const char*, int))
{
	const char** i;

	// Open the group
	strcpy(buffer, path);
	if (buffer[strlen(buffer)-1] == '/') buffer[strlen(buffer)-1] = '\0';
	cb(file, buffer, WST_OPENGROUP);

	for (i = package->files; *i; ++i)
	{
		const char* source = (*i);

		// For each file in the target directory...
		if (strlen(source) > strlen(path) && strncmp(source, path, strlen(path)) == 0)
		{
			// Look for a subdirectory...
			const char* ptr = strchr(source + strlen(path), '/');
			if (ptr != NULL)
			{
				// Have I processed this subdirectory already?
				const char** j;
				strncpy(buffer, source, ptr - source + 1);
				buffer[ptr - source + 1] = '\0';
				for (j = package->files; *j; ++j)
				{
					if (strncmp(buffer, *j, strlen(buffer)) == 0)
						break;
				}

				if (i == j)
				{
					char* newpath = (char*)malloc(strlen(buffer) + 1);
					strcpy(newpath, buffer);
					walkSourceList(file, package, newpath, cb);
					free(newpath);
				}
			}
		}
	}

	// Send the source files
	for (i = package->files; *i; ++i)
	{
		const char* source = (*i);
		const char* ptr = strrchr(source, '/');
		if (strncmp(path, source, strlen(path)) == 0 && ptr <= source + strlen(path))
			cb(file, source, WST_SOURCEFILE);
	}

	// Close the group
	strcpy(buffer, path);
	if (buffer[strlen(buffer)-1] == '/') buffer[strlen(buffer)-1] = '\0';
	cb(file, buffer, WST_CLOSEGROUP);
}

//-----------------------------------------------------------------------------

void writeList(FILE* file, const char** list, const char* prefix, const char* postfix, const char* infix, const char* (*check)(const char*,void*), void* data)
{
	int i = 0;
	while (*list)
	{
		const char* value = (check != NULL) ? check(*list, data) : *list;
		if (value != NULL)
		{
			if (i++ > 0) fprintf(file, infix);
			fprintf(file, "%s%s%s", prefix, value, postfix);
		}
		++list;
	}
}

//-----------------------------------------------------------------------------

const char* getCwd()
{
	plat_getcwd(buffer, 4096);
	return buffer;
}

int setCwd(const char* path)
{
	return plat_chdir(path);
}

//-----------------------------------------------------------------------------

int dirOpen(const char* base, const char* mask)
{
	strcpy(buffer, base);
	strcat(buffer, "/");
	strcat(buffer, mask);
	return plat_dirOpen(translatePath(buffer, NATIVE));
}

void dirClose()
{
	plat_dirClose();
}

int dirGetNext()
{
	return plat_dirGetNext();
}

const char* dirGetName()
{
	return plat_dirGetName();
}

int dirIsFile()
{
	return plat_dirIsFile();
}
