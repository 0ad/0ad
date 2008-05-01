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

#include "lib/allocators/shared_ptr.h"
#include "lib/path_util.h"
#include "lib/file/common/file_stats.h"
#include "lib/file/common/trace.h"
#include "lib/file/archive/archive.h"
#include "lib/file/io/io.h"
#include "vfs_tree.h"
#include "vfs_lookup.h"
#include "file_cache.h"


class VFS : public IVFS
{
public:
	VFS()
		: m_fileCache(ChooseCacheSize())
		, m_trace(CreateTrace(4*MiB))
	{
	}

	virtual LibError Mount(const VfsPath& mountPoint, const Path& path, uint flags /* = 0 */, uint priority /* = 0 */)
	{
		debug_assert(vfs_path_IsDirectory(mountPoint));
		// note: mounting subdirectories is now allowed.

		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(mountPoint, &m_rootDirectory, directory, 0, VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE));
		PRealDirectory realDirectory(new RealDirectory(path, priority, flags));
		directory->Attach(realDirectory);
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

	virtual LibError GetDirectoryEntries(const VfsPath& path, FileInfos* files, DirectoryNames* subdirectoryNames) const
	{
		debug_assert(vfs_path_IsDirectory(path));
		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(path, &m_rootDirectory, directory, 0));
		directory->GetEntries(files, subdirectoryNames);
		return INFO::OK;
	}

	// note: only allowing either reads or writes simplifies file cache
	// coherency (need only invalidate when closing a FILE_WRITE file).
	virtual LibError CreateFile(const VfsPath& pathname, shared_ptr<u8> fileContents, size_t size)
	{
		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(pathname, &m_rootDirectory, directory, 0, VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE));

		PRealDirectory realDirectory = directory->AssociatedDirectory();
		const std::string& name = pathname.leaf();
		RETURN_ERR(realDirectory->Store(name, fileContents, size));

		const VfsFile file(name, (off_t)size, time(0), realDirectory->Priority(), realDirectory);
		directory->AddFile(file);

		// wipe out any cached blocks. this is necessary to cover the (rare) case
		// of file cache contents predating the file write.
		m_fileCache.Remove(pathname);

		m_trace->NotifyStore(pathname.string().c_str(), size);
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
			else if(size > ChooseCacheSize())
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

		// rebuild the VFS, i.e. re-mount everything. open files are not affected.
		// necessary after loose files or directories change, so that the VFS
		// "notices" the changes and updates file locations. res calls this after
		// dir_watch reports changes; can also be called from the console after a
		// rebuild command. there is no provision for updating single VFS dirs -
		// it's not worth the trouble.
	virtual void Clear()
	{
		m_rootDirectory.ClearR();
	}

	virtual void Display() const
	{
		m_rootDirectory.DisplayR(0);
	}

	virtual LibError GetRealPath(const VfsPath& pathname, Path& realPathname)
	{
		VfsDirectory* directory;
		CHECK_ERR(vfs_Lookup(pathname, &m_rootDirectory, directory, 0));
		PRealDirectory realDirectory = directory->AssociatedDirectory();
		realPathname = realDirectory->GetPath() / pathname.leaf();
		return INFO::OK;
	}

private:
	static size_t ChooseCacheSize()
	{
		return 96*MiB;
	}

	mutable VfsDirectory m_rootDirectory;
	FileCache m_fileCache;
	PITrace m_trace;
};

//-----------------------------------------------------------------------------

PIVFS CreateVfs()
{
	return PIVFS(new VFS);
}
