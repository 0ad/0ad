/**
 * =========================================================================
 * File        : vfs_lookup.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_LOOKUP
#define INCLUDED_VFS_LOOKUP

// VFS paths are of the form: "(dir/)*file?"
// in English: '/' as path separator; trailing '/' required for dir names;
// no leading '/', since "" is the root dir.

// there is no restriction on path length; when dimensioning character
// arrays, prefer PATH_MAX.

// pathnames are case-insensitive.
// implementation:
//   when mounting, we get the exact filenames as reported by the OS;
//   we allow open requests with mixed case to match those,
//   but still use the correct case when passing to other libraries
//   (e.g. the actual open() syscall).
// rationale:
//   necessary, because some exporters output .EXT uppercase extensions
//   and it's unreasonable to expect that users will always get it right.


class VfsFile;
class VfsDirectory;

enum VfsLookupFlags
{
	// when encountering subdirectory components in the path(name) that
	// don't (yet) exist, add them.
	VFS_LOOKUP_CREATE = 1,

	VFS_LOOKUP_NO_POPULATE = 2
};

extern LibError vfs_Lookup(const char* vfsPathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile*& file, unsigned flags = 0);

/**
 * Find a file in the VFS by traversing pathname components.
 *
 * @param vfsPath must not end with a slash character.
 **/
extern VfsFile* vfs_LookupFile(const char* vfsPathname, const VfsDirectory* startDirectory, unsigned flags = 0);

/**
 * Find a directory in the VFS by traversing path components.
 *
 * @param vfsPath must end with a slash character.
 **/
extern VfsDirectory* vfs_LookupDirectory(const char* vfsPath, const VfsDirectory* startDirectory, unsigned flags = 0);

#endif	// #ifndef INCLUDED_VFS_LOOKUP
