//-----------------------------------------------------------------------------
// Premake - posix.c
//
// POSIX file and directory manipulation functions.
//
// Written by Jason Perkins (jason@379.com)
// Copyright (C) 2002 by 379, Inc.
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: posix.c,v 1.10 2004/04/16 22:18:38 jason379 Exp $
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <dlfcn.h>
#include <glob.h>
#include <unistd.h>
#include <sys/stat.h>

char nativePathSep = '/';

#if defined(__linux__)
const char* osIdent = "linux";
#elif defined(__FreeBSD__)
const char* osIdent = "freebsd";
#elif defined(__NetBSD__)
const char* osIdent = "netbsd";
#elif defined(__OpenBSD__)
const char* osIdent = "openbsd";
#elif defined(__APPLE__) && defined(__MACH__)
const char* osIdent = "macosx";
#else
#error Unknown OS type.
#endif

static glob_t globbuf;
static int iGlob;

//-----------------------------------------------------------------------------

int plat_chdir(const char* path)
{
	return !chdir(path);
}

//-----------------------------------------------------------------------------

int plat_copyFile(const char* src, const char* dest)
{
	char buffer[4096];
	sprintf(buffer, "cp %s %s", src, dest);
	return (system(buffer) == 0);
}

//-----------------------------------------------------------------------------

void plat_deleteDir(const char* path)
{
	char buffer[1024];
	strcpy(buffer, "rm -rf ");
	strcat(buffer, path);
	system(buffer);
}

//-----------------------------------------------------------------------------

void plat_deleteFile(const char* path)
{
	unlink(path);
}

//-----------------------------------------------------------------------------

int findLib(const char* lib, const char* path)
{
	char buffer[2048];
	struct stat sb;

	sprintf(buffer, "%s/lib%s.so", path, lib);
	if (stat(buffer, &sb) == 0 && !S_ISDIR(sb.st_mode)) return 1;

	sprintf(buffer, "%s/%s.so", path, lib);
	if (stat(buffer, &sb) == 0 && !S_ISDIR(sb.st_mode)) return 1;

	sprintf(buffer, "%s/%s", path, lib);
	if (stat(buffer, &sb) == 0 && !S_ISDIR(sb.st_mode)) return 1;

	return 0;
}

int plat_findLib(const char* lib, char* buffer, int size)
{
	FILE* file;

	if (findLib(lib, "/usr/lib"))
	{
		strcpy(buffer, "/usr/lib");
		return 1;
	}

	file = fopen("/etc/ld.so.conf", "rt");
	if (file == NULL) return 0;

	while (!feof(file))
	{
		// Read a line and trim off any trailing whitespace
		char linebuffer[2048];
		char* ptr;

		fgets(buffer, 2048, file);
		ptr = &buffer[strlen(buffer) - 1];
		while (isspace(*ptr))
			*(ptr--) = '\0';

		if (findLib(lib, buffer))
		{
			fclose(file);
			return 1;
		}
	}

	fclose(file);
	return 0;
}

//-----------------------------------------------------------------------------

int plat_generateUUID(char* uuid)
{
	FILE* rnd = fopen("/dev/random", "rb");
	fread(uuid, 16, 1, rnd);
	fclose(rnd);
}

//-----------------------------------------------------------------------------

void plat_getcwd(char* buffer, int size)
{
	getcwd(buffer, size);
}

//-----------------------------------------------------------------------------

int plat_isAbsPath(const char* buffer)
{
	return (buffer[0] == '/');
}

//-----------------------------------------------------------------------------

int plat_dirOpen(const char* mask)
{
	globbuf.gl_offs = 0;
	globbuf.gl_pathc = 0;
	globbuf.gl_pathv = NULL;
	glob(mask, GLOB_DOOFFS | GLOB_APPEND, NULL, &globbuf);
	iGlob = -1;
}

void plat_dirClose()
{
	globfree(&globbuf);
}

int plat_dirGetNext()
{
	return (++iGlob < globbuf.gl_pathc);
}

const char* plat_dirGetName()
{
	char* ptr = strrchr(globbuf.gl_pathv[iGlob], '/');
	ptr = (ptr == NULL) ? globbuf.gl_pathv[iGlob] : ptr + 1;
	return ptr;
}

int plat_dirIsFile()
{
	return 1;
}
