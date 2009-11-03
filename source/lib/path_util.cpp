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

#include "precompiled.h"
#include "path_util.h"

#include <cstring>
#include <cerrno>

#include "lib/wchar.h"


ERROR_ASSOCIATE(ERR::PATH_LENGTH, L"path exceeds PATH_MAX characters", ENAMETOOLONG);
ERROR_ASSOCIATE(ERR::PATH_EMPTY, L"path is an empty string", -1);
ERROR_ASSOCIATE(ERR::PATH_NOT_RELATIVE, L"path is not relative", -1);
ERROR_ASSOCIATE(ERR::PATH_NON_PORTABLE, L"path contains OS-specific dir separator", -1);
ERROR_ASSOCIATE(ERR::PATH_NON_CANONICAL, L"path contains unsupported .. or ./", -1);
ERROR_ASSOCIATE(ERR::PATH_COMPONENT_SEPARATOR, L"path component contains dir separator", -1);


bool path_is_dir_sep(wchar_t c)
{
	// note: ideally path strings would only contain '/' or even SYS_DIR_SEP.
	// however, windows-specific code (e.g. the sound driver detection)
	// uses these routines with '\\' strings. converting them all to
	// '/' and then back before passing to WinAPI would be annoying.
	// also, the self-tests verify correct operation of such strings.
	// it would be error-prone to only test the platform's separator
	// strings there. hence, we allow all separators here.
	if(c == '/' || c == '\\')
		return true;
	return false;
}

static bool path_is_dir_sepw(wchar_t c)
{
	// note: ideally path strings would only contain '/' or even SYS_DIR_SEP.
	// however, windows-specific code (e.g. the sound driver detection)
	// uses these routines with '\\' strings. converting them all to
	// '/' and then back before passing to WinAPI would be annoying.
	// also, the self-tests verify correct operation of such strings.
	// it would be error-prone to only test the platform's separator
	// strings there. hence, we allow all separators here.
	if(c == L'/' || c == L'\\')
		return true;
	return false;
}


bool path_IsDirectory(const wchar_t* path)
{
	if(path[0] == '\0')	// root dir
		return true;

	const wchar_t lastChar = path[wcslen(path)-1];
	if(path_is_dir_sep(lastChar))
		return true;

	return false;
}


// is s2 a subpath of s1, or vice versa?
// (equal counts as subpath)
bool path_is_subpath(const wchar_t* s1, const wchar_t* s2)
{
	// make sure s1 is the shorter string
	if(wcslen(s1) > wcslen(s2))
		std::swap(s1, s2);

	wchar_t c1 = 0, last_c1, c2;
	for(;;)
	{
		last_c1 = c1;
		c1 = *s1++, c2 = *s2++;

		// end of s1 reached:
		if(c1 == '\0')
		{
			// s1 matched s2 up until:
			if((c2 == '\0') ||	// its end (i.e. they're equal length) OR
				path_is_dir_sep(c2) ||		// start of next component OR
				path_is_dir_sep(last_c1))	// ", but both have a trailing slash
				// => is subpath
				return true;
		}

		// mismatch => is not subpath
		if(c1 != c2)
			return false;
	}
}


bool path_is_subpathw(const wchar_t* s1, const wchar_t* s2)
{
	// make sure s1 is the shorter string
	if(wcslen(s1) > wcslen(s2))
		std::swap(s1, s2);

	wchar_t c1 = 0, last_c1, c2;
	for(;;)
	{
		last_c1 = c1;
		c1 = *s1++, c2 = *s2++;

		// end of s1 reached:
		if(c1 == L'\0')
		{
			// s1 matched s2 up until:
			if((c2 == L'\0') ||	// its end (i.e. they're equal length) OR
				path_is_dir_sepw(c2) ||		// start of next component OR
				path_is_dir_sepw(last_c1))	// ", but both have a trailing slash
				// => is subpath
				return true;
		}

		// mismatch => is not subpath
		if(c1 != c2)
			return false;
	}
}


// if path is invalid, return a descriptive error code, otherwise INFO::OK.
LibError path_validate(const wchar_t* path)
{
	// disallow "/", because it would create a second 'root' (with name = "").
	// root dir is "".
	if(path[0] == '/')
		WARN_RETURN(ERR::PATH_NOT_RELATIVE);

	// scan each wchar_t in path string; count length.
	int c = 0;		// current wchar_t; used for .. detection
	size_t path_len = 0;
	for(;;)
	{
		const int last_c = c;
		c = path[path_len++];

		// whole path is too long
		if(path_len >= PATH_MAX)
			WARN_RETURN(ERR::PATH_LENGTH);

		// disallow:
		// - ".." (prevent going above the VFS root dir)
		// - "./" (security hole when mounting and not supported on Windows).
		// allow "/.", because CVS backup files include it.
		if(last_c == '.' && (c == '.' || c == '/'))
			WARN_RETURN(ERR::PATH_NON_CANONICAL);

		// disallow OS-specific dir separators
		if(c == '\\' || c == ':')
			WARN_RETURN(ERR::PATH_NON_PORTABLE);

		// end of string, no errors encountered
		if(c == '\0')
			break;
	}

	return INFO::OK;
}


