/**
 * =========================================================================
 * File        : vfs_path.h
 * Project     : 0 A.D.
 * Description : helper functions for VFS paths.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2004-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef VFS_PATH_H__
#define VFS_PATH_H__

#include "lib.h"

// internal use only:

// if path is invalid (see source for criteria), return a
// descriptive error code, otherwise ERR_OK.
extern LibError path_validate(const char* path);
#define CHECK_PATH(path) CHECK_ERR(path_validate(path))

// if name is invalid, (see source for criteria), return a
// descriptive error code, otherwise ERR_OK.
extern LibError path_component_validate(const char* name);


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
