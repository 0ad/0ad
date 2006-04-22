/**
 * =========================================================================
 * File        : path_util.cpp
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

#include "precompiled.h"

#include <string.h>

#include "lib.h"
#include "posix.h"
#include "path_util.h"


static inline bool is_dir_sep(char c)
{
	if(c == '/' || c == DIR_SEP)
		return true;
	return false;
}


// is s2 a subpath of s1, or vice versa?
bool path_is_subpath(const char* s1, const char* s2)
{
	// make sure s1 is the shorter string
	if(strlen(s1) > strlen(s2))
		std::swap(s1, s2);

	int c1 = 0, last_c1, c2;
	for(;;)
	{
		last_c1 = c1;
		c1 = *s1++, c2 = *s2++;

		// end of s1 reached:
		if(c1 == '\0')
		{
			// s1 matched s2 up until:
			if((c2 == '\0') ||	// its end (i.e. they're equal length) OR
				is_dir_sep(c2) ||		// start of next component OR
				is_dir_sep(last_c1))	// ", but both have a trailing slash
				// => is subpath
				return true;
		}

		// mismatch => is not subpath
		if(c1 != c2)
			return false;
	}
}


// if path is invalid, return a descriptive error code, otherwise ERR_OK.
LibError path_validate(const char* path)
{
	// disallow "/", because it would create a second 'root' (with name = "").
	// root dir is "".
	if(path[0] == '/')
		return ERR_PATH_NOT_RELATIVE;

	// scan each char in path string; count length.
	int c = 0;		// current char; used for .. detection
	size_t path_len = 0;
	for(;;)
	{
		const int last_c = c;
		c = path[path_len++];

		// whole path is too long
		if(path_len >= PATH_MAX)
			return ERR_PATH_LENGTH;

		// disallow:
		// - ".." (prevent going above the VFS root dir)
		// - "./" (security hole when mounting and not supported on Windows).
		// allow "/.", because CVS backup files include it.
		if(last_c == '.' && (c == '.' || c == '/'))
			return ERR_PATH_NON_CANONICAL;

		// disallow OS-specific dir separators
		if(c == '\\' || c == ':')
			return ERR_PATH_NON_PORTABLE;

		// end of string, no errors encountered
		if(c == '\0')
			break;
	}

	return ERR_OK;
}


// if name is invalid, return a descriptive error code, otherwise ERR_OK.
// (name is a path component, i.e. that between directory separators)
LibError path_component_validate(const char* name)
{
	// disallow empty strings
	if(*name == '\0')
		return ERR_PATH_EMPTY;

	for(;;)
	{
		const int c = *name++;

		// disallow *any* dir separators (regardless of which
		// platform we're on).
		if(c == '\\' || c == ':' || c == '/')
			return ERR_PATH_COMPONENT_SEPARATOR;

		// end of string, no errors encountered
		if(c == '\0')
			break;
	}

	return ERR_OK;
}


// copy path strings (provided for convenience).
void path_copy(char* dst, const char* src)
{
	strcpy_s(dst, PATH_MAX, src);
}


// combine <path1> and <path2> into one path, and write to <dst>.
// if necessary, a directory separator is added between the paths.
// each may be empty, filenames, or full paths.
// total path length (including '\0') must not exceed PATH_MAX.
LibError path_append(char* dst, const char* path1, const char* path2)
{
	const size_t len1 = strlen(path1);
	const size_t len2 = strlen(path2);
	size_t total_len = len1 + len2 + 1;	// includes '\0'

	// check if we need to add '/' between path1 and path2
	// note: the second can't start with '/' (not allowed by path_validate)
	bool need_separator = false;
	if(len1 != 0 && !is_dir_sep(path1[len1-1]))
	{
		total_len++;	// for '/'
		need_separator = true;
	}

	if(total_len+1 > PATH_MAX)
		WARN_RETURN(ERR_PATH_LENGTH);

	strcpy(dst, path1);	// safe
	dst += len1;
	if(need_separator)
		*dst++ = '/';
	strcpy(dst, path2);	// safe
	return ERR_OK;
}


// strip <remove> from the start of <src>, prepend <replace>,
// and write to <dst>.
// returns ERR_FAIL if the beginning of <src> doesn't match <remove>.
LibError path_replace(char* dst, const char* src, const char* remove, const char* replace)
{
	// remove doesn't match start of <src>
	const size_t remove_len = strlen(remove);
	if(strncmp(src, remove, remove_len) != 0)
		WARN_RETURN(ERR_FAIL);

	// get rid of trailing / in src (must not be included in remove)
	const char* start = src+remove_len;
	if(is_dir_sep(*start))
		start++;

	// prepend replace.
	RETURN_ERR(path_append(dst, replace, start));
	return ERR_OK;
}


//-----------------------------------------------------------------------------

// split paths into specific parts

// return pointer to the name component within path (i.e. skips over all
// characters up to the last dir separator, if any).
const char* path_name_only(const char* path)
{
	// first try: look for portable '/'
	const char* slash = strrchr(path, '/');
	// not present
	if(!slash)
	{
		// now look for platform-specific DIR_SEP
		slash = strrchr(path, DIR_SEP);
		// neither present, it's a filename only
		if(!slash)
			return path;
	}

	const char* name = slash+1;
	return name;
}


// if <path> contains a name component, it is stripped away.
void path_strip_fn(char* path)
{
	char* name = (char*)path_name_only(path);
	*name = '\0';	// cut off string here
}


// fill <dir> with the directory path portion of <path>
// ("" if root dir, otherwise ending with '/').
// note: copies to <dir> and proceeds to path_strip_fn it.
void path_dir_only(const char* path, char* dir)
{
	path_copy(dir, path);
	path_strip_fn(dir);
}


// return extension of <fn>, or "" if there is none.
// NOTE: does not include the period; e.g. "a.bmp" yields "bmp".
const char* path_extension(const char* fn)
{
	const char* dot = strrchr(fn, '.');
	if(!dot)
		return "";
	const char* ext = dot+1;
	return ext;
}


//-----------------------------------------------------------------------------

// convenience "class" that simplifies successively appending a filename to
// its parent directory. this avoids needing to allocate memory and calling
// strlen/strcat. used by wdll_ver and dir_next_ent.
// we want to maintain C compatibility, so this isn't a C++ class.

// write the given directory path into our buffer and set end/chars_left
// accordingly. <dir> need not but can end with a directory separator.
//
// note: <dir> and the filename set via path_package_append_file are separated by
// '/'. this is to allow use on portable paths; the function otherwise
// does not care if paths are relative/portable/absolute.
LibError path_package_set_dir(PathPackage* pp, const char* dir)
{
	// -1 allows for trailing DIR_SEP that will be added if not
	// already present.
	if(strcpy_s(pp->path, ARRAY_SIZE(pp->path)-1, dir) != 0)
		WARN_RETURN(ERR_PATH_LENGTH);
	size_t len = strlen(pp->path);
	// add directory separator if not already present
	// .. but only check this if dir != "" (=> len-1 is safe)
	if(len != 0)
	{
		char* last_char = pp->path+len-1;
		if(!is_dir_sep(*last_char))
		{
			*(last_char+1) = '/';
			// note: need to 0-terminate because pp.path is uninitialized
			// and we overwrite strcpy_s's terminator above.
			*(last_char+2) = '\0';
			// only bump by 1 - filename must overwrite '\0'.
			len++;
		}
	}

	pp->end = pp->path+len;
	pp->chars_left = ARRAY_SIZE(pp->path)-len;
	return ERR_OK;
}


// append the given filename to the directory established by the last
// path_package_set_dir on this package. the whole path is accessible at pp->path.
LibError path_package_append_file(PathPackage* pp, const char* path)
{
	CHECK_ERR(strcpy_s(pp->end, pp->chars_left, path));
	return ERR_OK;
}
