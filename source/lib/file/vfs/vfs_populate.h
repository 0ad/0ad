/* Copyright (C) 2010 Wildfire Games.
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
 * populate VFS directories with files
 */

#ifndef INCLUDED_VFS_POPULATE
#define INCLUDED_VFS_POPULATE

#include "lib/file/common/real_directory.h"

class VfsDirectory;

/**
 * attach a real directory to a VFS directory.
 *
 * when the VFS directory is accessed, it will first be populated from
 * the real directory. (this delays the impact of mounting a large
 * directory, distributing the cost from startup to the first accesses
 * to each subdirectory.)
 *
 * note: the most recently attached real directory will be used when
 * creating files in the VFS directory.
 **/
extern Status vfs_Attach(VfsDirectory* directory, const PRealDirectory& realDirectory);

/**
 * populate the directory from the attached real directory.
 *
 * adds each real file and subdirectory entry to the VFS directory.
 * the full contents of any archives in the real directory are also added.
 *
 * has no effect if no directory has been attached since the last populate.
 **/
extern Status vfs_Populate(VfsDirectory* directory);

#endif	// #ifndef INCLUDED_VFS_POPULATE
