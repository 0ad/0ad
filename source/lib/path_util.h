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
//   when reading, both '/' and SYS_DIR_SEP are accepted; '/' is written.

#ifndef PATH_UTIL_H__
#define PATH_UTIL_H__

#include "posix/posix_types.h"	// PATH_MAX


namespace ERR
{
	const LibError PATH_LENGTH              = -100300;
	const LibError PATH_EMPTY               = -100301;
	const LibError PATH_NOT_RELATIVE        = -100302;
	const LibError PATH_NON_PORTABLE        = -100303;
	const LibError PATH_NON_CANONICAL       = -100304;
	const LibError PATH_COMPONENT_SEPARATOR = -100305;
}


/**
 * check if path is valid. (see source for criteria)
 *
 * @return LibError (ERR::PATH_* or INFO::OK)
 **/
extern LibError path_validate(const char* path);

/**
 * return appropriate code if path is invalid, otherwise continue.
 **/
#define CHECK_PATH(path) RETURN_ERR(path_validate(path))

/**
 * check if name is valid. (see source for criteria)
 *
 * @return LibError (ERR::PATH_* or INFO::OK)
 **/
extern LibError path_component_validate(const char* name);


/**
 * is s2 a subpath of s1, or vice versa? (equal counts as subpath)
 *
 * @param s1, s2 comparand strings
 * @return bool
 **/
extern bool path_is_subpath(const char* s1, const char* s2);


/**
 * copy path strings (provided for convenience).
 *
 * @param dst destination; must be at least as large as source buffer,
 * and should hold PATH_MAX chars.
 * @param src source; should not exceed PATH_MAX chars
 **/
extern void path_copy(char* dst, const char* src);

/**
 * flags controlling path_append behavior
 **/
enum PathAppendFlags
{
	/**
	 * make sure <dst> ends up with a trailing slash. this is useful for
	 * VFS directory paths, which have that requirement.
	 **/
	PATH_APPEND_SLASH = 1
};

/**
 * append one path onto another, adding directory separator if necessary.
 *
 * @param dst destination into which combined path is written;
 * must hold at least PATH_MAX chars.
 * @param path1, path2 strings: empty, filenames, or full paths.
 * total resulting string must not exceed PATH_MAX chars.
 * @param flags see PathAppendFlags.
 * @return LibError
 **/
extern LibError path_append(char* dst, const char* path1, const char* path2,
	uint flags = 0);

/**
 * at the start of a path, replace the given substring with another.
 *
 * @param dst destination; must hold at least PATH_MAX chars.
 * @param src source string.
 * @param remove substring to remove; must match (case-sensitive) the
 * start of src.
 * @param replace string to prepend to output after stripping <remove>.
 * @return LibError; ERR::FAIL (without warning!) if <src> doesn't
 * match <remove>.
 **/
extern LibError path_replace(char* dst, const char* src, const char* remove, const char* replace);


/**
 * get the name component of a path.
 *
 * skips over all characters up to the last dir separator, if any.
 * @param path input path.
 * @return pointer to name component within <path>.
 **/
extern const char* path_name_only(const char* path);

/**
 * get the last component of a path.
 *
 * this is similar to path_name_only, but correctly handles VFS paths,
 * which must end with '/'. (path_name_only would return "")
 * @param path input path.
 * @return pointer to last component within <path>.
 **/
extern const char* path_last_component(const char* path);

/**
 * strip away the name component in a path.
 *
 * @param path input and output; chopped by inserting '\0'.
 **/

extern void path_strip_fn(char* path);

/**
 * retrieve only the directory path portion of a path.
 *
 * @param path source path.
 * @param dir output directory path ("" if root dir,
 * otherwise it ends with '/').
 * note: implementation via path_copy and path_strip_fn.
 **/
extern void path_dir_only(const char* path, char* dir);

/**
 * get filename's extension.
 *
 * @return pointer to extension within <fn>, or "" if there is none.
 * NOTE: does not include the period; e.g. "a.bmp" yields "bmp".
 **/
extern const char* path_extension(const char* fn);


/**
 * callback for each component in a path string.
 *
 * if path is empty (i.e. ""), this is not called.
 *
 * @param component: 0-terminated name of the component (does not
 * include any trailing slash!)
 * @param is_dir indicates if it's a directory (i.e. <component> is
 * followed by a slash in the original path).
 * rationale: a bool isn't as nice as a flag or enum, but vfs_tree already
 *   has TNodeType and we don't want to expose that or create a new one.
 * @param ctx: context parameter that was passed to path_foreach_component.
 * @return LibError; INFO::CB_CONTINUE to continue operation normally;
 * anything else will cause path_foreach_component to abort immediately and
 * return that. no need to 'abort' (e.g. return INFO::OK) after a filename is
 * encountered - that's taken care of automatically.
 **/
typedef LibError (*PathComponentCb)(const char* component, bool is_dir, void* ctx);

/**
 * call <cb> with <ctx> for each component in <path>.
 * @return LibError
 **/
extern LibError path_foreach_component(const char* path, PathComponentCb cb, void* ctx);


//-----------------------------------------------------------------------------

/**
 * convenience "class" that simplifies successively appending a filename to
 * its parent directory. this avoids needing to allocate memory and calling
 * strlen/strcat. used by wdll_ver and dir_next_ent.
 * we want to maintain C compatibility, so this isn't a C++ class.
 **/
struct PathPackage
{
	char* end;
	size_t chars_left;
	char path[PATH_MAX];
};

/**
 * write the given directory path into our buffer and set end/chars_left
 * accordingly. <dir> need not but can end with a directory separator.
 * 
 * note: <dir> and the filename set via path_package_append_file are separated by
 * '/'. this is to allow use on portable paths; the function otherwise
 * does not care if paths are relative/portable/absolute.
 * @return LibError
 **/
extern LibError path_package_set_dir(PathPackage* pp, const char* dir);

/**
 * append the given filename to the directory established by the last
 * path_package_set_dir on this package. the whole path is accessible at pp->path.
 * @return LibError
 **/
extern LibError path_package_append_file(PathPackage* pp, const char* path);

#endif	// #ifndef PATH_UTIL_H__
