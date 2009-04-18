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

/*
 * look up directories/files by traversing path components.
 */

#ifndef INCLUDED_VFS_LOOKUP
#define INCLUDED_VFS_LOOKUP

#include "vfs_path.h"

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
