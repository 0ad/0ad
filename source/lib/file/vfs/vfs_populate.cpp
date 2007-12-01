/**
 * =========================================================================
 * File        : vfs_populate.cpp
 * Project     : 0 A.D.
 * Description : 
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_populate.h"

#include "lib/path_util.h"
#include "lib/file/path.h"
#include "lib/file/archive/archive_zip.h"
#include "lib/file/posix/fs_posix.h"
#include "vfs_tree.h"
#include "vfs_lookup.h"
#include "vfs.h"	// mount flags

typedef void* PIWatch;


static Filesystem_Posix s_fsPosix;

// since directories are never removed except when rebuilding the VFS,
// we can store the IWatch references here.
static std::vector<PIWatch> s_watches;

static std::vector<PIArchiveReader> s_archiveReaders;

static std::vector<const VfsFile*> s_looseFiles;
static size_t s_numArchivedFiles;


//-----------------------------------------------------------------------------

// helper class that allows breaking up the logic into sub-functions without
// always having to pass directory/realDirectory as parameters.
class PopulateHelper : boost::noncopyable
{
public:
	PopulateHelper(VfsDirectory* directory, const RealDirectory& realDirectory)
		: m_directory(directory)
		, m_path(realDirectory.Path()), m_priority(realDirectory.Priority()), m_flags(realDirectory.Flags())
	{
	}

	LibError AddEntries() const
	{
		FileInfos files; files.reserve(100);
		Directories subdirectories; subdirectories.reserve(20);
		RETURN_ERR(s_fsPosix.GetDirectoryEntries(m_path, &files, &subdirectories));
		RETURN_ERR(AddFiles(files));
		AddSubdirectories(subdirectories);
		return INFO::OK;
	}

private:
	void AddFile(const FileInfo& fileInfo) const
	{
		static PIFileProvider provider(new FileProvider_Posix);
		const VfsFile* file = m_directory->AddFile(VfsFile(fileInfo, m_priority, provider, m_path));

		// notify archive builder that this file could be archived but
		// currently isn't; if there are too many of these, archive will
		// be rebuilt.
		// note: check if archivable to exclude stuff like screenshots
		// from counting towards the threshold.
		if(m_flags & VFS_ARCHIVABLE)
			s_looseFiles.push_back(file);
	}

	static void AddArchiveFile(const char* pathname, const FileInfo& fileInfo, const ArchiveEntry* archiveEntry, uintptr_t cbData)
	{
		PopulateHelper* this_ = (PopulateHelper*)cbData;

		// (we have to create missing subdirectories because archivers
		// don't always place directory entries before their files)
		const unsigned flags = VFS_LOOKUP_CREATE|VFS_LOOKUP_NO_POPULATE;
		VfsDirectory* directory = vfs_LookupDirectory(pathname, this_->m_directory, flags);
		debug_assert(directory);

		directory->AddFile(VfsFile(fileInfo, this_->m_priority, this_->m_archiveReader, archiveEntry));
		s_numArchivedFiles++;
	}

	LibError AddFiles(const FileInfos& files) const
	{
		PathPackage pp;
		path_package_set_dir(&pp, m_path);

		for(size_t i = 0; i < files.size(); i++)
		{
			const char* name = files[i].Name();
			path_package_append_file(&pp, name);

			PIArchiveReader archiveReader;
			const char* extension = path_extension(name);
			if(strcasecmp(extension, "zip") == 0)
				archiveReader = CreateArchiveReader_Zip(pp.path);
			else	// not a (supported) archive file
				AddFile(files[i]);

			RETURN_ERR(archiveReader->ReadEntries(AddArchiveFile, (uintptr_t)this));
			s_archiveReaders.push_back(archiveReader);
		}

		return INFO::OK;
	}

	void AddSubdirectories(const Directories& subdirectories) const
	{
		for(size_t i = 0; i < subdirectories.size(); i++)
		{
			const char* name = subdirectories[i];

			// skip version control directories - this avoids cluttering the
			// VFS with hundreds of irrelevant files.
			static const char* const svnName = path_Pool()->UniqueCopy(".svn");
			if(name == svnName)
				continue;

			VfsDirectory* subdirectory = m_directory->AddSubdirectory(name);

			const char* subdirectoryPath = path_append2(m_path, name);
			const RealDirectory realSubdirectory(subdirectoryPath, m_priority, m_flags);
			subdirectory->Attach(realSubdirectory);
		}
	}

	VfsDirectory* const m_directory;

	const char* m_path;
	const unsigned m_priority;
	const unsigned m_flags;

	// used when populating archives
	PIArchiveReader m_archiveReader;
};


//-----------------------------------------------------------------------------

LibError vfs_Populate(VfsDirectory* directory)
{
	const std::vector<RealDirectory>& realDirectories = directory->AttachedDirectories();
	for(size_t i = 0; i < realDirectories.size(); i++)
	{
		const RealDirectory& realDirectory = realDirectories[i];

		// note: we need to do this in each directory because some watch APIs
		// (e.g. FAM) cannot register entire directory trees with one call.
		if(realDirectory.Flags() & VFS_WATCH)
		{
			char osPath[PATH_MAX];
			(void)path_MakeAbsolute(realDirectory.Path(), osPath);
//			s_watches.push_back(CreateWatch(osPath));
		}

		PopulateHelper helper(directory, realDirectory);
		RETURN_ERR(helper.AddEntries());
	}

	return INFO::OK;
}
