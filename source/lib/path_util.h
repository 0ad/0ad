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

template<class Path_t>
static inline bool IsDirectory(const Path_t& pathname)
{
	if(pathname.empty())	// (ensure length()-1 is safe)
		return true;	// (the VFS root directory is represented as an empty string)

	// note: ideally, path strings would only contain '/' or even SYS_DIR_SEP.
	// however, windows-specific code (e.g. the sound driver detection)
	// uses these routines with '\\' strings. converting them all to
	// '/' and then back before passing to OS functions would be annoying.
	// also, the self-tests verify correct operation of such strings.
	// it would be error-prone to only test the platform's separator
	// strings there. hence, we allow all separators here.
	return pathname[pathname.length()-1] == '/' || pathname[pathname.length()-1] == '\\';
}

template<class Path_t>
static inline Path_t Path(const Path_t& pathname)
{
	size_t n = pathname.find_last_of('/');
	if(n == Path_t::npos)
	{
		n = pathname.find_last_of('\\');
		if(n == Path_t::npos)
			return L"";
	}
	return pathname.substr(0, n);
}

template<class Path_t>
static inline Path_t Filename(const Path_t& pathname)
{
	size_t n = pathname.find_last_of('/');
	if(n == Path_t::npos)
	{
		n = pathname.find_last_of('\\');
		if(n == Path_t::npos)
			return pathname;
	}
	return pathname.substr(n+1);
}

template<class Path_t>
static inline Path_t Basename(const Path_t& pathname)
{
	const Path_t filename = Filename(pathname);
	const size_t idxDot = filename.find_last_of('.');
	if(idxDot == Path_t::npos)
		return filename;
	return filename.substr(0, idxDot);
}

template<class Path_t>
static inline std::wstring Extension(const Path_t& pathname)
{
	const size_t idxDot = pathname.find_last_of('.');
	if(idxDot == Path_t::npos)
		return Path_t();
	return pathname.substr(idxDot);
}

static inline std::wstring JoinPathStrings(const std::wstring& path1, const std::wstring& path2)
{
	std::wstring ret = path1;
	if(!IsDirectory(path1))
		ret += '/';
	ret += path2;
	return ret;
}

template<class Path_t>
static inline Path_t Join(const Path_t& path1, const Path_t& path2)
{
	return JoinPathStrings(path1, path2);
}

template<class Path_t>
static inline Path_t Join(const Path_t& path1, const char* literal)
{
	return JoinPathStrings(path1, NativePathFromString(literal));
}

template<class Path_t>
static inline Path_t Join(const char* literal, const Path_t& path2)
{
	return JoinPathStrings(NativePathFromString(literal), path2);
}

template<class Path_t>
static inline Path_t AddSlash(const Path_t& path)
{
	return IsDirectory(path)? path : path+L'/';
}

template<class Path_t>
static inline Path_t ChangeExtension(const Path_t& pathname, const std::wstring& extension)
{
	return Join(Path(pathname), Basename(pathname)+extension);
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
