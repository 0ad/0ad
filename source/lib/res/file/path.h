/**
 * =========================================================================
 * File        : path.h
 * Project     : 0 A.D.
 * Description : helper functions for VFS paths.
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

#ifndef VFS_PATH_H__
#define VFS_PATH_H__

#include "lib.h"

#define VFS_PATH_IS_DIR(path) (*path == '\0' || path[strlen(path)-1] == '/')

struct NextNumberedFilenameInfo
{
	int next_num;
};

// fill V_next_fn (which must be big enough for PATH_MAX chars) with
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


extern bool path_is_atom_fn(const char* fn);

extern const char* file_get_random_name();

// note: other functions are declared directly in the public file.h header.


extern void path_init();
extern void path_shutdown();

#endif	// #ifndef VFS_PATH_H__
