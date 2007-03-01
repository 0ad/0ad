/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_LOG_FILE_H_
#define _FU_LOG_FILE_H_

class FUFile;

class FCOLLADA_EXPORT FULogFile
{
private:
	FUFile* file;

public:
	FULogFile(const fchar* filename);
	~FULogFile();

	void WriteLine(const char* filename, uint32 linenum, const char* message, ...);
	void WriteLine(const char* message, ...);
	void WriteLineV(const char* message, va_list& vars);

#ifdef UNICODE
	void WriteLine(const char* filename, uint32 line, const fchar* message, ...);
	void WriteLine(const fchar* message, ...);
	void WriteLineV(const fchar* message, va_list& vars);
#endif

	void Flush();
};

#endif // _FU_LOG_FILE_H_
