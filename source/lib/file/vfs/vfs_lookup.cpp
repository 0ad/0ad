/**
 * =========================================================================
 * File        : vfs_lookup.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_lookup.h"

#include "lib/path_util.h"	// path_foreach_component
#include "vfs_tree.h"
#include "vfs_populate.h"
#include "vfs.h"	// error codes


class PathResolver
{
public:
	PathResolver(VfsDirectory* startDirectory, uint flags = 0)
		: m_flags(flags), m_currentDirectory(startDirectory)
	{
	}

	static LibError Callback(const char* component, bool isDirectory, uintptr_t cbData)
	{
		PathResolver* pathResolver = (PathResolver*)cbData;
		return pathResolver->Next(component, isDirectory);
	}

	LibError Next(const char* component, bool isDirectory) const
	{
		if((m_flags & VFS_LOOKUP_NO_POPULATE ) == 0)
			RETURN_ERR(vfs_Populate(m_currentDirectory));

		// vfsLookup only sends us pathnames, so all components are directories
		debug_assert(isDirectory);

		if((m_flags & VFS_LOOKUP_CREATE) != 0)
			m_currentDirectory->AddSubdirectory(component);

		m_currentDirectory = m_currentDirectory->GetSubdirectory(component);
		if(!m_currentDirectory)
			WARN_RETURN(ERR::VFS_DIR_NOT_FOUND);

		return INFO::CB_CONTINUE;
	}

	VfsDirectory* Directory() const
	{
		return m_currentDirectory;
	}

private:
	unsigned m_flags;
	mutable VfsDirectory* m_currentDirectory;
};


LibError vfs_Lookup(const char* vfsPathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile*& file, unsigned flags)
{
	const char* vfsPath; const char* name;
	path_split(vfsPathname, &vfsPath, &name);

	// optimization: looking up each full path is rather slow, so we
	// cache the previous directory and use it if the path string
	// addresses match.
	static const char* vfsPreviousPath;
	static VfsDirectory* previousDirectory;
	if(vfsPath == vfsPreviousPath)
		directory = previousDirectory;
	else
	{
		PathResolver pathResolver(startDirectory, flags);
		RETURN_ERR(path_foreach_component(vfsPathname, PathResolver::Callback, (uintptr_t)&pathResolver));
		directory = pathResolver.Directory();
	}
	previousDirectory = directory;

	if(name[0] == '\0')
		file = 0;
	else
	{
		file = directory->GetFile(name);
		if(!file)
			WARN_RETURN(ERR::VFS_FILE_NOT_FOUND);
	}

	return INFO::OK;
}


VfsFile* vfs_LookupFile(const char* vfsPathname, VfsDirectory* startDirectory, unsigned flags)
{
	debug_assert(!path_IsDirectory(vfsPathname));

	VfsDirectory* directory; VfsFile* file;
	if(vfs_Lookup(vfsPathname, startDirectory, directory, file, flags) != INFO::OK)
		return 0;
	return file;
}


VfsDirectory* vfs_LookupDirectory(const char* vfsPath, VfsDirectory* startDirectory, unsigned flags)
{
	debug_assert(path_IsDirectory(vfsPath));

	VfsDirectory* directory; VfsFile* file;
	if(vfs_Lookup(vfsPath, startDirectory, directory, file, flags) != INFO::OK)
		return 0;
	return directory;
}
