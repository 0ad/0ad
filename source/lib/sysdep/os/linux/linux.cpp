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

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"

#define GNU_SOURCE
#include "mocks/dlfcn.h"
#include "mocks/unistd.h"

#include <cstdio>

LibError sys_get_executable_name(NativePath& pathname)
{
	const char* path;
	Dl_info dl_info;

	// Find the executable's filename
	memset(&dl_info, 0, sizeof(dl_info));
	if (!T::dladdr((void *)sys_get_executable_name, &dl_info) ||
		!dl_info.dli_fname )
	{
		return ERR::NO_SYS;
	}
	path = dl_info.dli_fname;

	// If this looks like an absolute path, use realpath to get the normalized
	// path (with no '.' or '..')
	if (path[0] == '/')
	{
		char resolvedBuf[PATH_MAX];
		char* resolved = realpath(path, resolvedBuf);
		if (!resolved)
			return ERR::FAIL;
		pathname = NativePathFromString(resolved);
		return INFO::OK;
	}

	// If this looks like a relative path, resolve against cwd and use realpath
	if (strchr(path, '/'))
	{
		char cwd[PATH_MAX];
		if (!T::getcwd(cwd, PATH_MAX))
			return ERR::NO_SYS;

		char absolute[PATH_MAX];
		int ret = snprintf(absolute, PATH_MAX, "%s/%s", cwd, path);
		if (ret < 0 || ret >= PATH_MAX)
			return ERR::NO_SYS; // path too long, or other error
		char resolvedBuf[PATH_MAX];
		char* resolved = realpath(absolute, resolvedBuf);
		if (!resolved)
			return ERR::NO_SYS;
		pathname = NativePathFromString(resolved);
		return INFO::OK;
	}

	// If it's not a path at all, i.e. it's just a filename, we'd
	// probably have to search through PATH to find it.
	// That's complex and should be uncommon, so don't bother.
	return ERR::NO_SYS;
}
