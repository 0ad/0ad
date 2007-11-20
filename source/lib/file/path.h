/**
 * =========================================================================
 * File        : path.h
 * Project     : 0 A.D.
 * Description : manage paths relative to a root directory
 * =========================================================================
 */

// license: GPL; see lib/license.txt

// path types:

// tag  type      type      separator
//      portable  relative  /
// os   native    absolute  SYS_DIR_SEP
// vfs  vfs       absolute  /

// the vfs root directory is "". no ':', '\\', "." or ".." are allowed.

#ifndef INCLUDED_PATH
#define INCLUDED_PATH

namespace ERR
{
	const LibError PATH_ROOT_DIR_ALREADY_SET = -110200;
	const LibError PATH_NOT_IN_ROOT_DIR      = -110201;
}

/**
 * establish the root OS directory (portable paths are relative to it)
 *
 * @param argv0 the value of argv[0] (used to determine the location
 * of the executable in case sys_get_executable_path fails). note that
 * the current directory cannot be used because it's not set when
 * starting via batch file.
 * @param rel_path root directory relative to the executable's directory.
 * the value is considered trusted since it will typically be hard-coded.
 *
 * example: executable in "$install_dir/system"; desired root dir is
 * "$install_dir/data" => rel_path = "../data".
 *
 * can only be called once unless path_ResetRootDir is called.
 **/
extern LibError path_SetRoot(const char* argv0, const char* rel_path);

/**
 * reset root directory that was previously established via path_SetRoot.
 *
 * this function avoids the security complaint that would be raised if
 * path_SetRoot is called twice; it is provided for the
 * legitimate application of a self-test setUp()/tearDown().
 **/
extern void path_ResetRootDir();


/**
 * return the absolute OS path for a given relative portable path.
 *
 * this is useful for external libraries that require the real filename.
 **/
extern void path_MakeAbsolute(const char* path, char* osPath);

/**
 * return the relative portable path for a given absolute OS path.
 *
 * this is useful when receiving paths from external libraries (e.g. FAM).
 **/
extern void path_MakeRelative(const char* osPath, char* path);

#endif	// #ifndef INCLUDED_PATH
