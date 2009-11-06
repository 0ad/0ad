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

#include "precompiled.h"
#include "vfs.h"

#include "lib/allocators/shared_ptr.h"
#include "lib/path_util.h"
#include "lib/file/common/file_stats.h"
#include "lib/file/common/trace.h"
#include "lib/file/archive/archive.h"
#include "lib/file/io/io.h"
#include "vfs_tree.h"
#include "vfs_lookup.h"
#include "vfs_populate.h"
#include "file_cache.h"


class VFS : public IVFS
{
public:
	VFS(size_t cacheSize)
		: m_cacheSize(cacheSize), m_fileCache(m_cacheSize)
		, m_trace(CreateTrace(4*MiB))
	{
	}

	virtual LibError Mount(const VfsPath& mountPoint, const fs::wpath& path, size_t flags /* = 0 */, size_t priority /* = 0 */)
	{
		RETURN_ERR(CreateDirectories(path, 0700));

		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(mountPoint, &m_rootDirectory, directory, 0, VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE));
		PRealDirectory realDirectory(new RealDirectory(path, priority, flags));
		RETURN_ERR(vfs_Attach(directory, realDirectory));
		return INFO::OK;
	}

	virtual LibError GetFileInfo(const VfsPath& pathname, FileInfo* pfileInfo) const
	{
		VfsDirectory* directory; VfsFile* file;
		LibError ret = vfs_Lookup(pathname, &m_rootDirectory, directory, &file);
		if(!pfileInfo)	// just indicate if the file exists without raising warnings.
			return ret;
		CHECK_ERR(ret);
		*pfileInfo = FileInfo(file->Name(), file->Size(), file->MTime());
		return INFO::OK;
	}

	virtual LibError GetDirectoryEntries(const VfsPath& path, FileInfos* fileInfos, DirectoryNames* subdirectoryNames) const
	{
		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(path, &m_rootDirectory, directory, 0));

		if(fileInfos)
		{
			const VfsDirectory::VfsFiles& files = directory->Files();
			fileInfos->clear();
			fileInfos->reserve(files.size());
			for(VfsDirectory::VfsFiles::const_iterator it = files.begin(); it != files.end(); ++it)
			{
				const VfsFile& file = it->second;
				fileInfos->push_back(FileInfo(file.Name(), file.Size(), file.MTime()));
			}
		}

		if(subdirectoryNames)
		{
			const VfsDirectory::VfsSubdirectories& subdirectories = directory->Subdirectories();
			subdirectoryNames->clear();
			subdirectoryNames->reserve(subdirectories.size());
			for(VfsDirectory::VfsSubdirectories::const_iterator it = subdirectories.begin(); it != subdirectories.end(); ++it)
				subdirectoryNames->push_back(it->first);
		}

		return INFO::OK;
	}

	virtual LibError CreateFile(const VfsPath& pathname, const shared_ptr<u8>& fileContents, size_t size)
	{
		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(pathname, &m_rootDirectory, directory, 0, VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE));

		const PRealDirectory& realDirectory = directory->AssociatedDirectory();
		const std::wstring& name = pathname.leaf();
		RETURN_ERR(realDirectory->Store(name, fileContents, size));

		// wipe out any cached blocks. this is necessary to cover the (rare) case
		// of file cache contents predating the file write.
		m_fileCache.Remove(pathname);

		const VfsFile file(name, size, time(0), realDirectory->Priority(), realDirectory);
		directory->AddFile(file);

		m_trace->NotifyStore(pathname.string().c_str(), size);
		return INFO::OK;
	}

	virtual LibError LoadFile(const VfsPath& pathname, shared_ptr<u8>& fileContents, size_t& size)
	{
		const bool isCacheHit = m_fileCache.Retrieve(pathname, fileContents, size);
		if(!isCacheHit)
		{
			VfsDirectory* directory; VfsFile* file;
			CHECK_ERR(vfs_Lookup(pathname, &m_rootDirectory, directory, &file));

			size = file->Size();
			// safely handle zero-length files
			if(!size)
				fileContents = DummySharedPtr((u8*)0);
			else if(size > m_cacheSize)
			{
				fileContents = io_Allocate(size);
				RETURN_ERR(file->Load(fileContents));
			}
			else
			{
				fileContents = m_fileCache.Reserve(size);
				RETURN_ERR(file->Load(fileContents));
				m_fileCache.Add(pathname, fileContents, size);
			}
		}

		stats_io_user_request(size);
		stats_cache(isCacheHit? CR_HIT : CR_MISS, size);
		m_trace->NotifyLoad(pathname.string().c_str(), size);

		return INFO::OK;
	}

	virtual std::wstring TextRepresentation() const
	{
		std::wstring textRepresentation;
		textRepresentation.reserve(100*KiB);
		DirectoryDescriptionR(textRepresentation, m_rootDirectory, 0);
		return textRepresentation;
	}

	virtual LibError GetRealPath(const VfsPath& pathname, fs::wpath& realPathname)
	{
		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(pathname, &m_rootDirectory, directory, 0));
		realPathname = directory->AssociatedDirectory()->Path() / pathname.leaf();
		return INFO::OK;
	}

	virtual LibError GetVirtualPath(const fs::wpath& realPathname, VfsPath& pathname)
	{
		const fs::wpath realPath = AddSlash(realPathname.branch_path());
		VfsPath path;
		RETURN_ERR(FindRealPathR(realPath, m_rootDirectory, L"", path));
		pathname = path / realPathname.leaf();
		return INFO::OK;
	}

	virtual LibError Invalidate(const VfsPath& pathname)
	{
		m_fileCache.Remove(pathname);

		VfsDirectory* directory;
		RETURN_ERR(vfs_Lookup(pathname, &m_rootDirectory, directory, 0));
		const std::wstring name = pathname.leaf();
		directory->Invalidate(name);

		return INFO::OK;
	}

	virtual void Clear()
	{
		m_rootDirectory.Clear();
	}

private:
	LibError FindRealPathR(const fs::wpath& realPath, const VfsDirectory& directory, const VfsPath& curPath, VfsPath& path)
	{
		PRealDirectory realDirectory = directory.AssociatedDirectory();
		if(realDirectory && realDirectory->Path() == realPath)
		{
			path = curPath;
			return INFO::OK;
		}

		const VfsDirectory::VfsSubdirectories& subdirectories = directory.Subdirectories();
		for(VfsDirectory::VfsSubdirectories::const_iterator it = subdirectories.begin(); it != subdirectories.end(); ++it)
		{
			const std::wstring& subdirectoryName = it->first;
			const VfsDirectory& subdirectory = it->second;
			LibError ret = FindRealPathR(realPath, subdirectory, AddSlash(curPath/subdirectoryName), path);
			if(ret == INFO::OK)
				return INFO::OK;
		}

		return ERR::PATH_NOT_FOUND;	// NOWARN
	}

	size_t m_cacheSize;
	FileCache m_fileCache;
	PITrace m_trace;
	mutable VfsDirectory m_rootDirectory;
};

//-----------------------------------------------------------------------------

PIVFS CreateVfs(size_t cacheSize)
{
	return PIVFS(new VFS(cacheSize));
}
