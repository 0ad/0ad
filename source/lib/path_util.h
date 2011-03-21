/* Copyright (c) 2010 Wildfire Games
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

#include "lib/native_path.h"
#include "lib/posix/posix_filesystem.h"

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
 * Get the name component of a path.
 * Skips over all characters up to the last dir separator, if any.
 *
 * @param path Input path.
 * @return pointer to name component within \<path\>.
 **/
LIB_API const wchar_t* path_name_only(const wchar_t* path);


namespace Path {

static inline NativePath Path(const NativePath& pathname)
{
	size_t n = pathname.find_last_of('/');
	if(n == NativePath::npos)
	{
		n = pathname.find_last_of('\\');
		if(n == NativePath::npos)
			return L"";
	}
	return pathname.substr(0, n);
}

static inline NativePath Filename(const NativePath& pathname)
{
	size_t n = pathname.find_last_of('/');
	if(n == NativePath::npos)
	{
		n = pathname.find_last_of('\\');
		if(n == NativePath::npos)
			return pathname;
	}
	return pathname.substr(n+1);
}

static inline NativePath Basename(const NativePath& pathname)
{
	const NativePath filename = Filename(pathname);
	const size_t idxDot = filename.find_last_of('.');
	if(idxDot == NativePath::npos)
		return filename;
	return filename.substr(0, idxDot);
}

static inline NativePath Extension(const NativePath& pathname)
{
	const size_t idxDot = pathname.find_last_of('.');
	if(idxDot == NativePath::npos)
		return NativePath();
	return pathname.substr(idxDot);
}

static inline NativePath Join(const NativePath& path1, const NativePath& path2)
{
	NativePath ret = path1;
	if(!path1.empty() && path1[path1.length()-1] != '/' && path1[path1.length()-1] != '\\')
		ret += '/';
	ret += path2;
	return ret;
}

static inline NativePath Join(const NativePath& path1, const NativePath& path2, const NativePath& path3)
{
	return Join(Join(path1, path2), path3);
}

static inline NativePath AddSlash(const NativePath& path)
{
	return (!path.empty() && path_is_dir_sep(path[path.length()-1]))? path : path+L'/';
}

static inline NativePath ChangeExtension(const NativePath& pathname, const NativePath& extension)
{
	return Join(Path(pathname), Basename(Filename(pathname))+extension);
}

}	// namespace Path

static inline bool FileExists(const NativePath& pathname)
{
	struct stat s;
	const bool exists = wstat(pathname.c_str(), &s) == 0;
	return exists;
}

static inline u64 FileSize(const NativePath& pathname)
{
	struct stat s;
	debug_assert(wstat(pathname.c_str(), &s) == 0);
	return s.st_size;
}

static inline bool DirectoryExists(const NativePath& path)
{
	WDIR* dir = wopendir(path.c_str());
	if(dir)
	{
		wclosedir(dir);
		return true;
	}
	return false;
}

#endif	// #ifndef INCLUDED_PATH_UTIL
