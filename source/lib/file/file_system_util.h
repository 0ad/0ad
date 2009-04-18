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
 * File        : file_system_util.h
 * Project     : 0 A.D.
 * Description : helper functions for directory access
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_FILE_SYSTEM_UTIL
#define INCLUDED_FILE_SYSTEM_UTIL

#include "lib/file/vfs/vfs.h"

extern void fs_SortFiles(FileInfos& files);
extern void fs_SortDirectories(DirectoryNames& directories);

extern LibError fs_GetPathnames(const PIVFS& fs, const VfsPath& path, const char* filter, VfsPaths& pathnames);


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
typedef LibError (*FileCallback)(const VfsPath& pathname, const FileInfo& fileInfo, const uintptr_t cbData);

enum DirFlags
{
	DIR_RECURSIVE = 1
};

/**
 * call back for each file in a directory tree
 *
 * @param cb see DirCallback
 * @param pattern that file names must match. '*' and '&' wildcards
 * are allowed. 0 matches everything.
 * @param flags see DirFlags
 * @param LibError
 **/
extern LibError fs_ForEachFile(const PIVFS& fs, const VfsPath& path, FileCallback cb, uintptr_t cbData, const char* pattern = 0, size_t flags = 0);


/**
 * determine the next available pathname with a given format.
 * this is useful when creating new files without overwriting the previous
 * ones (screenshots are a good example).
 *
 * @param pathnameFormat format string for the pathname; must contain one
 * format specifier for a size_t.
 * example: "screenshots/screenshot%04d.png"
 * @param nextNumber in: the first number to try; out: the next number.
 * if 0, numbers corresponding to existing files are skipped.
 * @param nextPathname receives the output.
 **/
extern void fs_NextNumberedFilename(const PIVFS& fs, const VfsPath& pathnameFormat, size_t& nextNumber, VfsPath& nextPathname);

#endif	 // #ifndef INCLUDED_FILE_SYSTEM_UTIL
