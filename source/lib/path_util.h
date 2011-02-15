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


template<class Path>
Path AddSlash(const Path& path)
{
	return (path.leaf() == L".")? path : path/L"/";
}

LIB_API fs::wpath wpath_from_path(const fs::path& pathname);
LIB_API fs::path path_from_wpath(const fs::wpath& pathname);


static inline std::wstring Path(const std::wstring& pathname)
{
	size_t n = pathname.find_last_of('/');
	if(n == std::wstring::npos)
	{
		n = pathname.find_last_of('\\');
		if(n == std::wstring::npos)
			return L"";
	}
	return pathname.substr(0, n);
}

static inline std::wstring Filename(const std::wstring& pathname)
{
	size_t n = pathname.find_last_of('/');
	if(n == std::wstring::npos)
	{
		n = pathname.find_last_of('\\');
		if(n == std::wstring::npos)
			return pathname;
	}
	return pathname.substr(n+1);
}


static inline std::wstring Basename(const std::wstring& filename)
{
	const size_t n = filename.find_last_of('.');
	if(n == std::wstring::npos)
		return filename;
	return filename.substr(0, n);
}

static inline std::wstring Extension(const std::wstring& filename)
{
	const size_t n = filename.find_last_of('.');
	if(n == std::wstring::npos)
		return std::wstring();
	return filename.substr(n);
}

static inline std::wstring Join(const std::wstring& path1, const std::wstring& path2)
{
	std::wstring ret = path1;
	if(!path1.empty() && path1[path1.length()-1] != '/' && path1[path1.length()-1] != '\\')
		ret += '/';
	ret += path2;
	return ret;
}

static inline std::wstring ChangeExtension(const std::wstring& pathname, const std::wstring& extension)
{
	return Join(Path(pathname), Basename(Filename(pathname))+extension);
}

#endif	// #ifndef INCLUDED_PATH_UTIL
