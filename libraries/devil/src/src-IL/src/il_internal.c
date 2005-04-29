//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-IL/src/il_internal.c
//
// Description: Internal stuff for DevIL
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#include <string.h>
#include <stdlib.h>


ILimage *iCurImage = NULL;


/* Siigron: added this for Linux... a #define should work, but for some reason
	it doesn't (anyone who knows why?) */
#if !defined(_WIN32) || (defined(_WIN32) && defined(__GNUC__)) // Cygwin
	int stricmp(const char *src1, const char *src2)
	{
		return strcasecmp(src1, src2);
	}
	int strnicmp(const char *src1, const char *src2, size_t max)
	{
		return strncasecmp(src1, src2, max);
	}
#elif defined(_WIN32_WCE)
	int stricmp(const char *src1, const char *src2)
	{
		return _stricmp(src1, src2);
	}
	int strnicmp(const char *src1, const char *src2, size_t max)
	{
		return _strnicmp(src1, src2, max);
	}
#endif /* _WIN32 */

#ifdef _UNICODE //_WIN32_WCE
	int iStrCmp(const ILstring src1, const ILstring src2)
	{
		return wcsicmp(src1, src2);
	}
#else
	int iStrCmp(const ILstring src1, const ILstring src2)
	{
		return stricmp(src1, src2);
	}
#endif


//! Glut's portability.txt says to use this...
char *ilStrDup(const char *Str)
{
	char *copy;

	copy = (char*)ialloc(strlen(Str) + 1);
	if (copy == NULL)
		return NULL;
	strcpy(copy, Str);
	return copy;
}


// Because MSVC++'s version is too stupid to check for NULL...
ILuint ilStrLen(const char *Str)
{
	const char *eos = Str;

	if (Str == NULL)
		return 0;

	while (*eos++);

	return((int)(eos - Str - 1));
}


// Simple function to test if a filename has a given extension, disregarding case
ILboolean iCheckExtension(const ILstring Arg, const ILstring Ext)
{
	ILboolean PeriodFound = IL_FALSE;
	ILint i;
#ifndef _UNICODE
	char *Argu = (char*)Arg;  // pointer to arg so we don't destroy arg

	if (Arg == NULL || Ext == NULL || !strlen(Arg) || !strlen(Ext))  // if not a good filename/extension, exit early
		return IL_FALSE;

	Argu += strlen(Arg);  // start at the end


	for (i = strlen(Arg); i >= 0; i--) {
		if (*Argu == '.') {  // try to find a period 
			PeriodFound = IL_TRUE;
			break;
		}
		Argu--;
	}

	if (!PeriodFound)  // if no period, no extension
		return IL_FALSE;

	if (!stricmp(Argu+1, Ext))  // extension and ext match?
		return IL_TRUE;

#else
	wchar_t *Argu = Arg;

	if (Arg == NULL || Ext == NULL || !wcslen(Arg) || !wcslen(Ext))  // if not a good filename/extension, exit early
		return IL_FALSE;

	Argu += wcslen(Arg);  // start at the end


	for (i = wcslen(Arg); i >= 0; i--) {
		if (*Argu == '.') {  // try to find a period 
			PeriodFound = IL_TRUE;
			break;
		}
		Argu--;
	}

	if (!PeriodFound)  // if no period, no extension
		return IL_FALSE;

	if (!wcsicmp(Argu+1, Ext))  // extension and ext match?
		return IL_TRUE;

#endif//_WIN32_WCE

	return IL_FALSE;  // if all else fails, return IL_FALSE
}


ILstring iGetExtension(const ILstring FileName)
{
	ILboolean PeriodFound = IL_FALSE;
#ifndef _UNICODE
	char *Ext = FileName;
	ILint i, Len = strlen(FileName);
#else
	wchar_t *Ext = FileName;
	ILint i, Len = wcslen(FileName);
#endif//_UNICODE

	if (FileName == NULL || !Len)  // if not a good filename/extension, exit early
		return NULL;

	Ext += Len;  // start at the end

	for (i = Len; i >= 0; i--) {
		if (*Ext == '.') {  // try to find a period 
			PeriodFound = IL_TRUE;
			break;
		}
		Ext--;
	}

	if (!PeriodFound)  // if no period, no extension
		return NULL;

	return Ext+1;
}


// Checks if the file exists
ILboolean iFileExists(const ILstring FileName)
{
#ifndef _UNICODE
	FILE *CheckFile = fopen(FileName, "rb");
#else
	FILE *CheckFile = _wfopen(FileName, L"rb");
#endif//_UNICODE

	if (CheckFile) {
		fclose(CheckFile);
		return IL_TRUE;
	}
	return IL_FALSE;
}


// Last time I tried, MSVC++'s fgets() was really really screwy
ILbyte *iFgets(char *buffer, ILuint maxlen)
{
	ILuint	counter = 0;
	ILint	temp = '\0';

	while ((temp = igetc()) && temp != '\n' && temp != IL_EOF && counter < maxlen) {
		buffer[counter] = temp;
		counter++;
	}
	buffer[counter] = '\0';
	
	if (temp == IL_EOF && counter == 0)  // Only return NULL if no data was "got".
		return NULL;

	return buffer;
}
