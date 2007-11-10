/**
 * =========================================================================
 * File        : path.h
 * Project     : 0 A.D.
 * Description : helper functions for VFS paths.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_PATH
#define INCLUDED_PATH


namespace ERR
{
	const LibError ROOT_DIR_ALREADY_SET = -110200;
	const LibError NOT_IN_ROOT_DIR      = -110201;
}




// establish the root directory from <rel_path>, which is treated as
// relative to the executable's directory (determined via argv[0]).
// all relative file paths passed to this module will be based from
// this root dir. 
//
// example: executable in "$install_dir/system"; desired root dir is
// "$install_dir/data" => rel_path = "../data".
//
// argv[0] is necessary because the current directory is unknown at startup
// (e.g. it isn't set when invoked via batch file), and this is the
// easiest portable way to find our install directory.
//
// can only be called once, by design (see below). rel_path is trusted.
extern LibError path_SetRoot(const char* argv0, const char* rel_path);


//
// path conversion functions (native <--> portable),
// for external libraries that require the real filename.
//
// replaces '/' with platform's directory separator and vice versa.
// verifies path length < PATH_MAX (otherwise return ERR::PATH_LENGTH).
//

// relative paths (relative to root dir)
extern LibError file_make_native_path(const char* path, char* n_path);
extern LibError file_make_portable_path(const char* n_path, char* path);

// as above, but with full native paths (portable paths are always relative).
// prepends current directory, resp. makes sure it matches the given path.
extern LibError file_make_full_native_path(const char* path, char* n_full_path);
extern LibError file_make_full_portable_path(const char* n_full_path, char* path);




#define VFS_PATH_IS_DIR(path) (*path == '\0' || path[strlen(path)-1] == '/')


extern bool path_is_atom_fn(const char* fn);



// allocate a copy of P_fn in our string pool. strings are equal iff
// their addresses are equal, thus allowing fast comparison.
//
// if the (generous) filename storage is full, 0 is returned.
// this is not ever expected to happen; callers need not check the
// return value because a warning is raised anyway.
extern const char* path_UniqueCopy(const char* P_fn);

extern const char* file_get_random_name();


extern const char* file_get_random_name();


/**
 * reset root directory that was previously established via path_SetRoot.
 *
 * this function avoids the security complaint that would be raised if
 * path_SetRoot is called twice; it is provided for the
 * legitimate application of a self-test setUp()/tearDown().
 **/
extern void path_ResetRootDir();

#endif	// #ifndef INCLUDED_PATH
