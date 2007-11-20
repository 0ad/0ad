/**
 * =========================================================================
 * File        : fs_posix.h
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths and provides fast I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FS_POSIX
#define INCLUDED_FS_POSIX

#include "lib/file/filesystem.h"

struct Filesystem_Posix : public IFilesystem
{
	virtual LibError GetFileInfo(const char* pathname, FileInfo& fileInfo) const;
	virtual LibError GetDirectoryEntries(const char* path, std::vector<FileInfo>* files, std::vector<const char*>* subdirectories) const;

	LibError DeleteFile(const char* pathname);
	LibError CreateDirectory(const char* dirPath);
	LibError DeleteDirectory(const char* dirPath);
};

#endif	// #ifndef INCLUDED_FS_POSIX
