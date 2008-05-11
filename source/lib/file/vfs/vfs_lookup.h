/**
 * =========================================================================
 * File        : vfs_lookup.h
 * Project     : 0 A.D.
 * Description : look up directories/files by traversing path components.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
extern LibError vfs_Lookup(const VfsPath& pathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile** pfile, int flags = 0);

#endif	// #ifndef INCLUDED_VFS_LOOKUP
