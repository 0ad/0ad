/**
 * =========================================================================
 * File        : dir_watch.h
 * Project     : 0 A.D.
 * Description : portable directory change notification API.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_DIR_WATCH
#define INCLUDED_DIR_WATCH

// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
extern LibError dir_add_watch(const char* path, intptr_t* watch);

extern LibError dir_cancel_watch(intptr_t watch);

extern LibError dir_get_changed_file(char* fn);

#endif	// #ifndef INCLUDED_DIR_WATCH
