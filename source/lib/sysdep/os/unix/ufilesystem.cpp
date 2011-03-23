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
#include "lib/sysdep/filesystem.h"

#include "lib/path.h"

#include <cstdio>

struct WDIR
{
	DIR* d;
	wchar_t name[PATH_MAX];
	wdirent ent;
};

WDIR* wopendir(const OsPath& path)
{
	DIR* d = opendir(OsString(path).c_str());
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
	wcscpy_s(wd->name, ARRAY_SIZE(wd->name), OsPath(ent->d_name).string().c_str());
	return &wd->ent;
}

int wclosedir(WDIR* wd)
{
	int ret = closedir(wd->d);
	delete wd;
	return ret;
}


int wopen(const OsPath& pathname, int oflag)
{
	debug_assert(!(oflag & O_CREAT));
	return open(OsString(pathname).c_str(), oflag);
}

int wopen(const OsPath& pathname, int oflag, mode_t mode)
{
	return open(OsString(pathname).c_str(), oflag, mode);
}

int wclose(int fd)
{
	return close(fd);
}


int wtruncate(const OsPath& pathname, off_t length)
{
	return truncate(OsString(pathname).c_str(), length);
}

int wunlink(const OsPath& pathname)
{
	return unlink(OsString(pathname).c_str());
}

int wrmdir(const OsPath& path)
{
	return rmdir(OsString(path).c_str());
}

int wrename(const OsPath& pathnameOld, const OsPath& pathnameNew)
{
	return rename(OsString(pathnameOld).c_str(), OsString(pathnameNew).c_str());
}

OsPath wrealpath(const OsPath& pathname)
{
	char resolvedBuf[PATH_MAX];
	const char* resolved = realpath(OsString(pathname).c_str(), resolvedBuf);
	if(!resolved)
		return OsPath();
	return resolved;
}

int wstat(const OsPath& pathname, struct stat* buf)
{
	return stat(OsString(pathname).c_str(), buf);
}

int wmkdir(const OsPath& path, mode_t mode)
{
	return mkdir(OsString(path).c_str(), mode);
}
