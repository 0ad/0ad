#include "precompiled.h"

#include <string.h>

#include "lib.h"
#include "file_internal.h"


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


// fill V_dir_only with the path portion of V_src_fn
// ("" if root dir, otherwise ending with /)
void path_dir_only(const char* V_src_fn, char* V_dir_only)
{
	vfs_path_copy(V_dir_only, V_src_fn);
	char* slash = strrchr(V_dir_only, '/');
	// was filename only; directory = "" (empty string)
	if(!slash)
		V_dir_only[0] = '\0';
	// normal directory+filename: cut off after last slash
	else
		*(slash+1) = '\0';
}


// return pointer to the name component within V_src_fn
const char* path_name_only(const char* V_src_fn)
{
	const char* slash = strrchr(V_src_fn, '/');
	return slash? slash+1 : V_src_fn;
}



// fill V_next_fn (which must be big enough for VFS_MAX_PATH chars) with
// the next numbered filename according to the pattern defined by V_fn_fmt.
// <nfi> must be initially zeroed (e.g. by defining as static) and passed
// each time.
// if <use_vfs> (default), the paths are treated as VFS paths; otherwise,
// file.cpp's functions are used. this is necessary because one of
// our callers needs a filename for VFS archive files.
//
// this function is useful when creating new files which are not to
// overwrite the previous ones, e.g. screenshots.
// example for V_fn_fmt: "screenshots/screenshot%04d.png".
void next_numbered_filename(const char* fn_fmt,
	NextNumberedFilenameInfo* nfi, char* next_fn, bool use_vfs)
{
	// (first call only:) scan directory and set next_num according to
	// highest matching filename found. this avoids filling "holes" in
	// the number series due to deleted files, which could be confusing.
	// example: add 1st and 2nd; [exit] delete 1st; [restart]
	// add 3rd -> without this measure it would get number 1, not 3. 
	if(nfi->next_num == 0)
	{
		char dir[VFS_MAX_PATH];
		path_dir_only(fn_fmt, dir);
		const char* name_fmt = path_name_only(fn_fmt);

		int max_num = -1; int num;
		DirEnt ent;

		if(use_vfs)
		{
			Handle hd = vfs_dir_open(dir);
			if(hd > 0)
			{
				while(vfs_dir_next_ent(hd, &ent, 0) == ERR_OK)
					if(!DIRENT_IS_DIR(&ent) && sscanf(ent.name, name_fmt, &num) == 1)
						max_num = MAX(num, max_num);
				(void)vfs_dir_close(hd);
			}
		}
		else
		{
			DirIterator it;
			if(dir_open(dir, &it) == ERR_OK)
			{
				while(dir_next_ent(&it, &ent) == ERR_OK)
					if(!DIRENT_IS_DIR(&ent) && sscanf(ent.name, name_fmt, &num) == 1)
						max_num = MAX(num, max_num);
				(void)dir_close(&it);
			}
		}

		nfi->next_num = max_num+1;
	}

	bool (*exists)(const char* fn) = use_vfs? vfs_exists : file_exists;

	// now increment number until that file doesn't yet exist.
	// this is fairly slow, but typically only happens once due
	// to scan loop above. (we still need to provide for looping since
	// someone may have added files in the meantime)
	// binary search isn't expected to improve things.
	do
		snprintf(next_fn, VFS_MAX_PATH, fn_fmt, nfi->next_num++);
	while(exists(next_fn));
}
