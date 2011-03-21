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

/*
 * Unix implementation of wchar_t versions of POSIX filesystem functions
 */

#include "precompiled.h"
#include "lib/posix/posix_filesystem.h"

#include "lib/path_util.h"

#include <cstdio>

struct WDIR
{
	DIR* d;
	wchar_t name[300];
	wdirent ent;
};

WDIR* wopendir(const wchar_t* wpath)
{
	const std::string path = StringFromNativePath(wpath);
	DIR* d = opendir(path.c_str());
	if(!d)
		return 0;
	WDIR* wd = new WDIR;
	wd->d = d;
	wd->name[0] = '\0';
	wd->ent.d_name = wd->name;
	return wd;
}

struct wdirent* wreaddir(WDIR* wd)
{
	dirent* ent = readdir(wd->d);
	if(!ent)
		return 0;
	NativePath name = NativePathFromString(ent->d_name);
	wcscpy_s(wd->name, ARRAY_SIZE(wd->name), name.c_str());
	return &wd->ent;
}

int wclosedir(WDIR* wd)
{
	int ret = closedir(wd->d);
	delete wd;
	return ret;
}


int wopen(const wchar_t* wpathname, int oflag)
{
	debug_assert(!(oflag & O_CREAT));
	const std::string pathname = StringFromNativePath(wpathname);
	return open(pathname.c_str(), oflag);
}

int wopen(const wchar_t* wpathname, int oflag, mode_t mode)
{
	const std::string pathname = StringFromNativePath(wpathname);
	return open(pathname.c_str(), oflag, mode);
}

int wclose(int fd)
{
	return close(fd);
}


int wtruncate(const wchar_t* wpathname, off_t length)
{
	const std::string pathname = StringFromNativePath(wpathname);
	return truncate(pathname.c_str(), length);
}

int wunlink(const wchar_t* wpathname)
{
	const std::string pathname = StringFromNativePath(wpathname);
	return unlink(pathname.c_str());
}

int wrmdir(const wchar_t* wpath)
{
	const std::string path = StringFromNativePath(wpath);
	return rmdir(path.c_str());
}

int wrename(const wchar_t* wpathnameOld, const wchar_t* wpathnameNew)
{
	const std::string pathnameOld = StringFromNativePath(wpathnameOld);
	const std::string pathnameNew = StringFromNativePath(wpathnameNew);
	return rename(pathnameOld.c_str(), pathnameNew.c_str());
}

wchar_t* wrealpath(const wchar_t* wpathname, wchar_t* wresolved)
{
	char resolvedBuf[PATH_MAX];
	const std::string pathname = StringFromNativePath(wpathname);
	const char* resolved = realpath(pathname.c_str(), resolvedBuf);
	if(!resolved)
		return 0;
	NativePath nresolved = NativePathFromString(resolved);
	wcscpy_s(wresolved, PATH_MAX, nresolved.c_str());
	return wresolved;
}

int wstat(const wchar_t* wpathname, struct stat* buf)
{
	const std::string pathname = StringFromNativePath(wpathname);
	return stat(pathname.c_str(), buf);
}

int wmkdir(const wchar_t* wpath, mode_t mode)
{
	const std::string path = StringFromNativePath(wpath);
	return mkdir(path.c_str(), mode);
}
