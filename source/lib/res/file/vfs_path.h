#ifndef VFS_PATH_H__
#define VFS_PATH_H__

#include "lib.h"

// internal use only:

// if path is invalid (see source for criteria), print a diagnostic message
// (indicating line number of the call that failed) and
// return a negative error code. used by CHECK_PATH.
extern LibError path_validate(const uint line, const char* path);
#define CHECK_PATH(path) CHECK_ERR(path_validate(__LINE__, path))

extern bool path_component_valid(const char* name);

// strip <remove> from the start of <src>, prepend <replace>,
// and write to <dst>.
// used when converting VFS <--> real paths.
extern LibError path_replace(char* dst, const char* src, const char* remove, const char* replace);


// fill V_dir_only with the path portion of V_src_fn
// ("" if root dir, otherwise ending with /)
extern void path_dir_only(const char* V_src_fn, char* V_dir_only);

// return pointer to the name component within V_src_fn
extern const char* path_name_only(const char* V_src_fn);


struct NextNumberedFilenameInfo
{
	int next_num;
};

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
extern void next_numbered_filename(const char* V_fn_fmt,
	NextNumberedFilenameInfo* nfi, char* V_next_fn, bool use_vfs = true);


#endif	// #ifndef VFS_PATH_H__
