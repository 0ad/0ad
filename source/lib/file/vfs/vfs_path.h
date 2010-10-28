/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
#ifdef BOOST_FILESYSTEM2_NAMESPACE
	namespace BOOST_FILESYSTEM2_NAMESPACE
#else
	namespace BOOST_FILESYSTEM_NAMESPACE
#endif
	{
		template<> struct is_basic_path<VfsPath>
		{
			BOOST_STATIC_CONSTANT(bool, value = true);
		};
	}
}

#endif	//	#ifndef INCLUDED_VFS_PATH
