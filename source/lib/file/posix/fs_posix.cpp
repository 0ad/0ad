/**
 * =========================================================================
 * File        : fp_posix.cpp
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

#include "../path.h"
#include "lib/posix/posix_filesystem.h"


//-----------------------------------------------------------------------------
// DirectoryIterator_Posix
//-----------------------------------------------------------------------------

DirectoryIterator_Posix::DirectoryIterator_Posix(const char* P_path)
{
	char N_path[PATH_MAX];
	(void)file_make_full_native_path(P_path, N_path);
	m_osDir = opendir(N_path);

	// note: copying to N_path and then &m_pp.path is inefficient but
	// more clear/robust. this is only called a few hundred times anyway.
	(void)path_package_set_dir(&m_pp, N_path);
}


DirectoryIterator_Posix::~DirectoryIterator_Posix()
{
	if(m_osDir)
	{
		const int ret = closedir(m_osDir);
		debug_assert(ret == 0);
	}
}


LibError DirectoryIterator_Posix::NextEntry(FilesystemEntry& fsEntry)
{
	if(!m_osDir)
		return ERR::DIR_NOT_FOUND;

get_another_entry:
	errno = 0;
	struct dirent* osEnt = readdir(m_osDir);
	if(!osEnt)
	{
		// no error, just no more entries to return
		if(!errno)
			return ERR::DIR_END;	// NOWARN
		return LibError_from_errno();
	}

	// copying into the global filename storage avoids the need for users to
	// free fsEntry.name and is convenient+safe.
	const char* atom_fn = path_UniqueCopy(osEnt->d_name);

	// get file information (mode, size, mtime)
	struct stat s;
#if OS_WIN
	// .. wposix readdir has enough information to return dirent
	//    status directly (much faster than calling stat).
	RETURN_ERR(readdir_stat_np(m_osDir, &s));
#else
	// .. call regular stat().
	errno = 0;
	// (we need the full pathname; don't use path_append because it would
	// unnecessarily call strlen.)
	path_package_append_file(&m_pp, atom_fn);
	if(stat(&m_pp->path, &s) != 0)
		return LibError_from_errno();
#endif

	// skip "undesirable" entries that POSIX readdir returns:
	if(S_ISDIR(s.st_mode))
	{
		// .. dummy directory entries ("." and "..")
		if(atom_fn[0] == '.' && (atom_fn[1] == '\0' || (atom_fn[1] == '.' && atom_fn[2] == '\0')))
			goto get_another_entry;

		s.st_size = -1;	// our way of indicating it's a directory
	}
	// .. neither dir nor file
	else if(!S_ISREG(s.st_mode))
		goto get_another_entry;

	fsEntry.size  = s.st_size;
	fsEntry.mtime = s.st_mtime;
	fsEntry.name  = atom_fn;
	return INFO::OK;
}


//-----------------------------------------------------------------------------
// Filesystem_Posix
//-----------------------------------------------------------------------------

LibError Filesystem_Posix::GetEntry(const char* P_pathname, FilesystemEntry& fsEntry) const
{
	char N_pathname[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(P_pathname, N_pathname));

	// if path ends in slash, remove it (required by stat)
	char* last_char = N_pathname+strlen(N_pathname)-1;
	if(path_is_dir_sep(*last_char))
		*last_char = '\0';

	errno = 0;
	struct stat s;
	memset(&s, 0, sizeof(s));
	if(stat(N_pathname, &s) != 0)
		return LibError_from_errno();

	fsEntry.size  = s.st_size;
	fsEntry.mtime = s.st_mtime;
	fsEntry.name = path_UniqueCopy(path_name_only(N_pathname));
	fsEntry.mount = 0;
	return INFO::OK;
}


LibError Filesystem_Posix::CreateDirectory(const char* P_dirPath)
{
	char N_dirPath[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(P_dirPath, N_dirPath));

	errno = 0;
	struct stat s;
	if(stat(N_dirPath, &s) != 0)
		return LibError_from_errno();

	errno = 0;
	if(mkdir(N_dirPath, S_IRWXO|S_IRWXU|S_IRWXG) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError Filesystem_Posix::DeleteDirectory(const char* P_dirPath)
{
	// note: we have to recursively empty the directory before it can
	// be deleted (required by Windows and POSIX rmdir()).

	char N_dirPath[PATH_MAX];
	RETURN_ERR(file_make_full_native_path(P_dirPath, N_dirPath));
	PathPackage N_pp;
	RETURN_ERR(path_package_set_dir(&N_pp, N_dirPath));

	{
		// (must go out of scope before rmdir)
		DirectoryIterator_Posix di(P_dirPath);
		for(;;)
		{
			FilesystemEntry fsEntry;
			LibError err = di.NextEntry(fsEntry);
			if(err == ERR::DIR_END)
				break;
			RETURN_ERR(err);

			if(fsEntry.IsDirectory())
			{
				char P_subdirPath[PATH_MAX];
				RETURN_ERR(path_append(P_subdirPath, P_dirPath, fsEntry.name));
				RETURN_ERR(DeleteDirectory(P_subdirPath));
			}
			else
			{
				RETURN_ERR(path_package_append_file(&N_pp, fsEntry.name));

				errno = 0;
				if(unlink(N_pp.path) != 0)
					return LibError_from_errno();
			}
		}
	}

	errno = 0;
	if(rmdir(N_dirPath) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


IDirectoryIterator* Filesystem_Posix::OpenDirectory(const char* P_dirPath) const
{
	return new DirectoryIterator_Posix(P_dirPath);
}


LibError CreateFile(const char* P_pathname, const u8* buf, size_t size, uint flags = 0)
{
	File_Posix file;
	RETURN_ERR(file.Open(P_pathname, 'w', flags));
	RETURN_ERR(io(file, 0, buf, size));
	return INFO::OK;
}


LibError Filesystem_Posix::DeleteFile(const char* P_pathname)
{
	char N_pathname[PATH_MAX+1];
	RETURN_ERR(file_make_full_native_path(P_pathname, N_pathname));

	errno = 0;
	if(unlink(N_pathname) != 0)
		return LibError_from_errno();

	return INFO::OK;
}


LibError LoadFile(const char* P_pathname, const u8*& buf, size_t size, uint flags = 0, IoCallback cb = 0, uintptr_t cbData = 0)
{
	File_Posix file;
	RETURN_ERR(file.Open(P_pathname, 'r', flags));
	RETURN_ERR(io(file, 0, buf, size, cb, cbData));

	return INFO::OK;
}
