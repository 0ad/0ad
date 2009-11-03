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

namespace FileImpl
{
	LIB_API LibError Open(const fs::wpath& pathname, wchar_t mode, int& fd);
	LIB_API void Close(int& fd);
	LIB_API LibError IO(int fd, wchar_t mode, off_t ofs, u8* buf, size_t size);
	LIB_API LibError Issue(aiocb& req, int fd, wchar_t mode, off_t alignedOfs, u8* alignedBuf, size_t alignedSize);
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

	LibError Open(const fs::wpath& pathname, wchar_t mode)
	{
		RETURN_ERR(FileImpl::Open(pathname, mode, m_fd));
		m_pathname = pathname;
		m_mode = mode;
		return INFO::OK;
	}

	void Close()
	{
		FileImpl::Close(m_fd);
	}

	File(const fs::wpath& pathname, wchar_t mode)
	{
		(void)Open(pathname, mode);
	}

	~File()
	{
		Close();
	}

	const fs::wpath& Pathname() const
	{
		return m_pathname;
	}

	wchar_t Mode() const
	{
		return m_mode;
	}

	LibError Issue(aiocb& req, wchar_t mode, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const
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
	fs::wpath m_pathname;
	int m_fd;
	wchar_t m_mode;
};

typedef shared_ptr<File> PFile;

#endif	// #ifndef INCLUDED_FILE
