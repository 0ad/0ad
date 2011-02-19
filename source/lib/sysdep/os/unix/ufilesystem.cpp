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

#include "lib/utf8.h"
#include "lib/path_util.h"

#include <cstdio>

struct WDIR
{
	DIR* d;
	wchar_t name[300];
	wdirent ent;
};

WDIR* wopendir(const wchar_t* path)
{
	fs::path path_c(path_from_wpath(path));
	DIR* d = opendir(path_c.string().c_str());
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
	std::wstring name = wstring_from_utf8(ent->d_name);
	wcscpy_s(wd->name, ARRAY_SIZE(wd->name), name.c_str());
	return &wd->ent;
}

int wclosedir(WDIR* wd)
{
	int ret = closedir(wd->d);
	delete wd;
	return ret;
}


int wopen(const wchar_t* pathname, int oflag)
{
	debug_assert(!(oflag & O_CREAT));
	fs::path pathname_c(path_from_wpath(pathname));
	return open(pathname_c.string().c_str(), oflag);
}

int wopen(const wchar_t* pathname, int oflag, mode_t mode)
{
	fs::path pathname_c(path_from_wpath(pathname));
	return open(pathname_c.string().c_str(), oflag, mode);
}

int wclose(int fd)
{
	return close(fd);
}


int wtruncate(const wchar_t* pathname, off_t length)
{
	fs::path pathname_c(path_from_wpath(pathname));
	return truncate(pathname_c.string().c_str(), length);
}

int wunlink(const wchar_t* pathname)
{
	fs::path pathname_c(path_from_wpath(pathname));
	return unlink(pathname_c.string().c_str());
}

int wrmdir(const wchar_t* path)
{
	fs::path path_c(path_from_wpath(path));
	return rmdir(path_c.string().c_str());
}

int wrename(const wchar_t* pathnameOld, const wchar_t* pathnameNew)
{
	fs::path pathnameOld_c(path_from_wpath(pathnameOld));
	fs::path pathnameNew_c(path_from_wpath(pathnameNew));
	return rename(pathnameOld_c.string().c_str(), pathnameNew_c.string().c_str());
}

wchar_t* wrealpath(const wchar_t* pathname, wchar_t* resolved)
{
	char resolved_buf[PATH_MAX];
	fs::path pathname_c(path_from_wpath(pathname));
	const char* resolved_c = realpath(pathname_c.string().c_str(), resolved_buf);
	if(!resolved_c)
		return 0;
	std::wstring resolved_s = wstring_from_utf8(resolved_c);
	wcscpy_s(resolved, PATH_MAX, resolved_s.c_str());
	return resolved;
}

int wstat(const wchar_t* pathname, struct stat* buf)
{
	fs::path pathname_c(path_from_wpath(pathname));
	return stat(pathname_c.string().c_str(), buf);
}

int wmkdir(const wchar_t* path, mode_t mode)
{
	fs::path path_c(path_from_wpath(path));
	return mkdir(path_c.string().c_str(), mode);
}
