//-----------------------------------------------------------------------------
// Premake - windows.c
//
// MS Windows file and directory manipulation functions.
//
// Copyright (C) 2002-2004 by Jason Perkins
// Source code licensed under the GPL, see LICENSE.txt for details.
//
// $Id: windows.c,v 1.10 2004/01/09 20:48:02 jason379 Exp $
//-----------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

char nativePathSep = '\\';
const char* osIdent = "windows";

static int (__stdcall *CoCreateGuid)(char*) = NULL;
static HANDLE hDir;
static WIN32_FIND_DATA entry;
static int first;

//-----------------------------------------------------------------------------

int plat_chdir(const char* path)
{
	return SetCurrentDirectory(path);
}

//-----------------------------------------------------------------------------

int plat_copyFile(const char* src, const char* dest)
{
	return CopyFile(src, dest, FALSE);
}

//-----------------------------------------------------------------------------

void plat_deleteDir(const char* path)
{
	WIN32_FIND_DATA data;
	HANDLE hDir;

	char* buffer = (char*)malloc(strlen(path) + 6);
	strcpy(buffer, path);
	strcat(buffer, "\\*.*");
	hDir = FindFirstFile(buffer, &data);
	if (hDir == INVALID_HANDLE_VALUE)
		return;
	free(buffer);

	do
	{
		if (strcmp(data.cFileName, ".") == 0) continue;
		if (strcmp(data.cFileName, "..") == 0) continue;
		
		buffer = (char*)malloc(strlen(path) + strlen(data.cFileName) + 2);
		strcpy(buffer, path);
		strcat(buffer, "\\");
		strcat(buffer, data.cFileName);

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			plat_deleteDir(buffer);
		else
			DeleteFile(buffer);

		free(buffer);
	} while (FindNextFile(hDir, &data));
	FindClose(hDir);

	RemoveDirectory(path);
}

//-----------------------------------------------------------------------------

void plat_deleteFile(const char* path)
{
	DeleteFile(path);
}

//-----------------------------------------------------------------------------

int plat_findLib(const char* lib, char* buffer, int size)
{
	HMODULE hLib = LoadLibrary(lib);
	if (hLib != NULL)
	{
		char* ptr;

		GetModuleFileName(hLib, buffer, size);
		for (ptr = buffer; *ptr != '\0'; ++ptr)
			if (*ptr == '\\') *ptr = '/';

		ptr = strrchr(buffer, '/');
		if (ptr) *ptr = '\0';

		FreeLibrary(hLib);
		return 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------

void plat_generateUUID(char* uuid)
{
	if (CoCreateGuid == NULL)
	{
		HMODULE hOleDll = LoadLibrary("OLE32.DLL");
		*((void**)&CoCreateGuid) = GetProcAddress(hOleDll, "CoCreateGuid");
	}

	CoCreateGuid(uuid);
}

//-----------------------------------------------------------------------------

void plat_getcwd(char* buffer, int size)
{
	GetCurrentDirectory(size, buffer);
}

//-----------------------------------------------------------------------------

int plat_isAbsPath(const char* buffer)
{
	return (buffer[0] == '/' || buffer[0] == '\\' || buffer[1] == ':');
}

//-----------------------------------------------------------------------------

int plat_dirOpen(const char* mask)
{
	hDir = FindFirstFile(mask, &entry);
	first = 1;
	return (hDir != INVALID_HANDLE_VALUE);
}

void plat_dirClose()
{
	if (hDir != INVALID_HANDLE_VALUE)
		FindClose(hDir);
}

int plat_dirGetNext()
{
	if (hDir == INVALID_HANDLE_VALUE) return 0;
	if (first)
	{
		first = !first;
		return 1;
	}
	else
	{
		return FindNextFile(hDir, &entry);
	}
}

const char* plat_dirGetName()
{
	return entry.cFileName;
}

int plat_dirIsFile()
{
	return (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}
