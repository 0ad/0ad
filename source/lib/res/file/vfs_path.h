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

#endif	// #ifndef VFS_PATH_H__
