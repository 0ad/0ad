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

#ifndef INCLUDED_NATIVE_PATH
#define INCLUDED_NATIVE_PATH

#include <string>

// rationale:
// this is conceptually a different kind of path, not a superset of VfsPath,
// hence NativePath instead of Path (PathUtil is a bit clunky as a
// namespace anyway).
// a typedef instead of wrapper class avoids the need for accessor functions
// (e.g. boost::filesystem::string()) at the cost of somewhat diminished safety.
// users are responsible for ensuring the path doesn't contain any forbidden
// characters (including any code points >= 0x100 on anything but Windows)
typedef std::wstring NativePath;

static inline NativePath NativePathFromString(const std::string& string)
{
	return NativePath(string.begin(), string.end());
}

#if OS_WIN

static inline std::wstring StringFromNativePath(const NativePath& npath)
{
	return npath;
}

#else

static inline std::string StringFromNativePath(const NativePath& npath)
{
	std::string string(npath.length(), '\0');
	for(size_t i = 0; i < npath.length(); i++)
	{
		debug_assert(npath[i] <= UCHAR_MAX);
		string[i] = npath[i];
	}
	return string;
}

#endif

#endif	// #ifndef INCLUDED_NATIVE_PATH
