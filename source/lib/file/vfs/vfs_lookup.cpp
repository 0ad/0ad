/* Copyright (C) 2013 Wildfire Games.
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
 * look up directories/files by traversing path components.
 */

#include "precompiled.h"
#include "lib/file/vfs/vfs_lookup.h"

#include "lib/external_libraries/suppress_boost_warnings.h"

#include "lib/sysdep/filesystem.h"
#include "lib/file/file.h"
#include "lib/file/vfs/vfs.h"	// error codes
#include "lib/file/vfs/vfs_tree.h"
#include "lib/file/vfs/vfs_populate.h"

#include "lib/timer.h"


static Status CreateDirectory(const OsPath& path)
{
	{
		const mode_t mode = S_IRWXU; // 0700 as prescribed by XDG basedir
		const int ret = wmkdir(path, mode);
		if(ret == 0)	// success
			return INFO::OK;
	}

	// failed because the directory already exists. this can happen
	// when the first vfs_Lookup has addMissingDirectories &&
	// !createMissingDirectories, and the directory is subsequently
	// created. return 'success' to attach the existing directory..
	if(errno == EEXIST)
	{
		// but first ensure it's really a directory (otherwise, a
		// file is "in the way" and needs to be deleted)
		struct stat s;
		const int ret = wstat(path, &s);
		ENSURE(ret == 0);	// (wmkdir said it existed)
		ENSURE(S_ISDIR(s.st_mode));
		return INFO::OK;
	}

	if (errno == EACCES)
		return ERR::FILE_ACCESS;

	// unexpected failure
	debug_printf("wmkdir failed with errno=%d\n", errno);
	DEBUG_WARN_ERR(ERR::LOGIC);
	WARN_RETURN(StatusFromErrno());
}


Status vfs_Lookup(const VfsPath& pathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile** pfile, size_t flags)
{
	// extract and validate flags (ensure no unknown bits are set)
	const bool addMissingDirectories    = (flags & VFS_LOOKUP_ADD) != 0;
	const bool createMissingDirectories = (flags & VFS_LOOKUP_CREATE) != 0;
	const bool skipPopulate = (flags & VFS_LOOKUP_SKIP_POPULATE) != 0;
	const bool createAlways = (flags & VFS_LOOKUP_CREATE_ALWAYS) != 0;
	ENSURE((flags & ~(VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE|VFS_LOOKUP_SKIP_POPULATE|VFS_LOOKUP_CREATE_ALWAYS)) == 0);

	directory = startDirectory;
	if(pfile)
		*pfile = 0;

	if(!skipPopulate)
		RETURN_STATUS_IF_ERR(vfs_Populate(directory));

	// early-out for pathname == "" when mounting into VFS root
	if(pathname.empty())	// (prevent iterator error in loop end condition)
	{
		if(pfile)	// preserve a guarantee that if pfile then we either return an error or set *pfile
			return ERR::VFS_FILE_NOT_FOUND;
		else
			return INFO::OK;
	}

	// for each directory component:
	size_t pos = 0;	// (needed outside of loop)
	for(;;)
	{
		const size_t nextSlash = pathname.string().find_first_of('/', pos);
		if(nextSlash == VfsPath::String::npos)
			break;
		const VfsPath subdirectoryName = pathname.string().substr(pos, nextSlash-pos);
		pos = nextSlash+1;

		VfsDirectory* subdirectory = directory->GetSubdirectory(subdirectoryName);
		if(!subdirectory)
		{
			if(addMissingDirectories)
				subdirectory = directory->AddSubdirectory(subdirectoryName);
			else
				return ERR::VFS_DIR_NOT_FOUND;	// NOWARN
		}

		if(createMissingDirectories && (!subdirectory->AssociatedDirectory()
		     || (createAlways && (subdirectory->AssociatedDirectory()->Flags() & VFS_MOUNT_REPLACEABLE) != 0)))
		{
			OsPath currentPath;
			if(directory->AssociatedDirectory())	// (is NULL when mounting into root)
				currentPath = directory->AssociatedDirectory()->Path();
			currentPath = currentPath / subdirectoryName;

			RETURN_STATUS_IF_ERR(CreateDirectory(currentPath));

			PRealDirectory realDirectory(new RealDirectory(currentPath, 0, 0));
			RETURN_STATUS_IF_ERR(vfs_Attach(subdirectory, realDirectory));
		}

		if(!skipPopulate)
			RETURN_STATUS_IF_ERR(vfs_Populate(subdirectory));

		directory = subdirectory;
	}

	if(pfile)
	{
		ENSURE(!pathname.IsDirectory());
		const VfsPath filename = pathname.string().substr(pos);
		*pfile = directory->GetFile(filename);
		if(!*pfile)
			return ERR::VFS_FILE_NOT_FOUND;	// NOWARN
	}

	return INFO::OK;
}
