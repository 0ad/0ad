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

#include "path.h"

namespace ERR
{
	const LibError FILE_ACCESS = -110300;
	const LibError IO          = -110301;
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
