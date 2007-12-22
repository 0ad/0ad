/**
 * =========================================================================
 * File        : path_util.h
 * Project     : 0 A.D.
 * Description : helper functions for path strings.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

// notes:
// - this module is independent of lib/file so that it can be used from
//   other code without pulling in the entire file manager.
// - there is no restriction on buffer lengths except the underlying OS.
//   input buffers must not exceed PATH_MAX chars, while outputs
//   must hold at least that much.
// - unless otherwise mentioned, all functions are intended to work with
//   native and portable and VFS paths.
//   when reading, both '/' and SYS_DIR_SEP are accepted; '/' is written.

#ifndef INCLUDED_PATH_UTIL
#define INCLUDED_PATH_UTIL

namespace ERR
{
	const LibError PATH_LENGTH              = -100300;
	const LibError PATH_EMPTY               = -100301;
	const LibError PATH_NOT_RELATIVE        = -100302;
	const LibError PATH_NON_PORTABLE        = -100303;
	const LibError PATH_NON_CANONICAL       = -100304;
	const LibError PATH_COMPONENT_SEPARATOR = -100305;
}


/**
 * check if path is valid. (see source for criteria)
 *
 * @return LibError (ERR::PATH_* or INFO::OK)
 **/
LIB_API LibError path_validate(const char* path);

/**
 * return appropriate code if path is invalid, otherwise continue.
 **/
#define CHECK_PATH(path) RETURN_ERR(path_validate(path))

/**
 * check if name is valid. (see source for criteria)
 *
 * @return LibError (ERR::PATH_* or INFO::OK)
 **/
LIB_API LibError path_component_validate(const char* name);

/**
 * is the given character a path separator character?
 *
 * @param c character to test
 * @return bool
 **/
LIB_API bool path_is_dir_sep(char c);

/**
 * is the given path(name) a directory?
 *
 * @return bool
 **/
LIB_API bool path_IsDirectory(const char* path);

/**
 * is s2 a subpath of s1, or vice versa? (equal counts as subpath)
 *
 * @param s1, s2 comparand strings
 * @return bool
 **/
LIB_API bool path_is_subpath(const char* s1, const char* s2);


/**
 * copy path strings (provided for convenience).
 *
 * @param dst destination; must be at least as large as source buffer,
 * and should hold PATH_MAX chars.
 * @param src source; should not exceed PATH_MAX chars
 **/
LIB_API void path_copy(char* dst, const char* src);

/**
 * flags controlling path_append behavior
 **/
enum PathAppendFlags
{
	/**
	 * make sure <dst> ends up with a trailing slash. this is useful for
	 * VFS directory paths, which have that requirement.
	 **/
	PATH_APPEND_SLASH = 1
};

/**
 * append one path onto another, adding directory separator if necessary.
 *
 * @param dst destination into which combined path is written;
 * must hold at least PATH_MAX chars.
 * @param path1, path2 strings: empty, filenames, or full paths.
 * total resulting string must not exceed PATH_MAX chars.
 * @param flags see PathAppendFlags.
 * @return LibError
 **/
LIB_API LibError path_append(char* dst, const char* path1, const char* path2, uint flags = 0);

/**
 * at the start of a path, replace the given substring with another.
 *
 * @param dst destination; must hold at least PATH_MAX chars.
 * @param src source string.
 * @param remove substring to remove; must match (case-sensitive) the
 * start of src.
 * @param replace string to prepend to output after stripping <remove>.
 * @return LibError; ERR::FAIL (without warning!) if <src> doesn't
 * match <remove>.
 **/
LIB_API LibError path_replace(char* dst, const char* src, const char* remove, const char* replace);

/**
 * combination of path_name_only and path_dir_only2
 * (more efficient than calling them separately)
 **/
LIB_API void path_split(const char* pathname, char* path, char* name);


/**
 * get the name component of a path.
 *
 * skips over all characters up to the last dir separator, if any.
 * @param path input path.
 * @return pointer to name component within <path>.
 **/
LIB_API const char* path_name_only(const char* path);

/**
 * get the last component of a path.
 *
 * this is similar to path_name_only, but correctly handles VFS paths,
 * which must end with '/'. (path_name_only would return "")
 * @param path input path.
 * @return pointer to last component within <path>.
 **/
LIB_API const char* path_last_component(const char* path);

/**
 * strip away the name component in a path.
 *
 * @param path input and output; chopped by inserting '\0'.
 **/
LIB_API void path_strip_fn(char* path);

/**
 * retrieve only the directory path portion of a path.
 *
 * @param path source path.
 * @param dir output directory path ("" if root dir,
 * otherwise it ends with '/').
 * note: implementation via path_copy and path_strip_fn.
 **/
LIB_API void path_dir_only(const char* path, char* dir);

/**
 * get filename's extension.
 *
 * @return pointer to extension within <fn>, or "" if there is none.
 * NOTE: does not include the period; e.g. "a.bmp" yields "bmp".
 **/
LIB_API const char* path_extension(const char* fn);

#endif	// #ifndef INCLUDED_PATH_UTIL
