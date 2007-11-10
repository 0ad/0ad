/**
 * =========================================================================
 * File        : fp_posix.h
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths and provides fast I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FS_POSIX
#define INCLUDED_FS_POSIX

#include "../filesystem.h"

#include "lib/path_util.h"	// PathPackage
#include "../io/io_manager.h"	// IoCallback

// layer on top of POSIX opendir/readdir/closedir that converts paths to
// the native representation and ignores non-file/directory entries.
class DirectoryIterator_Posix : public IDirectoryIterator
{
public:
	DirectoryIterator_Posix(const char* P_path);
	virtual ~DirectoryIterator_Posix();
	virtual LibError NextEntry(FilesystemEntry& fsEntry);

private:
	DIR* m_osDir;

	PathPackage m_pp;
};


struct Filesystem_Posix : public IFilesystem
{
	virtual char IdentificationCode() const
	{
		return 'F';
	}

	virtual int Precedence() const
	{
		return 1;
	} 

	virtual LibError GetEntry(const char* pathname, FilesystemEntry& fsEntry) const = 0;

	virtual LibError CreateDirectory(const char* dirPath);
	virtual LibError DeleteDirectory(const char* dirPath);
	virtual IDirectoryIterator* OpenDirectory(const char* dirPath) const;

	virtual LibError CreateFile(const char* pathname, const u8* buf, size_t size, uint flags = 0);
	virtual LibError DeleteFile(const char* pathname);
	virtual ssize_t LoadFile(const char* pathname, IoBuf& buf, uint flags = 0, IoCallback cb = 0, uintptr_t cbData = 0);
};


#endif	// #ifndef INCLUDED_FS_POSIX
