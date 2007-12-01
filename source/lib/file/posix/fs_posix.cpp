/**
 * =========================================================================
 * File        : fs_posix.cpp
 * Project     : 0 A.D.
 * Description : file layer on top of POSIX. avoids the need for
 *             : absolute paths and provides fast I/O.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "fs_posix.h"

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

/*virtual*/ LibError Filesystem_Posix::GetDirectoryEntries(const char* path, FileInfos* files, Directories* subdirectories) const
{
	// open directory
	char osPath[PATH_MAX];
	path_MakeAbsolute(path, osPath);
	errno = 0;
	boost::shared_ptr<DIR> osDir(opendir(osPath), DirDeleter());
	if(!osDir.get())
		return LibError_from_errno();

	// (we need absolute paths below; using this instead of path_append avoids
	// a strlen call for each entry.)
	PathPackage pp;
	(void)path_package_set_dir(&pp, osPath);

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

		const char* name = path_Pool()->UniqueCopy(osEnt->d_name);
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
		else if(subdirectories && S_ISDIR(s.st_mode) && !IsDummyDirectory(name))
			subdirectories->push_back(name);
	}
}


LibError Filesystem_Posix::GetFileInfo(const char* pathname, FileInfo& fileInfo) const
{
	char osPathname[PATH_MAX];
	path_MakeAbsolute(pathname, osPathname);

	// if path ends in slash, remove it (required by stat)
	char* last_char = osPathname+strlen(osPathname)-1;
	if(path_is_dir_sep(*last_char))
		*last_char = '\0';

	errno = 0;
	struct stat s;
	memset(&s, 0, sizeof(s));
	if(stat(osPathname, &s) != 0)
		return LibError_from_errno();

	const char* name = path_Pool()->UniqueCopy(path_name_only(osPathname));
	fileInfo = FileInfo(name, s.st_size, s.st_mtime);
	return INFO::OK;
}


LibError Filesystem_Posix::DeleteFile(const char* pathname)
{
	char osPathname[PATH_MAX+1];
	path_MakeAbsolute(pathname, osPathname);

	errno = 0;
	if(unlink(osPathname) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError Filesystem_Posix::CreateDirectory(const char* path)
{
	char osPath[PATH_MAX];
	path_MakeAbsolute(path, osPath);

	errno = 0;
	struct stat s;
	if(stat(osPath, &s) != 0)
		return LibError_from_errno();

	errno = 0;
	if(mkdir(osPath, S_IRWXO|S_IRWXU|S_IRWXG) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError Filesystem_Posix::DeleteDirectory(const char* path)
{
	// note: we have to recursively empty the directory before it can
	// be deleted (required by Windows and POSIX rmdir()).

	char osPath[PATH_MAX];
	path_MakeAbsolute(path, osPath);
	PathPackage pp;
	RETURN_ERR(path_package_set_dir(&pp, osPath));

	FileInfos files; Directories subdirectories;
	RETURN_ERR(GetDirectoryEntries(path, &files, &subdirectories));

	// delete files
	for(size_t i = 0; i < files.size(); i++)
	{
		RETURN_ERR(path_package_append_file(&pp, files[i].Name()));
		errno = 0;
		if(unlink(pp.path) != 0)
			return LibError_from_errno();
	}

	// recurse over subdirectories
	for(size_t i = 0; i < subdirectories.size(); i++)
	{
		char subdirectoryPath[PATH_MAX];
		path_append(subdirectoryPath, path, subdirectories[i]);
		RETURN_ERR(DeleteDirectory(subdirectoryPath));
	}

	errno = 0;
	if(rmdir(osPath) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


//-----------------------------------------------------------------------------

/*virtual*/ unsigned FileProvider_Posix::Precedence() const
{
	return 1u;
}

/*virtual*/ char FileProvider_Posix::LocationCode() const
{
	return 'F';
}

/*virtual*/ LibError FileProvider_Posix::LoadFile(const char* name, const void* location, u8* fileContents, size_t size) const
{
	const char* path = (const char*)location;
	const char* pathname = path_append2(path, name);
	File_Posix file;
	RETURN_ERR(file.Open(pathname, 'r'));
	RETURN_ERR(io_Read(file, 0, buf, size));
	return INFO::OK;
}

/*virtual*/ LibError FileProvider_Posix::StoreFile(const char* name, const void* location, const u8* fileContents, size_t size) const
{
	const char* path = (const char*)location;
	const char* pathname = path_append2(path, name);
	File_Posix file;
	RETURN_ERR(file.Open(pathname, 'r'));
	RETURN_ERR(io_Write(file, 0, buf, size));
	return INFO::OK;
}
