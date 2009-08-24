/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * simple POSIX file wrapper.
 */

#include "precompiled.h"
#include "file.h"

#include "lib/file/common/file_stats.h"


ERROR_ASSOCIATE(ERR::FILE_ACCESS, "Insufficient access rights to open file", EACCES);
ERROR_ASSOCIATE(ERR::IO, "Error during IO", EIO);


fs::path path_from_wpath(const fs::wpath& pathname)
{
	char pathname_c[PATH_MAX];
	size_t numConverted = wcstombs(pathname_c, pathname.file_string().c_str(), PATH_MAX);
	debug_assert(numConverted < PATH_MAX);
	return fs::path(pathname_c);
}


namespace FileImpl {

LibError Open(const fs::path& pathname, char mode, int& fd)
{
	int oflag = -1;
	switch(mode)
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
	fd = open(pathname.file_string().c_str(), oflag, S_IRWXO|S_IRWXU|S_IRWXG);
	if(fd < 0)
		WARN_RETURN(ERR::FILE_ACCESS);

	stats_open();
	return INFO::OK;
}


void Close(int& fd)
{
	if(fd)
	{
		close(fd);
		fd = 0;
	}
}


LibError IO(int fd, char mode, off_t ofs, u8* buf, size_t size)
{
	debug_assert(mode == 'r' || mode == 'w');

	ScopedIoMonitor monitor;

	lseek(fd, ofs, SEEK_SET);

	errno = 0;
	const ssize_t ret = (mode == 'w')? write(fd, buf, size) : read(fd, buf, size);
	if(ret < 0)
		return LibError_from_errno();

	const size_t totalTransferred = (size_t)ret;
	if(totalTransferred != size)
		WARN_RETURN(ERR::IO);

	monitor.NotifyOfSuccess(FI_LOWIO, mode, totalTransferred);
	return INFO::OK;
}


LibError Issue(aiocb& req, int fd, char mode, off_t alignedOfs, u8* alignedBuf, size_t alignedSize)
{
	memset(&req, 0, sizeof(req));
	req.aio_lio_opcode = (mode == 'w')? LIO_WRITE : LIO_READ;
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
