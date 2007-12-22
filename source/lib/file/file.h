/**
 * =========================================================================
 * File        : file.h
 * Project     : 0 A.D.
 * Description : simple POSIX file wrapper.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE
#define INCLUDED_FILE

#include "path.h"

namespace ERR
{
	const LibError FILE_ACCESS = -110200;
	const LibError IO          = -110201;
}

class LIB_API File
{
public:
	File();
	~File();

	LibError Open(const Path& pathname, char mode);
	void Close();

	const Path& Pathname() const
	{
		return m_pathname;
	}

	char Mode() const
	{
		return m_mode;
	}

	LibError Issue(aiocb& req, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const;
	static LibError WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize);

	LibError Read(off_t ofs, u8* buf, size_t size) const;
	LibError Write(off_t ofs, const u8* buf, size_t size) const;

private:
	LibError IO(off_t ofs, u8* buf, size_t size) const;

	Path m_pathname;
	char m_mode;
	int m_fd;
};

typedef shared_ptr<File> PFile;

#endif	// #ifndef INCLUDED_FILE
