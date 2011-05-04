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

#include "precompiled.h"
#include "lib/file/file.h"

#include "lib/sysdep/filesystem.h"	// O_*, S_*
#include "lib/file/common/file_stats.h"


STATUS_DEFINE(ERR, FILE_ACCESS, L"Insufficient access rights to open file", EACCES);
STATUS_DEFINE(ERR, FILE_NOT_FOUND, L"No such file or directory", ENOENT);


Status FileOpen(const OsPath& pathname, int opcode, int& fd)
{
	int oflag = 0;
	switch(opcode)
	{
	case LIO_READ:
		oflag = O_RDONLY;
		break;
	case LIO_WRITE:
		oflag = O_WRONLY|O_CREAT|O_TRUNC;
		break;
	default:
		DEBUG_WARN_ERR(ERR::LOGIC);
		break;
	}
#if OS_WIN
	oflag |= O_BINARY_NP;
#endif
	// prevent exploits by disallowing writes to our files by other users.
	// note that the system-wide installed cache is read-only.
	const mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;	// 0644
	fd = wopen(pathname, oflag, mode);
	if(fd < 0)
		return StatusFromErrno();	// NOWARN

	stats_open();
	return INFO::OK;
}


void FileClose(int& fd)
{
	if(fd >= 0)
	{
		wclose(fd);
		fd = -1;
	}
}
