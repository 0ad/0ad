/* Copyright (C) 2018 Wildfire Games.
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
#include "lib/sysdep/filesystem.h"	// O_*, S_*

namespace ERR
{
	const Status FILE_ACCESS = -110300;
	const Status FILE_NOT_FOUND = -110301;
}

// @param oflag: either O_RDONLY or O_WRONLY (in which case O_CREAT and
//   O_TRUNC are added), plus O_DIRECT if aio is desired
// @return file descriptor or a negative Status
LIB_API Status FileOpen(const OsPath& pathname, int oflag);
LIB_API void FileClose(int& fd);

class File
{
public:
	File()
		: m_PathName(), m_FileDescriptor(-1)
	{
	}

	File(const OsPath& pathname, int oflag)
	{
		THROW_STATUS_IF_ERR(Open(pathname, oflag));
	}

	~File()
	{
		Close();
	}

	Status Open(const OsPath& pathName, int openFlag)
	{
		Status ret = FileOpen(pathName, openFlag);
		RETURN_STATUS_IF_ERR(ret);
		m_PathName = pathName;
		m_FileDescriptor = static_cast<int>(ret);
		m_OpenFlag = openFlag;
		return INFO::OK;
	}

	void Close()
	{
		FileClose(m_FileDescriptor);
	}

	const OsPath& Pathname() const
	{
		return m_PathName;
	}

	int Descriptor() const
	{
		return m_FileDescriptor;
	}

	int Flags() const
	{
		return m_OpenFlag;
	}

private:
	OsPath m_PathName;
	int m_FileDescriptor;
	int m_OpenFlag;
};

typedef shared_ptr<File> PFile;

#endif	// #ifndef INCLUDED_FILE
