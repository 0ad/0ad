/**
 * =========================================================================
 * File        : directory_posix.cpp
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths and provides fast I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "file_system_posix.h"

#include <vector>
#include <algorithm>
#include <string>

#include "lib/path_util.h"
#include "lib/file/path.h"
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

/*virtual*/ LibError FileSystem_Posix::GetDirectoryEntries(const Path& path, FileInfos* files, DirectoryNames* subdirectoryNames) const
{
	// open directory
	errno = 0;
	DIR* pDir = opendir(path.external_file_string().c_str());
	if(!pDir)
		return LibError_from_errno();
	shared_ptr<DIR> osDir(pDir, DirDeleter());

	// (we need absolute paths below; using this instead of path_append avoids
	// a strlen call for each entry.)
	PathPackage pp;
	(void)path_package_set_dir(&pp, path.external_directory_string().c_str());

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
		path_package_append_file(&pp, name);
		if(stat(pp->path, &s) != 0)
			return LibError_from_errno();
#endif

		if(files && S_ISREG(s.st_mode))
			files->push_back(FileInfo(name, s.st_size, s.st_mtime));
		else if(subdirectoryNames && S_ISDIR(s.st_mode) && !IsDummyDirectory(name))
			subdirectoryNames->push_back(name);
	}
}


LibError FileSystem_Posix::GetFileInfo(const Path& pathname, FileInfo* pfileInfo) const
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


LibError FileSystem_Posix::DeleteFile(const Path& pathname)
{
	errno = 0;
	if(unlink(pathname.external_file_string().c_str()) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError FileSystem_Posix::CreateDirectory(const Path& path)
{
	errno = 0;
	if(mkdir(path.external_directory_string().c_str(), S_IRWXO|S_IRWXU|S_IRWXG) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError FileSystem_Posix::DeleteDirectory(const Path& path)
{
	// note: we have to recursively empty the directory before it can
	// be deleted (required by Windows and POSIX rmdir()).

	const char* osPath = path.external_directory_string().c_str();

	PathPackage pp;
	RETURN_ERR(path_package_set_dir(&pp, osPath));

	FileInfos files; DirectoryNames subdirectoryNames;
	RETURN_ERR(GetDirectoryEntries(path, &files, &subdirectoryNames));

	// delete files
	for(size_t i = 0; i < files.size(); i++)
	{
		RETURN_ERR(path_package_append_file(&pp, files[i].Name().c_str()));
		errno = 0;
		if(unlink(pp.path) != 0)
			return LibError_from_errno();
	}

	// recurse over subdirectoryNames
	for(size_t i = 0; i < subdirectoryNames.size(); i++)
		RETURN_ERR(DeleteDirectory(path/subdirectoryNames[i]));

	errno = 0;
	if(rmdir(osPath) != 0)
		return LibError_from_errno();

	return INFO::OK;
}
