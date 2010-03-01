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
 * look up directories/files by traversing path components.
 */

#ifndef INCLUDED_VFS_LOOKUP
#define INCLUDED_VFS_LOOKUP

#include "lib/file/vfs/vfs_path.h"

class VfsFile;
class VfsDirectory;

// note: VfsDirectory pointers are non-const because they may be
// populated during the lookup.

enum VfsLookupFlags
{
	// add (if they do not already exist) subdirectory components
	// encountered in the path[name].
	VFS_LOOKUP_ADD = 1,

	// create a real directory
	VFS_LOOKUP_CREATE = 2
};

/**
 * Resolve a pathname.
 *
 * @param pathname
 * @param vfsStartDirectory
 * @param directory is set to the last directory component that is encountered.
 * @param file is set to 0 if there is no name component, otherwise the
 * corresponding file.
 * @param flags see VfsLookupFlags.
 * @return LibError (INFO::OK if all components in pathname exist).
 *
 * to allow noiseless file-existence queries, this does not raise warnings.
 **/
extern LibError vfs_Lookup(const VfsPath& pathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile** pfile, size_t flags = 0);

#endif	// #ifndef INCLUDED_VFS_LOOKUP
