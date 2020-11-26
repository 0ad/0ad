/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUFile.h"

//
// FUFile
//

FUFile::FUFile(const fstring& filename, Mode mode)
:	filePtr(NULL)
,	filepath()
{
	Open(filename, mode);
}

FUFile::FUFile(const fchar* filename, Mode mode)
:	filePtr(NULL)
,	filepath()
{
	Open(filename, mode);
}

FUFile::FUFile()
:	filePtr(NULL)
,	filepath()
{}

FUFile::~FUFile()
{
	if (filePtr != NULL)
	{
		Close();
	}
}

bool FUFile::Open(const fchar* filename, Mode mode)
{
	if (filePtr != NULL) return false;
	filepath = filename;
		
	const fchar* openMode;
	switch (mode)
	{
	case READ: openMode = FC("rb"); break;
	case WRITE: openMode = FC("wb"); break;
	default: openMode = FC("rb"); break;
	}

#ifdef UNICODE
#ifdef WIN32
	filePtr = _wfopen(filename, openMode);
#else
	// No support for Unicode filenames on MacOSX?
	// Create copies of filename and openMode, because of memory management. 
	fm::string string1 = TO_STRING(filename);
	fm::string string2 = TO_STRING(openMode);
	filePtr = fopen(string1.c_str(), string2.c_str());
#endif // WIN32
#else
	filePtr = fopen(filename, openMode);
#endif // UNICODE
	if (filePtr == NULL)
	{
#ifdef WIN32
		int err;
		if (_get_errno(&err) == 0)
		{
			char buff[512];
			buff[0] = 0; // Null terminated by default
			strerror_s(buff, 512, _doserrno);
			fm::string msg;
			msg.reserve(1024);
			msg.insert(0, "Error! Could not open file: ");
			msg.append(TO_STRING(filename));
			msg.append('\n');
			msg.append(buff);
			WARNING_OUT(msg.c_str());
		}
#endif
		return false;
	}
	return true;
}

// Retrieve the file length
size_t FUFile::GetLength()
{
	FUAssert(IsOpen(), return 0);

	size_t currentPosition = (size_t) ftell(filePtr);
	if (fseek(filePtr, 0, SEEK_END) != 0) return 0;

	size_t length = (size_t) ftell(filePtr);
	if (fseek(filePtr, (long) currentPosition, SEEK_SET) != 0) return 0;

	return length;
}

// Reads in a piece of the file into the given buffer
bool FUFile::Read(void* buffer, size_t length)
{
	FUAssert(IsOpen(), return 0);
	return (fread(buffer, 1, length, filePtr) > 0);
}

// Write out some data to a file
bool FUFile::Write(const void* buffer, size_t length)
{
	FUAssert(IsOpen(), return 0);
	return (fwrite(buffer, 1, length, filePtr) > 0);
}

// Flush/close the file stream
void FUFile::Flush()
{
	FUAssert(IsOpen(),);
	fflush(filePtr);
}

void FUFile::Close()
{
	FUAssert(IsOpen(),);
	fclose(filePtr);
	filePtr = NULL;
}
