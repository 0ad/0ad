/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FULogFile.h"
#include "FUFile.h"

//
// FULogFile
//

FULogFile::FULogFile(const fchar* filename)
{
	file = new FUFile(filename, FUFile::WRITE);
}

FULogFile::~FULogFile()
{
	SAFE_DELETE(file);
}

void FULogFile::WriteLine(const char* filename, uint32 linenum, const char* message, ...)
{
	WriteLine("[%s:%d]", filename, (unsigned int) linenum);

	va_list vars;
	va_start(vars, message);
	WriteLineV(message, vars);
	va_end(vars);
}

void FULogFile::WriteLine(const char* message, ...)
{
	va_list vars;
	va_start(vars, message);
	WriteLineV(message, vars);
	va_end(vars);
}

void FULogFile::WriteLineV(const char* message, va_list& vars)
{
	size_t len = strlen(message);
	char* buffer = new char[len + 1024];
	vsnprintf(buffer, len + 1024, message, vars);
	buffer[len + 1023] = 0;

	if (file->IsOpen())
	{
		file->Write(buffer, strlen(buffer));
		file->Write("\n\r", 2);
		file->Flush();
	}

	SAFE_DELETE_ARRAY(buffer);
}

void FULogFile::Flush()
{
	file->Flush();
}

#ifdef UNICODE
void FULogFile::WriteLine(const char* filename, uint32 linenum, const fchar* message, ...)
{
	WriteLine("[%s:%d]", filename, (unsigned int) linenum);

	va_list vars;
	va_start(vars, message);
	WriteLineV(message, vars);
	va_end(vars);
}

void FULogFile::WriteLine(const fchar* message, ...)
{
	va_list vars;
	va_start(vars, message);
	WriteLineV(message, vars);
	va_end(vars);
}

void FULogFile::WriteLineV(const fchar* message, va_list& vars)
{
	uint32 length = (uint32) fstrlen(message);
	fchar* buffer = new fchar[length + 256];

	fvsnprintf(buffer, length + 256, message, vars);
	buffer[length + 255] = 0;

	if (file->IsOpen())
	{
		file->Write(buffer, fstrlen(buffer));
		file->Write("\n\r", 2);
		file->Flush();
	}
	SAFE_DELETE_ARRAY(buffer);
}
#endif // UNICODE
