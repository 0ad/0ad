/**
 * =========================================================================
 * File        : file_system_posix.h
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths and provides fast I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE_SYSTEM_POSIX
#define INCLUDED_FILE_SYSTEM_POSIX

#include "lib/file/path.h"
#include "lib/file/file_system.h"

struct FileSystem_Posix
{
	virtual LibError GetFileInfo(const Path& pathname, FileInfo* fileInfo) const;
	virtual LibError GetDirectoryEntries(const Path& path, FileInfos* files, DirectoryNames* subdirectoryNames) const;

	LibError DeleteFile(const Path& pathname);
	LibError CreateDirectory(const Path& dirPath);
	LibError DeleteDirectory(const Path& dirPath);
};

#endif	// #ifndef INCLUDED_FILE_SYSTEM_POSIX
