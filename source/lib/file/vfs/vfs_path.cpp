/**
 * =========================================================================
 * File        : vfs_path.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_path.h"

#include "lib/path_util.h"	// path_foreach_component
#include "vfs_tree.h"


static bool IsVfsPath(const char* path)
{
	if(path[0] == '\0')	// root dir
		return true;
	if(path[strlen(path)-1] == '/')	// ends in slash => directory path
		return true;
	return false;
}



class PathResolver
{
public:
	enum Flags
	{
		// when encountering subdirectory components in the path(name) that
		// don't (yet) exist, add them.
		CreateMissingDirectories = 1,
	};

	PathResolver(VfsDirectory* startDirectory, uint flags = 0)
		: m_currentDirectory(startDirectory), m_file(0)
		, m_createMissingDirectories((flags & CreateMissingDirectories) != 0)
	{
	}

	LibError Next(const char* component, bool isDir) const
	{
		m_currentDirectory->Populate();

		if(isDir)
		{
			if(m_createMissingDirectories)
				m_currentDirectory->AddSubdirectory(component);

			m_currentDirectory = m_currentDirectory->GetSubdirectory(component);
			if(!m_currentDirectory)
				WARN_RETURN(ERR::FAIL);
		}
		else
		{
			debug_assert(m_file == 0);	// can't have encountered any files yet
			m_file = m_currentDirectory->GetFile(component);
			if(!m_file)
				WARN_RETURN(ERR::FAIL);
		}

		return INFO::CB_CONTINUE;
	}

	VfsDirectory* Directory() const
	{
		return m_currentDirectory;
	}

	VfsFile* File() const
	{
		return m_file;
	}

private:
	mutable VfsDirectory* m_currentDirectory;
	mutable VfsFile* m_file;
	bool m_createMissingDirectories;
};


static LibError PathResolverCallback(const char* component, bool isDir, uintptr_t cbData)
{
	PathResolver* pathResolver = (PathResolver*)cbData;
	return pathResolver->Next(component, isDir);
}


VfsFile* LookupFile(const char* vfsPathname, VfsDirectory* startDirectory)
{
	debug_assert(!IsVfsPath(vfsPathname));

	PathResolver pathResolver(startDirectory);
	if(path_foreach_component(vfsPathname, PathResolverCallback, (uintptr_t)&pathResolver) != INFO::OK)
		return 0;
	return pathResolver.File();
}


VfsDirectory* LookupDirectory(const char* vfsPath, VfsDirectory* startDirectory)
{
	debug_assert(IsVfsPath(vfsPath));

	PathResolver pathResolver(startDirectory);
	if(path_foreach_component(vfsPath, PathResolverCallback, (uintptr_t)&pathResolver) != INFO::OK)
		return 0;
	return pathResolver.Directory();
}


void TraverseAndCreate(const char* vfsPath, VfsDirectory* startDirectory, VfsDirectory*& lastDirectory, VfsFile*& file)
{
	PathResolver pathResolver(startDirectory, PathResolver::CreateMissingDirectories);
	LibError ret = path_foreach_component(vfsPath, PathResolverCallback, (uintptr_t)&pathResolver);
	debug_assert(ret == INFO::OK);	// should never fail
	lastDirectory = pathResolver.Directory();
	file = pathResolver.File();
}
