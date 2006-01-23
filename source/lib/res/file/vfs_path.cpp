#include "precompiled.h"

#include <string.h>

#include "lib.h"
#include "vfs.h"
#include "vfs_path.h"


// path types:
// p_*: posix (e.g. mount object name or for open())
// v_*: vfs (e.g. mount point)
// fn : filename only (e.g. from readdir)
// dir_name: directory only, no path (e.g. subdir name)
//
// all paths must be relative (no leading '/'); components are separated
// by '/'; no ':', '\\', "." or ".." allowed; root dir is "".
//
// grammar:
// path ::= dir*file?
// dir  ::= name/
// file ::= name
// name ::= [^/]


// if path is invalid (see source for criteria), print a diagnostic message
// (indicating line number of the call that failed) and
// return a negative error code. used by CHECK_PATH.
LibError path_validate(const uint line, const char* path)
{
	size_t path_len = 0;	// counted as we go; checked against max.

	const char* msg = 0;	// error occurred <==> != 0

	int c = 0, last_c;		// used for ./ detection

	// disallow "/", because it would create a second 'root' (with name = "").
	// root dir is "".
	if(path[0] == '/')
	{
		msg = "starts with '/'";
		goto fail;
	}

	// scan each char in path string; count length.
	for(;;)
	{
		last_c = c;
		c = path[path_len++];

		// whole path is too long
		if(path_len >= VFS_MAX_PATH)
		{
			msg = "path too long";
			goto fail;
		}

		// disallow:
		// - ".." (prevent going above the VFS root dir)
		// - "./" (security hole when mounting and not supported on Windows).
		// allow "/.", because CVS backup files include it.
		if(last_c == '.' && (c == '.' || c == '/'))
		{
			msg = "contains '..' or './'";
			goto fail;
		}

		// disallow OS-specific dir separators
		if(c == '\\' || c == ':')
		{
			msg = "contains OS-specific dir separator (e.g. '\\', ':')";
			goto fail;
		}

		// end of string, all is well.
		if(c == '\0')
			goto ok;
	}

fail:
	debug_printf("%s called from line %u failed: %s\n", __func__, line, msg);
	debug_warn("failed");
	return ERR_FAIL;

ok:
	return ERR_OK;
}


bool path_component_valid(const char* name)
{
	// disallow empty strings
	if(*name == '\0')
		return false;

	int c = 0;
	for(;;)
	{
		int last_c = c;
		c = *name++;

		// disallow '..' (would allow escaping the VFS root dir sandbox)
		if(c == '.' && last_c == '.')
			return false;

		// disallow dir separators
		if(c == '\\' || c == ':' || c == '/')
			return false;

		if(c == '\0')
			return true;
	}
}


// convenience function
void vfs_path_copy(char* dst, const char* src)
{
	strcpy_s(dst, VFS_MAX_PATH, src);
}


// combine <path1> and <path2> into one path, and write to <dst>.
// if necessary, a directory separator is added between the paths.
// each may be empty, filenames, or full paths.
// total path length (including '\0') must not exceed VFS_MAX_PATH.
LibError vfs_path_append(char* dst, const char* path1, const char* path2)
{
	const size_t len1 = strlen(path1);
	const size_t len2 = strlen(path2);
	size_t total_len = len1 + len2 + 1;	// includes '\0'

	// check if we need to add '/' between path1 and path2
	// note: the second can't start with '/' (not allowed by path_validate)
	bool need_separator = false;
	if(len1 != 0 && path1[len1-1] != '/')
	{
		total_len++;	// for '/'
		need_separator = true;
	}

	if(total_len+1 > VFS_MAX_PATH)
		return ERR_PATH_LENGTH;

	strcpy(dst, path1);	// safe
	dst += len1;
	if(need_separator)
		*dst++ = '/';
	strcpy(dst, path2);	// safe
	return ERR_OK;
}


// strip <remove> from the start of <src>, prepend <replace>,
// and write to <dst>.
// used when converting VFS <--> real paths.
LibError path_replace(char* dst, const char* src, const char* remove, const char* replace)
{
	// remove doesn't match start of <src>
	const size_t remove_len = strlen(remove);
	if(strncmp(src, remove, remove_len) != 0)
		return ERR_FAIL;

	// get rid of trailing / in src (must not be included in remove)
	const char* start = src+remove_len;
	if(*start == '/' || *start == DIR_SEP)
		start++;

	// prepend replace.
	CHECK_ERR(vfs_path_append(dst, replace, start));
	return ERR_OK;
}
