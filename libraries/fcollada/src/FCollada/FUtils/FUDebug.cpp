/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUDebug.h"
#include "FULogFile.h"
#include "FUStringConversion.h"

//
// FUDebug
//

FUDebug::FUDebug() {}
FUDebug::~FUDebug() {}

#if defined(LINUX) || defined(__APPLE__)
#if defined(UNICODE)
#define STRING_OUT(sz) fprintf(stderr, TO_STRING(sz).c_str()); fflush(stderr);
#else
#define STRING_OUT(sz) fprintf(stderr, sz); fflush(stderr);
#endif // UNICODE
#elif defined(WIN32)
#define STRING_OUT(sz) OutputDebugString(sz); OutputDebugString(FC("\n"))
#elif defined(__PPU__)
#define STRING_OUT(sz) { fm::string szz = FUStringConversion::ToString(sz); printf(szz.c_str()); printf("\n"); }
#endif

#ifdef _DEBUG
static void DebugString(const fchar* message)
{
	STRING_OUT(message);
}

#ifdef UNICODE
static void DebugString(const char* message)
{
	fstring str = TO_FSTRING(message);
	DebugString(str);
}
#endif // UNICODE
#else // _DEBUG
static void DebugString(const fchar* UNUSED(message)) {}

#ifdef UNICODE
static void DebugString(const char* UNUSED(message)) {}
#endif //UNICODE
#endif //_DEBUG

void FUDebug::DebugOut(uint8 verbosity, uint32 line, const char* filename, const char* message, ...)
{
	va_list vars;
	va_start(vars, message);
	FUDebug::DebugOutV(verbosity, filename, line, message, vars);
	va_end(vars);
}

void FUDebug::DebugOut(uint8 verbosity, const char* message, ...)
{
	va_list vars;
	va_start(vars, message);
	FUDebug::DebugOutV(verbosity, message, vars);
	va_end(vars);
}

void FUDebug::DebugOutV(uint8 verbosity, const char* filename, uint32 line, const char* message, va_list& vars)
{
	char buffer[256];
	snprintf(buffer, 256, "[%s@%lu] ", filename, line);
	buffer[255] = 0;
	DebugString(buffer);

	FUDebug::DebugOutV(verbosity, message, vars);
}

void FUDebug::DebugOutV(uint8 verbosity, const char* message, va_list& vars)
{
	uint32 length = (uint32) strlen(message);
	char* buffer = new char[length + 256];
	vsnprintf(buffer, length + 256, message, vars);
	length = (uint32) min(strlen(buffer), (size_t) length + 253);
	buffer[length] = '\n';
	buffer[length+1] = '\r';
	buffer[length+2] = 0;

	DebugString(buffer);
	FUError::SetCustomErrorString(buffer);
	FUError::Error((FUError::Level) verbosity, FUError::ERROR_CUSTOM_STRING, 5000);

	SAFE_DELETE_ARRAY(buffer);
}

#ifdef UNICODE
void FUDebug::DebugOut(uint8 verbosity, uint32 line, const char* filename, const fchar* message, ...)
{
	va_list vars;
	va_start(vars, message);
	FUDebug::DebugOutV(verbosity, filename, line, message, vars);
	va_end(vars);
}

void FUDebug::DebugOut(uint8 verbosity, const fchar* message, ...)
{
	va_list vars;
	va_start(vars, message);
	FUDebug::DebugOutV(verbosity, message, vars);
	va_end(vars);
}

void FUDebug::DebugOutV(uint8 verbosity, const char* filename, uint32 line, const fchar* message, va_list& vars)
{
	char buffer[256];
	snprintf(buffer, 256, "[%s@%lu] ", filename, line);
	buffer[255] = 0;
	DebugString(buffer);

	FUDebug::DebugOutV(verbosity, message, vars);
}

void FUDebug::DebugOutV(uint8 verbosity, const fchar* message, va_list& vars)
{
	uint32 length = (uint32) fstrlen(message);
	fchar* buffer = new fchar[length + 256];
	fvsnprintf(buffer, length + 256, message, vars);
	length = (uint32) min(wcslen(buffer), (size_t) length + 253);
	buffer[length] = '\n';
	buffer[length+1] = '\r';
	buffer[length+2] = 0;

	DebugString(buffer);
	FUError::SetCustomErrorString(TO_STRING((const fchar*) buffer));
	FUError::Error((FUError::Level) verbosity, FUError::ERROR_CUSTOM_STRING, 5000);

	SAFE_DELETE_ARRAY(buffer);
}
#endif // UNICODE

