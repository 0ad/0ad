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

struct IFile
{
	virtual LibError Open(const Path& pathname, char mode) = 0;
	virtual LibError Open(const fs::wpath& pathname, char mode) = 0;
	virtual void Close() = 0;

	virtual const Path& Pathname() const = 0;
	virtual char Mode() const = 0;

	virtual LibError Issue(aiocb& req, off_t alignedOfs, u8* alignedBuf, size_t alignedSize) const = 0;
	virtual LibError WaitUntilComplete(aiocb& req, u8*& alignedBuf, size_t& alignedSize) = 0;

	virtual LibError Read(off_t ofs, u8* buf, size_t size) const = 0;
	virtual LibError Write(off_t ofs, const u8* buf, size_t size) const = 0;
};

typedef shared_ptr<IFile> PIFile;

LIB_API PIFile CreateFile_Posix();

#endif	// #ifndef INCLUDED_FILE
