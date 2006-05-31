/**
 * =========================================================================
 * File        : path_util.h
 * Project     : 0 A.D.
 * Description : helper functions for path strings.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2006 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

// notes:
// - this module is split out of lib/res/file so that it can be used from
//   other code without pulling in the entire file manager.
// - there is no restriction on buffer lengths except the underlying OS.
//   input buffers must not exceed PATH_MAX chars, while outputs
//   must hold at least that much.
// - unless otherwise mentioned, all functions are intended to work with
//   native and portable and VFS paths.
//   when reading, both '/' and DIR_SEP are accepted; '/' is written.

#ifndef PATH_UTIL_H__
#define PATH_UTIL_H__

#include "posix.h"	// PATH_MAX

// if path is invalid (see source for criteria), return a
// descriptive error code, otherwise ERR_OK.
extern LibError path_validate(const char* path);
#define CHECK_PATH(path) RETURN_ERR(path_validate(path))

// if name is invalid, (see source for criteria), return a
// descriptive error code, otherwise ERR_OK.
extern LibError path_component_validate(const char* name);


// is s2 a subpath of s1, or vice versa?
// (equal counts as subpath)
extern bool path_is_subpath(const char* s1, const char* s2);

// if path is invalid, return a descriptive error code, otherwise ERR_OK.
extern LibError path_validate(const char* path);

// if name is invalid, return a descriptive error code, otherwise ERR_OK.
// (name is a path component, i.e. that between directory separators)
extern LibError path_component_validate(const char* name);

// copy path strings (provided for convenience).
extern void path_copy(char* dst, const char* src);

enum PathAppendFlags
{
	// make sure <dst> ends up with a trailing slash. this is useful for
	// VFS directory paths, which have that requirement.
	PATH_APPEND_SLASH = 1
};

// combine <path1> and <path2> into one path, and write to <dst>.
// if necessary, a directory separator is added between the paths.
// each may be empty, filenames, or full paths.
// total path length (including '\0') must not exceed PATH_MAX.
extern LibError path_append(char* dst, const char* path1, const char* path2,
	uint flags = 0);

// strip <remove> from the start of <src>, prepend <replace>,
// and write to <dst>.
// returns ERR_FAIL (without warning!) if the beginning of <src> doesn't
// match <remove>.
extern LibError path_replace(char* dst, const char* src, const char* remove, const char* replace);


// return pointer to the name component within path (i.e. skips over all
// characters up to the last dir separator, if any).
extern const char* path_name_only(const char* path);

// return last component within path. this is similar to path_name_only,
// but correctly handles VFS paths, which must end with '/'.
// (path_name_only would return "")
extern const char* path_last_component(const char* path);

// if <path> contains a name component, it is stripped away.
extern void path_strip_fn(char* path);

// fill <dir> with the directory path portion of <path>
// ("" if root dir, otherwise ending with '/').
// note: copies to <dir> and proceeds to path_strip_fn it.
extern void path_dir_only(const char* path, char* dir);

// return extension of <fn>, or "" if there is none.
// NOTE: does not include the period; e.g. "a.bmp" yields "bmp".
extern const char* path_extension(const char* fn);


// called for each component in a path string, indicating if it's
// a directory (i.e. <component> is followed by a slash in the original
// path).
// if path is empty (i.e. ""), this is not called.
//
// component: 0-terminated name of the component (does not include any
// trailing slash!)
// ctx: context parameter that was passed to path_foreach_component.
// return: INFO_CB_CONTINUE to continue operation normally; anything else
// will cause path_foreach_component to abort immediately and return that.
// no need to 'abort' (e.g. return ERR_OK) after a filename is encountered -
// that's taken care of automatically.
//
// rationale:
// - we indicate if it's a directory via bool. this isn't as nice as a
//   flag or enum, but vfs_tree already has TNodeType and we don't want
//   to expose that or create a new one.
typedef LibError (*PathComponentCb)(const char* component, bool is_dir, void* ctx);

// call <cb> with <ctx> for each component in <path>.
extern LibError path_foreach_component(const char* path, PathComponentCb cb, void* ctx);


//-----------------------------------------------------------------------------

// convenience "class" that simplifies successively appending a filename to
// its parent directory. this avoids needing to allocate memory and calling
// strlen/strcat. used by wdll_ver and dir_next_ent.
// we want to maintain C compatibility, so this isn't a C++ class.

struct PathPackage
{
	char* end;
	size_t chars_left;
	char path[PATH_MAX];
};

// write the given directory path into our buffer and set end/chars_left
// accordingly. <dir> need not but can end with a directory separator.
//
// note: <dir> and the filename set via path_package_append_file are separated by
// '/'. this is to allow use on portable paths; the function otherwise
// does not care if paths are relative/portable/absolute.
extern LibError path_package_set_dir(PathPackage* pp, const char* dir);

// append the given filename to the directory established by the last
// path_package_set_dir on this package. the whole path is accessible at pp->path.
extern LibError path_package_append_file(PathPackage* pp, const char* path);

#endif	// #ifndef PATH_UTIL_H__
