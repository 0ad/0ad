/**
 * =========================================================================
 * File        : vfs_lookup.cpp
 * Project     : 0 A.D.
 * Description : look up directories/files by traversing path components.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

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
			Path currentPath;
			if(directory->AssociatedDirectory())	// (is NULL when mounting into root)
				currentPath = directory->AssociatedDirectory()->GetPath()/subdirectoryName;

			if(mkdir(currentPath.external_directory_string().c_str(), S_IRWXO|S_IRWXU|S_IRWXG) == 0)
			{
				PRealDirectory realDirectory(new RealDirectory(currentPath, 0, 0));
				RETURN_ERR(vfs_Attach(subdirectory, realDirectory));
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
