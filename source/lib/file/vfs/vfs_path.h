/**
 * =========================================================================
 * File        : vfs_path.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_VFS_PATH
#define INCLUDED_VFS_PATH

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

/**
 * Find a file in the VFS by traversing pathname components.
 *
 * @param vfsPath must not end with a slash character.
 **/
extern VfsFile* LookupFile(const char* vfsPathname, const VfsDirectory* startDirectory);

/**
 * Find a directory in the VFS by traversing path components.
 *
 * @param vfsPath must end with a slash character.
 **/
extern VfsDirectory* LookupDirectory(const char* vfsPath, const VfsDirectory* startDirectory);

/**
 * Traverse a path, creating subdirectories that didn't yet exist.
 *
 * @param lastDirectory receives the last directory that was encountered
 * @param file references the VFS file represented by vfsPath, or 0 if
 * it doesn't include a filename.
 **/
extern void TraverseAndCreate(const char* vfsPath, VfsDirectory* startDirectory, VfsDirectory*& lastDirectory, VfsFile*& file);

#endif	// #ifndef INCLUDED_VFS_PATH
