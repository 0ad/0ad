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
 * look up directories/files by traversing path components.
 */

#include "precompiled.h"
#include "vfs_lookup.h"

#include "lib/path_util.h"	// path_foreach_component
#include "vfs.h"	// error codes
#include "vfs_tree.h"
#include "vfs_populate.h"

#include "lib/timer.h"


LibError vfs_Lookup(const VfsPath& pathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile** pfile, size_t flags)
{
	// extract and validate flags (ensure no unknown bits are set)
	const bool addMissingDirectories    = (flags & VFS_LOOKUP_ADD) != 0;
	const bool createMissingDirectories = (flags & VFS_LOOKUP_CREATE) != 0;
	debug_assert((flags & ~(VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE)) == 0);

	if(pfile)
		*pfile = 0;

	directory = startDirectory;
	RETURN_ERR(vfs_Populate(directory));

	// early-out for pathname == "" when mounting into VFS root
	if(pathname.empty())	// (prevent iterator error in loop end condition)
		return INFO::OK;

	// for each directory component:
	VfsPath::iterator it;	// (used outside of loop to get filename)
	for(it = pathname.begin(); it != --pathname.end(); ++it)
	{
		const std::string& subdirectoryName = *it;

		VfsDirectory* subdirectory = directory->GetSubdirectory(subdirectoryName);
		if(!subdirectory)
		{
			if(addMissingDirectories)
				subdirectory = directory->AddSubdirectory(subdirectoryName);
			else
				return ERR::VFS_DIR_NOT_FOUND;	// NOWARN
		}

		if(createMissingDirectories && !subdirectory->AssociatedDirectory())
		{
			fs::path currentPath;
			if(directory->AssociatedDirectory())	// (is NULL when mounting into root)
				currentPath = directory->AssociatedDirectory()->Path();
			currentPath /= subdirectoryName;

			const int ret = mkdir(currentPath.external_directory_string().c_str(), S_IRWXU);
			if(ret == 0)
			{
				PRealDirectory realDirectory(new RealDirectory(currentPath, 0, 0));
				RETURN_ERR(vfs_Attach(subdirectory, realDirectory));
			}
			else if(errno != EEXIST)	// notify of unexpected failures
			{
				debug_printf("mkdir failed with errno=%d\n", errno);
				debug_assert(0);
			}
		}

		RETURN_ERR(vfs_Populate(subdirectory));
		directory = subdirectory;
	}

	if(pfile)
	{
		const std::string& filename = *it;
		debug_assert(filename != ".");	// asked for file but specified directory path
		*pfile = directory->GetFile(filename);
		if(!*pfile)
			return ERR::VFS_FILE_NOT_FOUND;	// NOWARN
	}

	return INFO::OK;
}
