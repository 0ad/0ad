/* Copyright (C) 2010 Wildfire Games.
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
 * Unix implementation of wchar_t versions of POSIX filesystem functions
 */

#include "precompiled.h"
#include "lib/posix/posix_filesystem.h"

#include "lib/wchar.h"
#include "lib/path_util.h"

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


int wopen(const wchar_t* pathname, int oflag, ...)
{
	mode_t mode = S_IRWXG|S_IRWXO|S_IRWXU;
	if(oflag & O_CREAT)
	{
		va_list args;
		va_start(args, oflag);
		mode = va_arg(args, mode_t);
		va_end(args);
	}

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
