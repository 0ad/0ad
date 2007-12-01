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
#include "lib/file/trace.h"
#include "lib/file/archive/archive.h"
#include "lib/file/posix/fs_posix.h"
#include "vfs_tree.h"
#include "vfs_lookup.h"
#include "vfs_populate.h"
#include "file_cache.h"


PIFileProvider provider;

class Filesystem_VFS::Impl : public IFilesystem
{
public:
	Impl()
		: m_fileCache(ChooseCacheSize()), m_trace(CreateTrace(4*MiB))
	{
	}

	LibError Mount(const char* vfsPath, const char* path, uint flags /* = 0 */, uint priority /* = 0 */)
	{
		// make sure caller didn't forget the required trailing '/'.
		debug_assert(path_IsDirectory(vfsPath));

		// note: we no longer need to check if mounting a subdirectory -
		// the new RealDirectory scheme doesn't care.

		// disallow "." because "./" isn't supported on Windows.
		// "./" and "/." are caught by CHECK_PATH.
		if(!strcmp(path, "."))
			WARN_RETURN(ERR::PATH_NON_CANONICAL);

		VfsDirectory* directory = vfs_LookupDirectory(vfsPath, &m_rootDirectory, VFS_LOOKUP_CREATE);
		directory->Attach(RealDirectory(path, priority, flags));
		return INFO::OK;
	}

	virtual LibError GetFileInfo(const char* vfsPathname, FileInfo& fileInfo) const
	{
		VfsFile* file = vfs_LookupFile(vfsPathname, &m_rootDirectory);
		if(!file)
			return ERR::VFS_FILE_NOT_FOUND;	// NOWARN
		file->GetFileInfo(fileInfo);
		return INFO::OK;
	}

	virtual LibError GetDirectoryEntries(const char* vfsPath, FileInfos* files, Directories* subdirectories) const
	{
		VfsDirectory* directory = vfs_LookupDirectory(vfsPath, &m_rootDirectory);
		if(!directory)
			WARN_RETURN(ERR::VFS_DIR_NOT_FOUND);
		directory->GetEntries(files, subdirectories);
		return INFO::OK;
	}

	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	LibError CreateFile(const char* vfsPathname, const u8* buf, size_t size)
	{
		VfsDirectory* directory = vfs_LookupDirectory(vfsPathname, &m_rootDirectory);
		if(!directory)
			WARN_RETURN(ERR::VFS_DIR_NOT_FOUND);
		const char* name = path_name_only(vfsPathname);

		const RealDirectory& realDirectory = directory->AttachedDirectories().back();
		const char* location = realDirectory.Path();
		const VfsFile file(FileInfo(name, (off_t)size, time(0)), realDirectory.Priority(), provider, location);
		file.Store(buf, size);
		directory->AddFile(file);
		
		// wipe out any cached blocks. this is necessary to cover the (rare) case
		// of file cache contents predating the file write.
		m_fileCache.Remove(vfsPathname);

		m_trace.get()->NotifyStore(vfsPathname, size);
		return INFO::OK;
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
			VfsFile* file = vfs_LookupFile(vfsPathname, &m_rootDirectory);
			if(!file)
				WARN_RETURN(ERR::VFS_FILE_NOT_FOUND);
			contents = m_fileCache.Reserve(file->Size());
			RETURN_ERR(file->Load((u8*)contents.get()));
			m_fileCache.Add(vfsPathname, contents, size);
		}

		stats_io_user_request(size);
		stats_cache(isCacheHit? CR_HIT : CR_MISS, size, vfsPathname);
		m_trace.get()->NotifyLoad(vfsPathname, size);

		return INFO::OK;
	}

	void RefreshFileInfo(const char* pathname)
	{
		//VfsFile* file = LookupFile(vfsPathname, &m_rootDirectory);
	}

	// "backs off of" all archives - closes their files and allows them to
	// be rewritten or deleted (required by archive builder).
	// must call mount_rebuild when done with the rewrite/deletes,
	// because this call leaves the VFS in limbo!!
	void ReleaseArchives()
	{
	}

		// rebuild the VFS, i.e. re-mount everything. open files are not affected.
		// necessary after loose files or directories change, so that the VFS
		// "notices" the changes and updates file locations. res calls this after
		// dir_watch reports changes; can also be called from the console after a
		// rebuild command. there is no provision for updating single VFS dirs -
		// it's not worth the trouble.
	void Clear()
	{
		m_rootDirectory.ClearR();
	}

	void Display()
	{
		m_rootDirectory.DisplayR(0);
	}


private:
	static size_t ChooseCacheSize()
	{
		return 96*MiB;
	}

	VfsDirectory m_rootDirectory;
	FileCache m_fileCache;
	PITrace m_trace;
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

/*virtual*/ LibError Filesystem_VFS::GetDirectoryEntries(const char* vfsPath, FileInfos* files, Directories* subdirectories) const
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

void Filesystem_VFS::RefreshFileInfo(const char* pathname)
{
	impl.get()->RefreshFileInfo(pathname);
}

void Filesystem_VFS::Display() const
{
	impl.get()->Display();
}

void Filesystem_VFS::Clear()
{
	impl.get()->Clear();
}
