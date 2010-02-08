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
