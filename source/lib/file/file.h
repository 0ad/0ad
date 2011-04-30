/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * simple POSIX file wrapper.
 */

#ifndef INCLUDED_FILE
#define INCLUDED_FILE

#include "lib/os_path.h"
#include "lib/posix/posix_aio.h"	// opcode: LIO_READ or LIO_WRITE

namespace ERR
{
	const LibError FILE_ACCESS = -110300;
	const LibError FILE_NOT_FOUND = -110301;
}

LIB_API LibError FileOpen(const OsPath& pathname, int opcode, int& fd);
LIB_API void FileClose(int& fd);

class File
{
public:
	File()
		: pathname(), fd(-1)
	{
	}

	File(const OsPath& pathname, int opcode)
	{
		(void)Open(pathname, opcode);
	}

	~File()
	{
		Close();
	}

	LibError Open(const OsPath& pathname, int opcode)
	{
		RETURN_ERR(FileOpen(pathname, opcode, fd));
		this->pathname = pathname;
		this->opcode = opcode;
		return INFO::OK;
	}

	void Close()
	{
		FileClose(fd);
	}

	const OsPath& Pathname() const
	{
		return pathname;
	}

	int Descriptor() const
	{
		return fd;
	}

	int Opcode() const
	{
		return opcode;
	}

private:
	OsPath pathname;
	int fd;
	int opcode;
};

typedef shared_ptr<File> PFile;

#endif	// #ifndef INCLUDED_FILE
