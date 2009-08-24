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

#ifndef INCLUDED_FILE
#define INCLUDED_FILE

namespace ERR
{
	const LibError FILE_ACCESS = -110300;
	const LibError IO          = -110301;
}

LIB_API fs::path path_from_wpath(const fs::wpath& pathname);

namespace FileImpl
{
	LIB_API LibError Open(const fs::path& pathname, char mode, int& fd);
	LIB_API void Close(int& fd);
	LIB_API LibError IO(int fd, char mode, off_t ofs, u8* buf, size_t size);
	LIB_API LibError Issue(aiocb& req, int fd, char mode, off_t alignedOfs, u8* alignedBuf, size_t alignedSize);
	LIB_API LibError WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize);
}


class File
{
	NONCOPYABLE(File);
public:
	File()
		: m_pathname(), m_fd(0)
	{
	}

	LibError Open(const fs::path& pathname, char mode)
	{
		RETURN_ERR(FileImpl::Open(pathname, mode, m_fd));
		m_pathname = pathname;
		m_mode = mode;
		return INFO::OK;
	}

	LibError Open(const fs::wpath& pathname, char mode)
	{
		m_pathname = path_from_wpath(pathname);
		RETURN_ERR(FileImpl::Open(m_pathname, mode, m_fd));
		m_mode = mode;
		return INFO::OK;
	}

	void Close()
	{
		FileImpl::Close(m_fd);
	}

	File(const fs::path& pathname, char mode)
	{
		(void)Open(pathname, mode);
	}

	File(const fs::wpath& pathname, char mode)
	{
		(void)Open(pathname, mode);
	}

	~File()
	{
		Close();
	}

	const fs::path& Pathname() const
	{
		return m_pathname;
	}

	char Mode() const
	{
		return m_mode;
	}

	LibError Issue(aiocb& req, char mode, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const
	{
		return FileImpl::Issue(req, m_fd, mode, alignedOfs, alignedBuf, alignedSize);
	}

	LibError Write(off_t ofs, const u8* buf, size_t size)
	{
		return FileImpl::IO(m_fd, 'w', ofs, const_cast<u8*>(buf), size);
	}

	LibError Read(off_t ofs, u8* buf, size_t size) const
	{
		return FileImpl::IO(m_fd, 'r', ofs, buf, size);
	}

private:
	fs::path m_pathname;
	int m_fd;
	char m_mode;
};

typedef shared_ptr<File> PFile;

#endif	// #ifndef INCLUDED_FILE
