#ifndef VFS_UTIL_H__
#define VFS_UTIL_H__

#include "lib.h"

// if path is invalid (see source for criteria), print a diagnostic message
// (indicating line number of the call that failed) and
// return a negative error code. used by CHECK_PATH.
extern int path_validate(const uint line, const char* path);

#define CHECK_PATH(path) CHECK_ERR(path_validate(__LINE__, path))

// convenience function
extern void path_copy(char* dst, const char* src);

// combine <path1> and <path2> into one path, and write to <dst>.
// if necessary, a directory separator is added between the paths.
// each may be empty, filenames, or full paths.
// total path length (including '\0') must not exceed VFS_MAX_PATH.
extern int path_append(char* dst, const char* path1, const char* path2);

// strip <remove> from the start of <src>, prepend <replace>,
// and write to <dst>.
// used when converting VFS <--> real paths.
extern int path_replace(char* dst, const char* src, const char* remove, const char* replace);

#endif	// #ifndef VFS_UTIL_H__