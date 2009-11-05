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


ERROR_ASSOCIATE(ERR::PATH_EMPTY, L"path is an empty string", -1);
ERROR_ASSOCIATE(ERR::PATH_COMPONENT_SEPARATOR, L"path component contains dir separator", -1);
ERROR_ASSOCIATE(ERR::PATH_NOT_FOUND, L"path not found", -1);


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


fs::wpath wpath_from_path(const fs::path& pathname)
{
	return wstring_from_string(pathname.string());
}

fs::path path_from_wpath(const fs::wpath& pathname)
{
	return string_from_wstring(pathname.string());
}
