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
 * helper functions for path strings.
 */

// notes:
// - this module is independent of lib/file so that it can be used from
//   other code without pulling in the entire file manager.
// - there is no restriction on buffer lengths except the underlying OS.
//   input buffers must not exceed PATH_MAX chars, while outputs
//   must hold at least that much.
// - unless otherwise mentioned, all functions are intended to work with
//   native and VFS paths.
//   when reading, both '/' and SYS_DIR_SEP are accepted; '/' is written.

#ifndef INCLUDED_PATH_UTIL
#define INCLUDED_PATH_UTIL

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
LIB_API LibError path_validate(const wchar_t* path);

/**
 * return appropriate code if path is invalid, otherwise continue.
 **/
#define CHECK_PATH(path) RETURN_ERR(path_validate(path))

/**
 * check if name is valid. (see source for criteria)
 *
 * @return LibError (ERR::PATH_* or INFO::OK)
 **/
LIB_API LibError path_component_validate(const wchar_t* name);

/**
 * is the given character a path separator character?
 *
 * @param c character to test
 * @return bool
 **/
LIB_API bool path_is_dir_sep(wchar_t c);

/**
 * is the given path(name) a directory?
 *
 * @return bool
 **/
LIB_API bool path_IsDirectory(const wchar_t* path);

/**
 * is s2 a subpath of s1, or vice versa? (equal counts as subpath)
 *
 * @param s1, s2 comparand strings
 * @return bool
 **/
LIB_API bool path_is_subpath(const wchar_t* s1, const wchar_t* s2);
LIB_API bool path_is_subpathw(const wchar_t* s1, const wchar_t* s2);


/**
 * copy path strings (provided for convenience).
 *
 * @param dst destination; must be at least as large as source buffer,
 * and should hold PATH_MAX chars.
 * @param src source; should not exceed PATH_MAX chars
 **/
LIB_API void path_copy(wchar_t* dst, const wchar_t* src);

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
 **/
LIB_API void path_append(wchar_t* dst, const wchar_t* path1, const wchar_t* path2, size_t flags = 0);

/**
 * get the name component of a path.
 *
 * skips over all characters up to the last dir separator, if any.
 * @param path input path.
 * @return pointer to name component within <path>.
 **/
LIB_API const wchar_t* path_name_only(const wchar_t* path);

/**
 * strip away the name component in a path.
 *
 * @param path input and output; chopped by inserting '\0'.
 **/
LIB_API void path_strip_fn(wchar_t* path);	// deprecated in favor of fs::branch_path

/**
 * get filename's extension.
 *
 * @return pointer to extension within <fn>, or "" if there is none.
 * NOTE: does not include the period; e.g. "a.bmp" yields "bmp".
 **/
LIB_API const wchar_t* path_extension(const wchar_t* fn);	// deprecated in favor of fs::extension


LIB_API fs::wpath wpath_from_path(const fs::path& pathname);
LIB_API fs::path path_from_wpath(const fs::wpath& pathname);

#endif	// #ifndef INCLUDED_PATH_UTIL
