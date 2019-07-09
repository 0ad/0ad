/* Copyright (C) 2019 Wildfire Games.
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
 * Path string class, similar to boost::filesystem::basic_path.
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

#ifndef INCLUDED_PATH
#define INCLUDED_PATH

#if CONFIG_ENABLE_BOOST
# include "boost/functional/hash.hpp"
#endif

#include "lib/utf8.h"

#include <cstring>

namespace ERR
{
	const Status PATH_CHARACTER_ILLEGAL   = -100300;
	const Status PATH_CHARACTER_UNSAFE    = -100301;
	const Status PATH_NOT_FOUND           = -100302;
	const Status PATH_MIXED_SEPARATORS    = -100303;
}

/**
 * is s2 a subpath of s1, or vice versa? (equal counts as subpath)
 *
 * @param s1, s2 comparand strings
 * @return bool
 **/
LIB_API bool path_is_subpath(const wchar_t* s1, const wchar_t* s2);

/**
 * Get the path component of a path.
 * Skips over all characters up to the last dir separator, if any.
 *
 * @param path Input path.
 * @return pointer to path component within \<path\>.
 **/
LIB_API const wchar_t* path_name_only(const wchar_t* path);


// NB: there is a need for 'generic' paths (e.g. for Trace entry / archive pathnames).
// converting between specialized variants via c_str would be inefficient, and the
// Os/VfsPath typedefs are hopefully sufficient to avoid errors.
class Path
{
public:
	typedef std::wstring String;

	Path()
	{
		DetectSeparator();
	}

	Path(const Path& p)
		: path(p.path)
	{
		DetectSeparator();
	}

	Path(const char* p)
		: path((const unsigned char*)p, (const unsigned char*)p+strlen(p))
		// interpret bytes as unsigned; makes no difference for ASCII,
		// and ensures OsPath on Unix will only contain values 0 <= c < 0x100
	{
		DetectSeparator();
	}

	Path(const wchar_t* p)
		: path(p, p+wcslen(p))
	{
		DetectSeparator();
	}

	Path(const std::string& s)
		: path((const unsigned char*)s.c_str(), (const unsigned char*)s.c_str()+s.length())
	{
		DetectSeparator();
	}

	Path(const std::wstring& s)
		: path(s)
	{
		DetectSeparator();
	}

	Path& operator=(const Path& rhs)
	{
		path = rhs.path;
		DetectSeparator();	// (warns if separators differ)
		return *this;
	}

	bool empty() const
	{
		return path.empty();
	}

	const String& string() const
	{
		return path;
	}

	/**
	 * Return a UTF-8 version of the path, in a human-readable but potentially
	 * lossy form. It is *not* safe to take this string and construct a new
	 * Path object from it (it may fail for some non-ASCII paths) - it should
	 * only be used for displaying paths to users.
	 */
	std::string string8() const
	{
		Status err;
#if !OS_WIN
		// On Unix, assume paths consisting of 8-bit charactes saved in this wide string.
		std::string spath(path.begin(), path.end());

		// Return it if it's valid UTF-8
		wstring_from_utf8(spath, &err);
		if(err == INFO::OK)
			return spath;

		// Otherwise assume ISO-8859-1 and let utf8_from_wstring treat each character as a Unicode code point.
#endif
		// On Windows, paths are UTF-16 strings. We don't support non-BMP characters so we can assume it's simply a wstring.
		return utf8_from_wstring(path, &err);
	}

	bool operator<(const Path& rhs) const
	{
		return path < rhs.path;
	}

	bool operator==(const Path& rhs) const
	{
		return path == rhs.path;
	}

	bool operator!=(const Path& rhs) const
	{
		return !operator==(rhs);
	}

	bool IsDirectory() const
	{
		if(empty())	// (ensure length()-1 is safe)
			return true;	// (the VFS root directory is represented as an empty string)
		return path[path.length()-1] == separator;
	}

	Path Parent() const
	{
		const size_t idxSlash = path.find_last_of(separator);
		if(idxSlash == String::npos)
			return L"";
		return path.substr(0, idxSlash);
	}

	Path Filename() const
	{
		const size_t idxSlash = path.find_last_of(separator);
		if(idxSlash == String::npos)
			return path;
		return path.substr(idxSlash+1);
	}

	Path Basename() const
	{
		const Path filename = Filename();
		const size_t idxDot = filename.string().find_last_of('.');
		if(idxDot == String::npos)
			return filename;
		return filename.string().substr(0, idxDot);
	}

	// (Path return type allows callers to use our operator==)
	Path Extension() const
	{
		const Path filename = Filename();
		const size_t idxDot = filename.string().find_last_of('.');
		if(idxDot == String::npos)
			return Path();
		return filename.string().substr(idxDot);
	}

	Path ChangeExtension(Path extension) const
	{
		return Parent() / Path(Basename().string() + extension.string());
	}

	Path operator/(Path rhs) const
	{
		Path ret = *this;
		if(ret.path.empty())	// (empty paths assume '/')
			ret.separator = rhs.separator;
		if(!ret.IsDirectory())
			ret.path += ret.separator;

		if(rhs.path.find((ret.separator == '/')? '\\' : '/') != String::npos)
		{
			PrintToDebugOutput();
			rhs.PrintToDebugOutput();
			DEBUG_WARN_ERR(ERR::PATH_MIXED_SEPARATORS);
		}
		ret.path += rhs.path;
		return ret;
	}

	/**
	 * Return the path before the common part of both paths
	 * @param other Indicates the start of the path which should be removed
	 * @note other should be a VfsPath, while this should be an OsPath
	 */
	Path BeforeCommon(Path other) const
	{
		Path ret = *this;
		if(ret.empty() || other.empty())
			return L"";

		// Convert the separator to allow for string comparison
		if(other.separator != ret.separator)
			replace(other.path.begin(), other.path.end(), other.separator, ret.separator);

		const size_t idx = ret.path.rfind(other.path);
		if(idx == String::npos)
			return L"";

		return path.substr(0, idx);
	}

	static Status Validate(String::value_type c);

private:
	void PrintToDebugOutput() const
	{
		debug_printf("Path %s, separator %c\n", string8().c_str(), (char)separator);
	}

	void DetectSeparator()
	{
		const size_t idxBackslash = path.find('\\');

		if(path.find('/') != String::npos && idxBackslash != String::npos)
		{
			PrintToDebugOutput();
			DEBUG_WARN_ERR(ERR::PATH_MIXED_SEPARATORS);
		}

		// (default to '/' for empty strings)
		separator = (idxBackslash == String::npos)? '/' : '\\';
	}

	String path;

	// note: ideally, path strings would only contain '/' or even SYS_DIR_SEP.
	// however, Windows-specific code (e.g. the sound driver detection)
	// uses these routines with '\\' strings. the boost::filesystem approach of
	// converting them all to '/' and then back via external_file_string is
	// annoying and inefficient. we allow either type of separators,
	// appending whichever was first encountered. when modifying the path,
	// we ensure the same separator is used.
	wchar_t separator;
};

static inline std::wostream& operator<<(std::wostream& s, const Path& path)
{
	s << path.string();
	return s;
}

static inline std::wistream& operator>>(std::wistream& s, Path& path)
{
	Path::String string;
	s >> string;
	path = Path(string);
	return s;
}

#if CONFIG_ENABLE_BOOST

namespace boost {

template<>
struct hash<Path> : std::unary_function<Path, std::size_t>
{
	std::size_t operator()(const Path& path) const
	{
		return hash_value(path.string());
	}
};

}

#endif	// #if CONFIG_ENABLE_BOOST

#endif	// #ifndef INCLUDED_PATH
