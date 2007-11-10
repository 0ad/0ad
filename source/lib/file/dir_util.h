/**
 * =========================================================================
 * File        : dir_util.h
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_DIR_UTIL
#define INCLUDED_DIR_UTIL

#include "filesystem.h"

extern bool dir_FileExists(IFilesystem* fs, const char* pathname);
extern bool dir_DirectoryExists(IFilesystem* fs, const char* dirPath);


typedef std::vector<FilesystemEntry> FilesystemEntries;

// enumerate all directory entries in <P_path>; add to container and
// then sort it by filename.
extern LibError dir_GatherSortedEntries(IFilesystem* fs, const char* dirPath, FilesystemEntries& fsEntries);


// called by dir_ForEachSortedEntry for each entry in the directory.
// return INFO::CB_CONTINUE to continue calling; anything else will cause
// dir_ForEachSortedEntry to abort and immediately return that value.
typedef LibError (*DirCallback)(const char* pathname, const FilesystemEntry& fsEntry, const uintptr_t cbData);


// call <cb> for each file and subdirectory in <dir> (alphabetical order),
// passing the entry name (not full path!), stat info, and <user>.
//
// first builds a list of entries (sorted) and remembers if an error occurred.
// if <cb> returns non-zero, abort immediately and return that; otherwise,
// return first error encountered while listing files, or 0 on success.
//
// rationale:
//   this makes dir_ForEachSortedEntry and zip_enum slightly incompatible, since zip_enum
//   returns the full path. that's necessary because VFS zip_cb
//   has no other way of determining what VFS dir a Zip file is in,
//   since zip_enum enumerates all files in the archive (not only those
//   in a given dir). no big deal though, since add_ent has to
//   special-case Zip files anyway.
//   the advantage here is simplicity, and sparing callbacks the trouble
//   of converting from/to native path (we just give 'em the dirent name).
extern LibError dir_ForEachSortedEntry(IFilesystem* fs, const char* dirPath, DirCallback cb, uintptr_t cbData);


// retrieve the next (order is unspecified) dir entry matching <filter>.
// return 0 on success, ERR::DIR_END if no matching entry was found,
// or a negative error code on failure.
// filter values:
// - 0: anything;
// - "/": any subdirectory;
// - "/|<pattern>": any subdirectory, or as below with <pattern>;
// - <pattern>: any file whose name matches; ? and * wildcards are allowed.
//
// note that the directory entries are only scanned once; after the
// end is reached (-> ERR::DIR_END returned), no further entries can
// be retrieved, even if filter changes (which shouldn't happen - see impl).
//
// rationale: we do not sort directory entries alphabetically here.
// most callers don't need it and the overhead is considerable
// (we'd have to store all entries in a vector). it is left up to
// higher-level code such as VfsUtil.
extern LibError dir_filtered_next_ent(DirectoryIterator& di, FilesystemEntry& fsEntry, const char* filter);




enum DirEnumFlags
{
	VFS_DIR_RECURSIVE = 1
};

// call <cb> for each entry matching <user_filter> (see vfs_next_dirent) in
// directory <path>; if flags & VFS_DIR_RECURSIVE, entries in
// subdirectories are also returned.
extern LibError dir_FilteredForEachEntry(IFilesystem* fs, const char* dirPath, uint enum_flags, const char* filter, DirCallback cb, uintptr_t cbData);


struct NextNumberedFilenameState
{
	int next_num;
};


// fill V_next_fn (which must be big enough for PATH_MAX chars) with
// the next numbered filename according to the pattern defined by V_fn_fmt.
// <nnfs> must be initially zeroed (e.g. by defining as static) and passed
// each time.
// if <use_vfs> (default), the paths are treated as VFS paths; otherwise,
// file.cpp's functions are used. this is necessary because one of
// our callers needs a filename for VFS archive files.
//
// this function is useful when creating new files which are not to
// overwrite the previous ones, e.g. screenshots.
// example for V_fn_fmt: "screenshots/screenshot%04d.png".
extern void dir_NextNumberedFilename(IFilesystem* fs, const char* V_fn_fmt, NextNumberedFilenameState* nnfs, char* V_next_fn);

#endif	// #ifndef INCLUDED_DIR_UTIL
