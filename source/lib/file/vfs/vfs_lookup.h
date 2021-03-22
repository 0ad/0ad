/* Copyright (C) 2021 Wildfire Games.
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
	// Add (if they do not already exist) subdirectory components
	// encountered in the path[name].
	// If subdirectores do not exist on disk, they will be created.
	VFS_LOOKUP_ADD = 1,

	// Don't populate the directories encountered. This makes sense
	// when adding files from an archive, which would otherwise
	// cause nearly every directory to be populated.
	VFS_LOOKUP_SKIP_POPULATE = 2,

	// Perform a 'real path' lookup.
	// Because the VFS maps multiple 'disk paths' to a single tree of paths,
	// the 'real directory' of a VFS directory at any given time may be almost anything,
	// in particular not its real parent directory on disk.
	// To make writing predictable, we'll return a path relative to the 'disk path' of the
	// highest priority subdirectory found in the lookup path.
	// See test_vfs_real_paths.h for examples of this behaviour.
	VFS_LOOKUP_REAL_PATH = 4
};

/**
 * Resolve a pathname.
 *
 * @param pathname
 * @param startDirectory VfsStartDirectory.
 * @param directory is set to the last directory component that is encountered.
 * @param pfile File is set to 0 if there is no name component, otherwise the
 *		  corresponding file.
 * @param flags @see VfsLookupFlags.
 * @return Status (INFO::OK if all components in pathname exist).
 *
 * to allow noiseless file-existence queries, this does not raise warnings.
 **/
extern Status vfs_Lookup(const VfsPath& pathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile** pfile, size_t flags = 0);

#endif	// #ifndef INCLUDED_VFS_LOOKUP