// if name is invalid, return a descriptive error code, otherwise INFO::OK.
// (name is a path component, i.e. that between directory separators)
LibError path_component_validate(const wchar_t* name)
{
	// disallow empty strings
	if(*name == '\0')
		WARN_RETURN(ERR::PATH_EMPTY);

	for(;;)
	{
		const int c = *name++;

		// disallow *any* dir separators (regardless of which
		// platform we're on).
		if(c == '\\' || c == ':' || c == '/')
			WARN_RETURN(ERR::PATH_COMPONENT_SEPARATOR);

		// end of string, no errors encountered
		if(c == '\0')
			break;
	}

	return INFO::OK;
}


// copy path strings (provided for convenience).
void path_copy(wchar_t* dst, const wchar_t* src)
{
	wcscpy_s(dst, PATH_MAX, src);
}


// combine <path1> and <path2> into one path, and write to <dst>.
// if necessary, a directory separator is added between the paths.
// each may be empty, filenames, or full paths.
// total path length (including '\0') must not exceed PATH_MAX.
void path_append(wchar_t* dst, const wchar_t* path1, const wchar_t* path2, size_t flags)
{
	const size_t len1 = wcslen(path1);
	const size_t len2 = wcslen(path2);
	size_t total_len = len1 + len2 + 1;	// includes '\0'
	const bool no_end_slash1 = (len1 == 0 || !path_is_dir_sep(path1[len1-1]));
	const bool no_end_slash2 = (len2 == 0 || !path_is_dir_sep(path2[len2-1]));

	// check if we need to add '/' between path1 and path2
	// notes:
	// - the second can't start with '/' (not allowed by path_validate)
	// - must check len2 as well - if it's empty, we'd end up
	//   inadvertently terminating the string with '/'.
	bool need_separator = false;
	if(len2 != 0 && len1 != 0 && no_end_slash1)
	{
		total_len++;	// for '/'
		need_separator = true;
	}

	// check if trailing slash requested and not already present
	bool need_terminator = false;
	if(flags & PATH_APPEND_SLASH && no_end_slash2)
	{
		total_len++;	// for '/'
		need_terminator = true;
	}

	if(total_len > PATH_MAX)
	{
		DEBUG_WARN_ERR(ERR::PATH_LENGTH);
		*dst = 0;
		return;
	}

	SAFE_WCSCPY(dst, path1);
	dst += len1;
	if(need_separator)
		*dst++ = '/';
	SAFE_WCSCPY(dst, path2);
	if(need_terminator)
		SAFE_WCSCPY(dst+len2, L"/");
}


//-----------------------------------------------------------------------------

// return pointer to the name component within path (i.e. skips over all
// characters up to the last dir separator, if any).
const wchar_t* path_name_only(const wchar_t* path)
{
	const wchar_t* slash1 = wcsrchr(path, '/');
	const wchar_t* slash2 = wcsrchr(path, '\\');
	// neither present, it's a filename only
	if(!slash1 && !slash2)
		return path;

	// return name, i.e. component after the last slash
	const wchar_t* name = std::max(slash1, slash2)+1;
	if(name[0] != '\0')	// else path_component_validate would complain
		path_component_validate(name);
	return name;
}

// if <path> contains a name component, it is stripped away.
void path_strip_fn(wchar_t* path)
{
	wchar_t* name = (wchar_t*)path_name_only(path);
	*name = '\0';	// cut off string here
	debug_assert(path_IsDirectory(path));
}


// return extension of <fn>, or "" if there is none.
// NOTE: does not include the period; e.g. "a.bmp" yields "bmp".
const wchar_t* path_extension(const wchar_t* fn)
{
	const wchar_t* dot = wcsrchr(fn, '.');
	if(!dot)
		return L"";
	const wchar_t* ext = dot+1;
	return ext;
}


fs::wpath wpath_from_path(const fs::path& pathname)
{
	return wstring_from_string(pathname.string());
}

fs::path path_from_wpath(const fs::wpath& pathname)
{
	return string_from_wstring(pathname.string());
}
