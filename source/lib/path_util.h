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

#include "lib/external_libraries/boost_filesystem.h"

namespace ERR
{
	const LibError PATH_EMPTY               = -100300;
	const LibError PATH_COMPONENT_SEPARATOR = -100301;
	const LibError PATH_NOT_FOUND           = -100302;
}

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
 * is s2 a subpath of s1, or vice versa? (equal counts as subpath)
 *
 * @param s1, s2 comparand strings
 * @return bool
 **/
LIB_API bool path_is_subpath(const wchar_t* s1, const wchar_t* s2);

/**
 * get the name component of a path.
 *
 * skips over all characters up to the last dir separator, if any.
 * @param path input path.
 * @return pointer to name component within <path>.
 **/
LIB_API const wchar_t* path_name_only(const wchar_t* path);


template<class Path>
Path AddSlash(const Path& path)
{
	return (path.leaf() == L".")? path : path/L"/";
}

LIB_API fs::wpath wpath_from_path(const fs::path& pathname);
LIB_API fs::path path_from_wpath(const fs::wpath& pathname);

#endif	// #ifndef INCLUDED_PATH_UTIL
