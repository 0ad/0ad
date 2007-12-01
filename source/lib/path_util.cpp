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
LibError path_append(char* dst, const char* path1, const char* path2, uint flags)
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
		WARN_RETURN(ERR::PATH_LENGTH);

	SAFE_STRCPY(dst, path1);
	dst += len1;
	if(need_separator)
		*dst++ = '/';
	SAFE_STRCPY(dst, path2);
	if(need_terminator)
		SAFE_STRCPY(dst+len2, "/");

	return INFO::OK;
}


const char* path_append2(const char* path1, const char* path2, uint flags)
{
	char dst[PATH_MAX];
	LibError ret = path_append(dst, path1, path2, flags);
	debug_assert(ret == INFO::OK);
	return path_Pool()->UniqueCopy(dst);
}


// strip <remove> from the start of <src>, prepend <replace>,
// and write to <dst>.
// returns ERR::FAIL (without warning!) if the beginning of <src> doesn't
// match <remove>.
LibError path_replace(char* dst, const char* src, const char* remove, const char* replace)
{
	// remove doesn't match start of <src>
	const size_t remove_len = strlen(remove);
	if(strncmp(src, remove, remove_len) != 0)
		return ERR::FAIL;	// NOWARN

	// if removing will leave a separator at beginning of src, remove it
	// (example: "a/b"; removing "a" would yield "/b")
	const char* start = src+remove_len;
	if(path_is_dir_sep(*start))
		start++;

	// prepend replace.
	RETURN_ERR(path_append(dst, replace, start));
	return INFO::OK;
}




//-----------------------------------------------------------------------------

// split paths into specific parts

void path_split(const char* pathname, const char** ppath, const char** pname)
{
	const char* name = path_name_only(pathname);
	if(pname)
	{
		if(name[0] == '\0')
			*pname = 0;
		else
			*pname = path_Pool()->UniqueCopy(name);
	}

	if(ppath)
	{
		char pathnameCopy[PATH_MAX];
		path_copy(pathnameCopy, pathname);

		pathnameCopy[name-pathname] = '\0';	// strip filename
		*ppath = path_Pool()->UniqueCopy(pathnameCopy);
	}
}

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
	return std::max(slash1, slash2)+1;
}


// return last component within path. this is similar to path_name_only,
// but correctly handles VFS paths, which must end with '/'.
// (path_name_only would return "")
const char* path_last_component(const char* path)
{
	// ('\0' is end of set string)
	static const char separators[3] = { '/', '\\', '\0' };

	const char* pos = path;
	const char* last_component = path;

	for(;;)
	{
		// catches empty path and those with trailing separator
		if(*pos == '\0')
			break;
		last_component = pos;
		const size_t component_len = strcspn(pos, separators);
		// catches paths without trailing separator
		// (the 'pos +=' would skip their 0-terminator)
		if(pos[component_len] == '\0')
			break;
		pos += component_len+1;	// +1 for separator
	}

	return last_component;
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
void path_dir_only(const char* pathname, char* path)
{
	path_copy(path, pathname);
	path_strip_fn(path);
}

const char* path_dir_only2(const char* pathname)
{
	char path[PATH_MAX];
	path_dir_only(pathname, path);
	return path_Pool()->UniqueCopy(path);
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


LibError path_foreach_component(const char* path_org, PathComponentCb cb, uintptr_t cbData)
{
	CHECK_PATH(path_org);

	// copy into (writeable) buffer so we can 'tokenize' path components by
	// replacing '/' with '\0'.
	char path[PATH_MAX];
	strcpy_s(path, ARRAY_SIZE(path), path_org);
	char* cur_component = path;

	bool is_dir = true;	// until we find a component without slash

	// successively navigate to the next component in <path>.
	for(;;)
	{
		// at end of string - done.
		// (this happens if <path> is empty or ends with slash)
		if(*cur_component == '\0')
			break;

		// find end of cur_component
		char* slash = (char*)strchr(cur_component, '/');
		// .. try other separator
		if(!slash)
			slash = (char*)strchr(cur_component, '\\');

		// decide its type and 0-terminate
		// .. filename (by definition)
		if(!slash)
			is_dir = false;
		// .. directory
		else
			*slash = '\0';	// 0-terminate cur_component

		LibError ret = cb(cur_component, is_dir, cbData);
		// callback wants to abort - return its value.
		if(ret != INFO::CB_CONTINUE)
			return ret;

		// filename is by definition the last component. abort now
		// in case the callback didn't.
		if(!is_dir)
			break;

		// advance to next component
		// .. undo having replaced '/' with '\0' - this means <path> will
		//    store the complete path up to and including cur_component.
		*slash = '/';
		cur_component = slash+1;
	}

	return INFO::OK;
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
	// -1 allows for trailing '/' that will be added if not
	// already present.
	if(strcpy_s(pp->path, ARRAY_SIZE(pp->path)-1, dir) != 0)
		WARN_RETURN(ERR::PATH_LENGTH);
	size_t len = strlen(pp->path);
	// add directory separator if not already present
	// .. but only check this if dir != "" (=> len-1 is safe)
	if(len != 0)
	{
		char* last_char = pp->path+len-1;
		if(!path_is_dir_sep(*last_char))
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
	return INFO::OK;
}


void path_package_copy(PathPackage* pp_dst, const PathPackage* pp_src)
{
	*pp_dst = *pp_src;
	const ptrdiff_t end_ofs = pp_src->end - pp_src->path;
	pp_dst->end = pp_dst->path + end_ofs;
}


// append the given filename to the directory established by the last
// path_package_set_dir on this package. the whole path is accessible at pp->path.
LibError path_package_append_file(PathPackage* pp, const char* path)
{
	CHECK_ERR(strcpy_s(pp->end, pp->chars_left, path));
	return INFO::OK;
}


//-----------------------------------------------------------------------------

StringPool* path_Pool()
{
	static StringPool* instance;
	if(!instance)
		instance = new StringPool(8*MiB);
	return instance;
}
