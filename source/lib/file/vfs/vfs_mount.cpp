/**
 * =========================================================================
 * File        : vfs_mount.cpp
 * Project     : 0 A.D.
 * Description : mounts files and archives into VFS
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_mount.h"

#include "lib/file/archive/archive_zip.h"


//-----------------------------------------------------------------------------
// ArchiveEnumerator
//-----------------------------------------------------------------------------

class ArchiveEnumerator
{
public:
	ArchiveEnumerator(VfsDirectory* directory)
		: m_startDirectory(directory), m_previousPath(0), m_previousDirectory(0)
	{
	}

	LibError Next(const char* pathname, const ArchiveEntry& archiveEntry)
	{
		vfs_opt_notify_archived_file(pathname);

		const char* name = path_name_only(pathname);
		const char* path = path_dir_only2(pathname);

		// into which directory should the file be inserted?
		VfsDirectory* directory = m_previousDirectory;
		if(path != m_previousPath)
		{
			// we have to create them if missing, since we can't rely on the
			// archiver placing directories before subdirs or files that
			// reference them (WinZip doesn't always).

			VfsFile* file;
			TraverseAndCreate(path, m_startDirectory, directory, file);
			debug_assert(file == 0);	// should not be a file on the path

			m_previousPath = path;
			m_previousDirectory = directory;
		}

		FileInfo fileInfo;
		fileInfo.name = name;
		fileInfo.size = archiveEntry.usize;
		fileInfo.mtime = archiveEntry.mtime;
		directory->AddFile(name, fileInfo);

		return INFO::CB_CONTINUE;
	}

private:
	VfsDirectory* m_startDirectory;

	// optimization: looking up each full path is rather slow, so we
	// cache the previous directory and use it if the path string
	// addresses match.
	const char* m_previousPath;
	VfsDirectory* m_previousDirectory;
};

static LibError ArchiveEnumeratorCallback(const char* pathname, const ArchiveEntry& archiveEntry, uintptr_t cbData)
{
	ArchiveEnumerator* archiveEnumerator = (ArchiveEnumerator*)cbData;
	return ArchiveEnumerator->Next(pathname, archiveEntry);
}


//-----------------------------------------------------------------------------
// Mount
//-----------------------------------------------------------------------------

// not many instances => don't worry about efficiency.
// note: only check for archives in the root directory of the mounting
// because doing so for every single file would entail serious overhead
class Mount
{
	Mount(const char* vfsPath, const char* path, uint flags, uint priority)
		: m_vfsPath(vfsPath), m_path(path), m_flags(flags), m_priority(priority)
	{
	}

	const char* VfsPath() const
	{
		return m_vfsPath;
	}

	const char* Path() const
	{
		return m_path;
	}

	// actually mount the specified entry. split out of vfs_mount,
	// because when invalidating (reloading) the VFS, we need to
	// be able to mount without changing the mount list.
	LibError Mount(VfsDirectory* rootDirectory)
	{
		VfsDirectory* directory; VfsFile* file;
		TraverseAndCreate(m_vfsPath, rootDirectory, directory, file);
		debug_assert(file == 0);	// should not be a file on the path

		directory->AssociateWithRealDirectory(m_path);

		// add archives
		directory->Populate();
		std::vector<FileInfo> files;
		directory->GetEntries(&files, 0);
		for(size_t i = 0; i < files.size(); i++)
		{
			FileInfo& fileInfo = files[i];
			const char* pathname = path_append2(m_path, fileInfo.Name());
			const char* extension = path_extension(fileInfo.Name());
			boost::shared_ptr<IArchiveReader> archiveReader;
			if(strcasecmp(extension, "zip") == 0)
				archiveReader = CreateArchiveReader_Zip(pathname);
			else
				continue;	// not a (supported) archive file

			ArchiveEnumerator archiveEnumerator(directory);
			RETURN_ERR(archiveReader->ReadEntries(ArchiveEnumeratorCallback, (uintptr_t)&archiveEnumerator));
			m_archiveReaders.push_back(archiveReader);
		}
	}

	// "backs off of" all archives - closes their files and allows them to
	// be rewritten or deleted (required by archive builder).
	// must call mount_rebuild when done with the rewrite/deletes,
	// because this call leaves the VFS in limbo!!
	void ReleaseArchives()
	{
		m_archiveReaders.clear();
	}

private:
	bool IsArchivable() const
	{
		return (m_flags & MOUNT_ARCHIVABLE) != 0;
	}

	// mounting into this VFS directory;
	// must end in '/' (unless if root td, i.e. "")
	const char* m_vfsPath;

	const char* m_path;

	std::vector<boost::shared_ptr<IArchiveReader> > m_archiveReaders;

	uint m_flags;	// MountFlags

	uint m_priority;
};


//-----------------------------------------------------------------------------
// MountManager
//-----------------------------------------------------------------------------


class MountManager
{
public:
	// mount <P_real_path> into the VFS at <V_mount_point>,
	//   which is created if it does not yet exist.
	// files in that directory override the previous VFS contents if
	//   <pri>(ority) is not lower.
	// all archives in <P_real_path> are also mounted, in alphabetical order.
	//
	// flags determines extra actions to perform; see VfsMountFlags.
	//
	// P_real_path = "." or "./" isn't allowed - see implementation for rationale.
	LibError Add(const char* vfsPath, const char* path, uint flags, uint priority)
	{
		// make sure caller didn't forget the required trailing '/'.
		debug_assert(VFS_PATH_IS_DIR(vfsPath));

		// make sure it's not already mounted, i.e. in mounts.
		// also prevents mounting a parent directory of a previously mounted
		// directory, or vice versa. example: mount $install/data and then
		// $install/data/mods/official - mods/official would also be accessible
		// from the first mount point - bad.
		char existingVfsPath[PATH_MAX];
		if(GetVfsPath(path, existingVfsPath))
			WARN_RETURN(ERR::ALREADY_MOUNTED);

		// disallow "." because "./" isn't supported on Windows.
		// "./" and "/." are caught by CHECK_PATH.
		if(!strcmp(path, "."))
			WARN_RETURN(ERR::PATH_NON_CANONICAL);

		// (count this as "init" to obviate a separate timer)
		stats_vfs_init_start();
		mounts[path] = Mount(vfsPath, path, flags, priority);
		LibError ret = remount(m);
		stats_vfs_init_finish();
		return ret;
	}

	void Remove(const char* path)
	{
		const size_t numRemoved = mounts.erase(path);
		debug_assert(numRemoved == 1);
	}

	// rebuild the VFS, i.e. re-mount everything. open files are not affected.
	// necessary after loose files or directories change, so that the VFS
	// "notices" the changes and updates file locations. res calls this after
	// dir_watch reports changes; can also be called from the console after a
	// rebuild command. there is no provision for updating single VFS dirs -
	// it's not worth the trouble.
	void RedoAll()
	{
		for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		{
			const Mount& mount = it->second;
			mount.Apply();
		}
	}

	// if <path> or its ancestors are mounted,
	// return a VFS path that accesses it.
	// used when receiving paths from external code.
	bool GetVfsPath(const char* path, char* vfsPath) const
	{
		for(MountIt it = mounts.begin(); it != mounts.end(); ++it)
		{
			const Mount& mount = it->second;

			const char* remove = mount.Path();
			const char* replace = mount.VfsPath();
			if(path_replace(vfsPath, path, remove, replace) == INFO::OK)
				return true;
		}

		return false;
	}

	void ReleaseArchives()
	{
		// "backs off of" all archives - closes their files and allows them to
		// be rewritten or deleted (required by archive builder).
		// must call mount_rebuild when done with the rewrite/deletes,
		// because this call leaves the VFS in limbo!!

	}

private:
	typedef std::map<const char*, Mount> Mounts;
	typedef Mounts::iterator MountIt;
	Mounts mounts;
};



// 2006-05-09 JW note: we are wanting to move XMB files into a separate
// folder tree (no longer interspersed with XML), so that deleting them is
// easier and dirs are less cluttered.
//
// if several mods are active, VFS would have several OS directories mounted
// into a VfsDirectory and could no longer automatically determine the write target.
//
// one solution would be to use a set_write_target API to choose the
// correct dir; however, XMB files may be generated whilst editing
// (which also requires a write_target to write files that are actually
// currently in archives), so we'd need to save/restore write_target.
// this would't be thread-safe => disaster.
//
// a vfs_store_to(filename, flags, N_actual_path) API would work, but it'd
// impose significant burden on users (finding the actual native dir),
// and be prone to abuse. additionally, it would be difficult to
// propagate N_actual_path to VFile_reload where it is needed;
// this would end up messy.
//
// instead, we'll write XMB files into VFS path "mods/$MODNAME/..",
// into which the realdir of the same name (located in some writable folder)
// is mounted; VFS therefore can write without problems.
//
// however, other code (e.g. archive builder) doesn't know about this
// trick - it only sees the flat VFS namespace, which doesn't
// include mods/$MODNAME (that is hidden). to solve this, we also mount
// any active mod's XMB dir into VFS root for read access.
//
// write_targets are no longer necessary since files and directories are
// directly associated with a real directory; when writing files, we can
// do so directly.
