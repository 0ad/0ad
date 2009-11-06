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

#include "precompiled.h"

#include "lib/wchar.h"
#include "lib/sysdep/sysdep.h"
#include "lib/external_libraries/boost_filesystem.h"

#define GNU_SOURCE
#include "mocks/dlfcn.h"
#include "mocks/unistd.h"

LibError sys_get_executable_name(fs::wpath& pathname)
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
		pathname = wstring_from_string(resolved);
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
		pathname = wstring_from_string(resolved);
		return INFO::OK;
	}

	// If it's not a path at all, i.e. it's just a filename, we'd
	// probably have to search through PATH to find it.
	// That's complex and should be uncommon, so don't bother.
	return ERR::NO_SYS;
}
