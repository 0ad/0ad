/**
 * =========================================================================
 * File        : dir_util.h
 * Project     : 0 A.D.
 * Description : helper functions for directory access
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_DIR_UTIL
#define INCLUDED_DIR_UTIL

#include "filesystem.h"

extern bool dir_FileExists(IFilesystem* fs, const char* pathname);

extern void dir_SortFiles(FileInfos& files);
extern void dir_SortDirectories(Directories& directories);

/**
 * called for files in a directory.
 *
 * @param pathname full pathname (since FileInfo only gives the name).
 * @param fileInfo file information
 * @param cbData user-specified context
 * @return INFO::CB_CONTINUE on success; any other value will immediately
 * be returned to the caller (no more calls will be forthcoming).
 *
 * CAVEAT: pathname and fileInfo are only valid until the function
 * returns!
 **/
typedef LibError (*DirCallback)(const char* pathname, const FileInfo& fileInfo, const uintptr_t cbData);

enum DirFlags
{
	/// include files in subdirectories.
	VFS_DIR_RECURSIVE = 1
};

/**
 * call back for each file in a directory (tree)
 *
 * @param cb see DirCallback
 * @param pattern that file names must match. '*' and '&' wildcards
 * are allowed. 0 matches everything.
 * @param flags see DirFlags
 * @param LibError
 **/
extern LibError dir_ForEachFile(IFilesystem* fs, const char* path, DirCallback cb, uintptr_t cbData, const char* pattern = 0, uint flags = 0);


/**
 * determine the next available pathname with a given format.
 * this is useful when creating new files without overwriting the previous
 * ones (screenshots are a good example).
 *
 * @param pathnameFmt format string for the pathname; must contain one
 * format specifier for an (unsigned) int.
 * example: "screenshots/screenshot%04d.png"
 * @param nextNumber in: the first number to try; out: the next number.
 * if 0, numbers corresponding to existing files are skipped.
 * @param receives the output; must hold at least PATH_MAX characters.
 **/
extern void dir_NextNumberedFilename(IFilesystem* fs, const char* pathnameFmt, unsigned& nextNumber, char* nextPathname);

#endif	 // #ifndef INCLUDED_DIR_UTIL
