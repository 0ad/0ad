/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * manage paths relative to a root directory
 */

// path types:

// tag  type      type      separator
//      portable  relative  /
// os   native    absolute  SYS_DIR_SEP
// vfs  vfs       absolute  /

// the vfs root directory is "". no ':', '\\', "." or ".." are allowed.

#ifndef INCLUDED_PATH
#define INCLUDED_PATH

struct PathTraits;
typedef fs::basic_path<std::string, PathTraits> Path;

struct PathTraits
{
	typedef std::string internal_string_type;
	typedef std::string external_string_type;

	static LIB_API external_string_type to_external(const Path&, const internal_string_type& src);
	static LIB_API internal_string_type to_internal(const external_string_type& src);
};

namespace boost
{
	namespace filesystem
	{
		template<> struct is_basic_path<Path>
		{
			BOOST_STATIC_CONSTANT(bool, value = true);
		};
	}
}

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
 * @param relativePath root directory relative to the executable's directory.
 * the value is considered trusted since it will typically be hard-coded.
 *
 * example: executable in "$install_dir/system"; desired root dir is
 * "$install_dir/data" => rel_path = "../data".
 *
 * can only be called once unless path_ResetRootDir is called.
 **/
LIB_API LibError path_SetRoot(const char* argv0, const char* relativePath);

/**
 * reset root directory that was previously established via path_SetRoot.
 *
 * this function avoids the security complaint that would be raised if
 * path_SetRoot is called twice; it is provided for the
 * legitimate application of a self-test setUp()/tearDown().
 **/
LIB_API void path_ResetRootDir();

// note: path_MakeAbsolute has been replaced by Path::external_directory_string.

#endif	// #ifndef INCLUDED_PATH
