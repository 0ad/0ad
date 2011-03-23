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
#include "lib/posix/posix_aio.h"
#include "lib/file/common/file_stats.h"


ERROR_ASSOCIATE(ERR::FILE_ACCESS, L"Insufficient access rights to open file", EACCES);
ERROR_ASSOCIATE(ERR::IO, L"Error during IO", EIO);


namespace FileImpl {

LibError Open(const OsPath& pathname, wchar_t accessType, int& fd)
{
	int oflag = 0;
	switch(accessType)
	{
	case 'r':
		oflag = O_RDONLY;
		break;
	case 'w':
		oflag = O_WRONLY|O_CREAT|O_TRUNC;
		break;
	case '+':
		oflag = O_RDWR;
		break;
	default:
		debug_assert(0);
	}
#if OS_WIN
	oflag |= O_BINARY_NP;
#endif
	// prevent exploits by disallowing writes to our files by other users.
	// note that the system-wide installed cache is read-only.
	const mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;	// 0644
	fd = wopen(pathname, oflag, mode);
	if(fd < 0)
		return LibError_from_errno(false);

	stats_open();
	return INFO::OK;
}


void Close(int& fd)
{
	if(fd >= 0)
	{
		wclose(fd);
		fd = -1;
	}
}


LibError IO(int fd, wchar_t accessType, off_t ofs, u8* buf, size_t size)
{
	debug_assert(accessType == 'r' || accessType == 'w');

	ScopedIoMonitor monitor;

	lseek(fd, ofs, SEEK_SET);

	errno = 0;
	const ssize_t ret = (accessType == 'w')? write(fd, buf, size) : read(fd, buf, size);
	if(ret < 0)
		return LibError_from_errno();

	const size_t totalTransferred = (size_t)ret;
	if(totalTransferred != size)
		WARN_RETURN(ERR::IO);

	monitor.NotifyOfSuccess(FI_LOWIO, accessType, totalTransferred);
	return INFO::OK;
}


LibError Issue(aiocb& req, int fd, wchar_t accessType, off_t alignedOfs, u8* alignedBuf, size_t alignedSize)
{
	memset(&req, 0, sizeof(req));
	req.aio_lio_opcode = (accessType == 'w')? LIO_WRITE : LIO_READ;
	req.aio_buf        = (volatile void*)alignedBuf;
	req.aio_fildes     = fd;
	req.aio_offset     = alignedOfs;
	req.aio_nbytes     = alignedSize;
	struct sigevent* sig = 0;	// no notification signal
	aiocb* const reqs = &req;
	if(lio_listio(LIO_NOWAIT, &reqs, 1, sig) != 0)
		return LibError_from_errno();
	return INFO::OK;
}


LibError WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize)
{
	// wait for transfer to complete.
	while(aio_error(&req) == EINPROGRESS)
	{
		aiocb* const reqs = &req;
		aio_suspend(&reqs, 1, (timespec*)0);	// wait indefinitely
	}

	const ssize_t bytesTransferred = aio_return(&req);
	if(bytesTransferred == -1)	// transfer failed
		WARN_RETURN(ERR::IO);

	alignedBuf = (u8*)req.aio_buf;	// cast from volatile void*
	alignedSize = bytesTransferred;
	return INFO::OK;
}

}	// namespace FileImpl
