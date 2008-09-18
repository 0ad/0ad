/**
 * =========================================================================
 * File        : path_util.cpp
 * Project     : 0 A.D.
 * Description : helper functions for path strings.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "path_util.h"

#include <cstring>
#include <cerrno>


ERROR_ASSOCIATE(ERR::PATH_LENGTH, "Path exceeds PATH_MAX characters", ENAMETOOLONG);
ERROR_ASSOCIATE(ERR::PATH_EMPTY, "Path is an empty string", -1);
ERROR_ASSOCIATE(ERR::PATH_NOT_RELATIVE, "Path is not relative", -1);
ERROR_ASSOCIATE(ERR::PATH_NON_PORTABLE, "Path contains OS-specific dir separator", -1);
ERROR_ASSOCIATE(ERR::PATH_NON_CANONICAL, "Path contains unsupported .. or ./", -1);
ERROR_ASSOCIATE(ERR::PATH_COMPONENT_SEPARATOR, "Path component contains dir separator", -1);


bool path_is_dir_sep(char c)
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

bool path_is_dir_sepw(wchar_t c)
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


bool path_IsDirectory(const char* path)
{
	if(path[0] == '\0')	// root dir
		return true;

	const char lastChar = path[strlen(path)-1];
	if(path_is_dir_sep(lastChar))
		return true;

	return false;
}


// is s2 a subpath of s1, or vice versa?
// (equal counts as subpath)
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
LibError path_validate(const char* path)
{
	// disallow "/", because it would create a second 'root' (with name = "").
	// root dir is "".
	if(path[0] == '/')
		WARN_RETURN(ERR::PATH_NOT_RELATIVE);

	// scan each char in path string; count length.
	int c = 0;		// current char; used for .. detection
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
LibError path_component_validate(const char* name)
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
void path_copy(char* dst, const char* src)
{
	strcpy_s(dst, PATH_MAX, src);
}


// combine <path1> and <path2> into one path, and write to <dst>.
// if necessary, a directory separator is added between the paths.
// each may be empty, filenames, or full paths.
// total path length (including '\0') must not exceed PATH_MAX.
void path_append(char* dst, const char* path1, const char* path2, size_t flags)
{
	const size_t len1 = strlen(path1);
	const size_t len2 = strlen(path2);
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

	SAFE_STRCPY(dst, path1);
	dst += len1;
	if(need_separator)
		*dst++ = '/';
	SAFE_STRCPY(dst, path2);
	if(need_terminator)
		SAFE_STRCPY(dst+len2, "/");
}


//-----------------------------------------------------------------------------

// return pointer to the name component within path (i.e. skips over all
// characters up to the last dir separator, if any).
const char* path_name_only(const char* path)
{
	const char* slash1 = strrchr(path, '/');
	const char* slash2 = strrchr(path, '\\');
	// neither present, it's a filename only
	if(!slash1 && !slash2)
		return path;

	// return name, i.e. component after the last portable or platform slash
	const char* name = std::max(slash1, slash2)+1;
	if(name[0] != '\0')	// else path_component_validate would complain
		path_component_validate(name);
	return name;
}

// if <path> contains a name component, it is stripped away.
void path_strip_fn(char* path)
{
	char* name = (char*)path_name_only(path);
	*name = '\0';	// cut off string here
	debug_assert(path_IsDirectory(path));
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
