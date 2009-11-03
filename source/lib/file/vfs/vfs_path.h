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

#ifndef INCLUDED_VFS_PATH
#define INCLUDED_VFS_PATH

struct VfsPathTraits;

/**
 * VFS path of the form "(dir/)*file?"
 *
 * in other words: the root directory is "" and paths are separated by '/'.
 * a trailing slash is allowed for directory names.
 * rationale: it is important to avoid a leading slash because that might be
 * confused with an absolute POSIX path.
 *
 * there is no restriction on path length; when dimensioning character
 * arrays, prefer PATH_MAX.
 *
 * rationale: a distinct specialization of basic_path prevents inadvertent
 * assignment from other path types.
 **/
typedef fs::basic_path<std::wstring, VfsPathTraits> VfsPath;

typedef std::vector<VfsPath> VfsPaths;

struct VfsPathTraits
{
	typedef std::wstring internal_string_type;
	typedef std::wstring external_string_type;

	static external_string_type to_external(const VfsPath&, const internal_string_type& src)
	{
		return src;
	}

	static internal_string_type to_internal(const external_string_type& src)
	{
		return src;
	}
};

namespace boost
{
	namespace filesystem
	{
		template<> struct is_basic_path<VfsPath>
		{
			BOOST_STATIC_CONSTANT(bool, value = true);
		};
	}
}

/**
 * Does a path appear to refer to a directory? (non-authoritative)
 *
 * note: only used as a safeguard.
 **/
extern bool vfs_path_IsDirectory(const VfsPath& pathname);

#endif	//	#ifndef INCLUDED_VFS_PATH
