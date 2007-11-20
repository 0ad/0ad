/**
 * =========================================================================
 * File        : vfs.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs.h"

#include "lib/path_util.h"
#include "lib/file/file_stats.h"
#include "file_cache.h"
#include "vfs_tree.h"
#include "vfs_path.h"
#include "vfs_mount.h"


class Filesystem_VFS::Impl : public IFilesystem
{
public:
	Impl()
		: m_fileCache(ChooseCacheSize())
	{
	}

	~Impl()
	{
		trace_shutdown();
		mount_shutdown();
	}

	LibError Mount(const char* vfsPath, const char* path, uint flags /* = 0 */, uint priority /* = 0 */)
	{

	}

	void Unmount(const char* path)
	{

	}

	virtual LibError GetFileInfo(const char* vfsPathname, FileInfo& fileInfo) const
	{
		VfsFile* file = LookupFile(vfsPathname, &m_tree.Root());
		if(!file)
			return ERR::VFS_FILE_NOT_FOUND;	// NOWARN
		file->GetFileInfo(fileInfo);
		return INFO::OK;
	}

	virtual LibError GetDirectoryEntries(const char* vfsPath, std::vector<FileInfo>* files, std::vector<const char*>* subdirectories) const
	{
		VfsDirectory* directory = LookupDirectory(vfsPath, &m_tree.Root());
		if(!directory)
			WARN_RETURN(ERR::VFS_DIR_NOT_FOUND);
		directory->GetEntries(files, subdirectories);
		return INFO::OK;
	}

	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	LibError CreateFile(const char* vfsPathname, const u8* buf, size_t size)
	{
		VfsDirectory* directory = LookupDirectory(vfsPathname, &m_tree.Root());
		if(!directory)
			WARN_RETURN(ERR::VFS_DIR_NOT_FOUND);
		const char* name = path_name_only(vfsPathname);
		directory->CreateFile(name, buf, size);

		// wipe out any cached blocks. this is necessary to cover the (rare) case
		// of file cache contents predating the file write.
		m_fileCache.Remove(vfsPathname);
	}

	// read the entire file.
	// return number of bytes transferred (see above), or a negative error code.
	//
	// if non-NULL, <cb> is called for each block transferred, passing <cbData>.
	// it returns how much data was actually transferred, or a negative error
	// code (in which case we abort the transfer and return that value).
	// the callback mechanism is useful for user progress notification or
	// processing data while waiting for the next I/O to complete
	// (quasi-parallel, without the complexity of threads).
	LibError LoadFile(const char* vfsPathname, FileContents& contents, size_t& size)
	{
		vfsPathname = path_Pool()->UniqueCopy(vfsPathname);
		debug_printf("VFS| load %s\n", vfsPathname);

		const bool isCacheHit = m_fileCache.Retrieve(vfsPathname, contents, size);
		if(!isCacheHit)
		{
			VfsFile* file = LookupFile(vfsPathname, &m_tree.Root());
			if(!file)
				WARN_RETURN(ERR::VFS_FILE_NOT_FOUND);
			contents = m_fileCache.Reserve(vfsPathname, file->Size());
			RETURN_ERR(file->Load((u8*)contents.get()));
			m_fileCache.Add(vfsPathname, contents, size);
		}

		stats_io_user_request(size);
		stats_cache(isCacheHit? CR_HIT : CR_MISS, size, vfsPathname);
		trace_notify_io(vfsPathname, size);

		return INFO::OK;
	}

	void RefreshFileInfo(const char* pathname)
	{
		//VfsFile* file = LookupFile(vfsPathname, &m_tree.Root());
	}

	void Display()
	{
		m_tree.Display();
	}

private:
	static size_t ChooseCacheSize()
	{
		return 96*MiB;
	}

	void Rebuild()
	{
		m_tree.Clear();
		m_mounts.RedoAll();
	}

	VfsTree m_tree;
	FileCache m_fileCache;
};

//-----------------------------------------------------------------------------

Filesystem_VFS::Filesystem_VFS(void* trace)
{
}

/*virtual*/ Filesystem_VFS::~Filesystem_VFS()
{
}

/*virtual*/ LibError Filesystem_VFS::GetFileInfo(const char* vfsPathname, FileInfo& fileInfo) const
{
	return impl.get()->GetFileInfo(vfsPathname, fileInfo);
}

/*virtual*/ LibError Filesystem_VFS::GetDirectoryEntries(const char* vfsPath, std::vector<FileInfo>* files, std::vector<const char*>* subdirectories) const
{
	return impl.get()->GetDirectoryEntries(vfsPath, files, subdirectories);
}

LibError Filesystem_VFS::CreateFile(const char* vfsPathname, const u8* data, size_t size)
{
	return impl.get()->CreateFile(vfsPathname, data, size);
}

LibError Filesystem_VFS::LoadFile(const char* vfsPathname, FileContents& contents, size_t& size)
{
	return impl.get()->LoadFile(vfsPathname, contents, size);
}

LibError Filesystem_VFS::Mount(const char* vfsPath, const char* path, uint flags, uint priority)
{
	return impl.get()->Mount(vfsPath, path, flags, priority);
}

void Filesystem_VFS::Unmount(const char* path)
{
	return impl.get()->Unmount(path);
}

void Filesystem_VFS::RefreshFileInfo(const char* pathname)
{
	impl.get()->RefreshFileInfo(pathname);
}

void Filesystem_VFS::Display() const
{
	impl.get()->Display();
}
