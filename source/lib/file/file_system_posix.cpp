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
 * file layer on top of POSIX. avoids the need for absolute paths and
 * provides fast I/O.
 */

#include "precompiled.h"
#include "file_system_posix.h"

#include <vector>
#include <algorithm>
#include <string>

#include "lib/path_util.h"
#include "lib/posix/posix_filesystem.h"


struct DirDeleter
{
	void operator()(DIR* osDir) const
	{
		const int ret = closedir(osDir);
		debug_assert(ret == 0);
	}
};

// is name "." or ".."?
static bool IsDummyDirectory(const char* name)
{
	if(name[0] != '.')
		return false;
	return (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}

/*virtual*/ LibError FileSystem_Posix::GetDirectoryEntries(const fs::path& path, FileInfos* files, DirectoryNames* subdirectoryNames) const
{
	// open directory
	errno = 0;
	DIR* pDir = opendir(path.external_file_string().c_str());
	if(!pDir)
		return LibError_from_errno(false);
	shared_ptr<DIR> osDir(pDir, DirDeleter());

	for(;;)
	{
		errno = 0;
		struct dirent* osEnt = readdir(osDir.get());
		if(!osEnt)
		{
			// no error, just no more entries to return
			if(!errno)
				return INFO::OK;
			return LibError_from_errno();
		}

		const char* name = osEnt->d_name;
		RETURN_ERR(path_component_validate(name));

		// get file information (mode, size, mtime)
		struct stat s;
#if OS_WIN
		// .. wposix readdir has enough information to return dirent
		//    status directly (much faster than calling stat).
		RETURN_ERR(readdir_stat_np(osDir.get(), &s));
#else
		// .. call regular stat().
		errno = 0;
		const fs::path pathname(path/name);
		if(stat(pathname.external_directory_string().c_str(), &s) != 0)
			return LibError_from_errno();
#endif

		if(files && S_ISREG(s.st_mode))
			files->push_back(FileInfo(name, s.st_size, s.st_mtime));
		else if(subdirectoryNames && S_ISDIR(s.st_mode) && !IsDummyDirectory(name))
			subdirectoryNames->push_back(name);
	}
}


LibError FileSystem_Posix::GetFileInfo(const fs::path& pathname, FileInfo* pfileInfo) const
{
	char osPathname[PATH_MAX];
	path_copy(osPathname, pathname.external_directory_string().c_str());

	// if path ends in slash, remove it (required by stat)
	char* last_char = osPathname+strlen(osPathname)-1;
	if(path_is_dir_sep(*last_char))
		*last_char = '\0';

	errno = 0;
	struct stat s;
	memset(&s, 0, sizeof(s));
	if(stat(osPathname, &s) != 0)
		return LibError_from_errno();

	const char* name = path_name_only(osPathname);
	*pfileInfo = FileInfo(name, s.st_size, s.st_mtime);
	return INFO::OK;
}


LibError FileSystem_Posix::DeleteDirectory(const fs::path& path)
{
	// note: we have to recursively empty the directory before it can
	// be deleted (required by Windows and POSIX rmdir()).

	FileInfos files; DirectoryNames subdirectoryNames;
	RETURN_ERR(GetDirectoryEntries(path, &files, &subdirectoryNames));

	// delete files
	for(size_t i = 0; i < files.size(); i++)
	{
		const fs::path pathname(path/files[i].Name());
		errno = 0;
		if(unlink(pathname.external_file_string().c_str()) != 0)
			return LibError_from_errno();
	}

	// recurse over subdirectoryNames
	for(size_t i = 0; i < subdirectoryNames.size(); i++)
		RETURN_ERR(DeleteDirectory(path/subdirectoryNames[i]));

	errno = 0;
	if(rmdir(path.external_directory_string().c_str()) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


PIFileSystem_Posix CreateFileSystem_Posix()
{
	return PIFileSystem_Posix(new FileSystem_Posix);
}
