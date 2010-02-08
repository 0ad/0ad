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
 * helper functions for directory access
 */

#ifndef INCLUDED_FILE_SYSTEM_UTIL
#define INCLUDED_FILE_SYSTEM_UTIL

#include "lib/file/vfs/vfs.h"

namespace fs_util {

extern void SortFiles(FileInfos& files);
extern void SortDirectories(DirectoryNames& directories);

extern LibError GetPathnames(const PIVFS& fs, const VfsPath& path, const wchar_t* filter, VfsPaths& pathnames);


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
extern LibError ForEachFile(const PIVFS& fs, const VfsPath& path, FileCallback cb, uintptr_t cbData, const wchar_t* pattern = 0, size_t flags = 0);


/**
 * determine the next available pathname with a given format.
 * this is useful when creating new files without overwriting the previous
 * ones (screenshots are a good example).
 *
 * @param pathnameFormat format string for the pathname; must contain one
 * format specifier for an integer.
 * example: "screenshots/screenshot%04d.png"
 * @param nextNumber in: the first number to try; out: the next number.
 * if 0, numbers corresponding to existing files are skipped.
 * @param nextPathname receives the output.
 **/
extern void NextNumberedFilename(const PIVFS& fs, const VfsPath& pathnameFormat, size_t& nextNumber, VfsPath& nextPathname);

}	// namespace fs_util

#endif	 // #ifndef INCLUDED_FILE_SYSTEM_UTIL
