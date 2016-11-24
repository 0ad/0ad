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

#include "lib/file/common/file_stats.h"

static const StatusDefinition fileStatusDefinitions[] = {
	{ ERR::FILE_ACCESS, L"Insufficient access rights to open file", EACCES },
	{ ERR::FILE_NOT_FOUND, L"No such file or directory", ENOENT }
};
STATUS_ADD_DEFINITIONS(fileStatusDefinitions);


Status FileOpen(const OsPath& pathname, int oflag)
{
	ENSURE((oflag & ~(O_RDONLY|O_WRONLY|O_DIRECT)) == 0);
	if(oflag & O_WRONLY)
		oflag |= O_CREAT|O_TRUNC;
	// prevent exploits by disallowing writes to our files by other users.
	// note that the system-wide installed cache is read-only.
	const mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;	// 0644
	const int fd = wopen(pathname, oflag, mode);
	if(fd < 0)
		return StatusFromErrno();	// NOWARN

	stats_open();
	return (Status)fd;
}


void FileClose(int& fd)
{
	if(fd >= 0)
	{
		wclose(fd);
		fd = -1;
	}
}
