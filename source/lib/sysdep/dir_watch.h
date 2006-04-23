/**
 * =========================================================================
 * File        : dir_watch.h
 * Project     : 0 A.D.
 * Description : portable directory change notification API.
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

#ifndef DIR_WATCH_H__
#define DIR_WATCH_H__

// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
extern LibError dir_add_watch(const char* path, intptr_t* watch);

extern LibError dir_cancel_watch(intptr_t watch);

extern LibError dir_get_changed_file(char* fn);

#endif	// #ifndef DIR_WATCH_H__