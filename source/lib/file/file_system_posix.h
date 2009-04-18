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

/**
 * =========================================================================
 * File        : file_system_posix.h
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths and provides fast I/O.
 * =========================================================================
 */

#ifndef INCLUDED_FILE_SYSTEM_POSIX
#define INCLUDED_FILE_SYSTEM_POSIX

#include "lib/file/path.h"
#include "lib/file/file_system.h"

// jw 2007-12-20: we'd love to replace this with boost::filesystem,
// but basic_directory_iterator does not yet cache file_size and
// last_write_time in file_status. (they each entail a stat() call,
// which is unacceptably slow.)

struct FileSystem_Posix
{
	virtual LibError GetFileInfo(const Path& pathname, FileInfo* fileInfo) const;
	virtual LibError GetDirectoryEntries(const Path& path, FileInfos* files, DirectoryNames* subdirectoryNames) const;

	LibError DeleteDirectory(const Path& dirPath);
};

typedef shared_ptr<FileSystem_Posix> PIFileSystem_Posix;

LIB_API PIFileSystem_Posix CreateFileSystem_Posix();

#endif	// #ifndef INCLUDED_FILE_SYSTEM_POSIX
