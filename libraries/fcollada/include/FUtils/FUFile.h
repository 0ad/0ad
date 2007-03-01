/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_FILE_H_
#define _FU_FILE_H_

class FCOLLADA_EXPORT FUFile
{
public:
	enum Mode
	{
		READ,
		WRITE
	};

private:
	FILE* filePtr;
	
public:
	FUFile(const fstring& filename, Mode mode);
	FUFile(const fchar* filename, Mode mode);
	FUFile();
	~FUFile();
	
	inline bool Open(const fstring& filename, Mode mode) { return Open(filename.c_str(), mode); }
	bool Open(const fchar* filename, Mode mode);
	bool IsOpen() { return filePtr != NULL; }

	// Retrieve the file info
	size_t GetLength();
	FILE* GetHandle() { return filePtr; }

	// Writes/Reads in a piece of the file from/into the given buffer
	bool Read(void* buffer, size_t length);
	bool Write(const void* buffer, size_t length);

	// Flush/close the file stream
	void Flush();
	void Close();
};

#endif // _FU_FILE_H_
