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


class File_Posix : public IFile
{
public:
	~File_Posix()
	{
		Close();
	}

	virtual LibError Open(const fs::path& pathname, char mode)
	{
		debug_assert(mode == 'w' || mode == 'r');

		m_pathname = pathname;
		m_mode = mode;

		int oflag = (mode == 'r')? O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
#if OS_WIN
		oflag |= O_BINARY_NP;
#endif
		m_fd = open(m_pathname.external_file_string().c_str(), oflag, S_IRWXO|S_IRWXU|S_IRWXG);
		if(m_fd < 0)
			WARN_RETURN(ERR::FILE_ACCESS);

		stats_open();
		return INFO::OK;
	}

	virtual LibError Open(const fs::wpath& pathname, char mode)
	{
		char pathname_c[PATH_MAX];
		size_t numConverted = wcstombs(pathname_c, pathname.file_string().c_str(), PATH_MAX);
		debug_assert(numConverted < PATH_MAX);
		return Open(pathname_c, mode);
	}

	virtual void Close()
	{
		m_mode = '\0';

		if(m_fd)
		{
			close(m_fd);
			m_fd = 0;
		}
	}

	virtual const fs::path& Pathname() const
	{
		return m_pathname;
	}

	virtual char Mode() const
	{
		return m_mode;
	}

	virtual LibError Issue(aiocb& req, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const
	{
		memset(&req, 0, sizeof(req));
		req.aio_lio_opcode = (m_mode == 'w')? LIO_WRITE : LIO_READ;
		req.aio_buf        = (volatile void*)alignedBuf;
		req.aio_fildes     = m_fd;
		req.aio_offset     = alignedOfs;
		req.aio_nbytes     = alignedSize;
		struct sigevent* sig = 0;	// no notification signal
		aiocb* const reqs = &req;
		if(lio_listio(LIO_NOWAIT, &reqs, 1, sig) != 0)
			return LibError_from_errno();
		return INFO::OK;
	}

	virtual LibError WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize)
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

	virtual LibError Write(off_t ofs, const u8* buf, size_t size) const
	{
		return IO(ofs, const_cast<u8*>(buf), size);
	}

	virtual LibError Read(off_t ofs, u8* buf, size_t size) const
	{
		return IO(ofs, buf, size);
	}

private:
	LibError IO(off_t ofs, u8* buf, size_t size) const
	{
		ScopedIoMonitor monitor;

		lseek(m_fd, ofs, SEEK_SET);

		errno = 0;
		const ssize_t ret = (m_mode == 'w')? write(m_fd, buf, size) : read(m_fd, buf, size);
		if(ret < 0)
			return LibError_from_errno();

		const size_t totalTransferred = (size_t)ret;
		if(totalTransferred != size)
			WARN_RETURN(ERR::IO);

		monitor.NotifyOfSuccess(FI_LOWIO, m_mode, totalTransferred);
		return INFO::OK;
	}

	fs::path m_pathname;
	char m_mode;
	int m_fd;
};


PIFile CreateFile_Posix()
{
	return PIFile(new File_Posix);
}
