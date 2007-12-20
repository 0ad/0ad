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
#include "lib/file/file_system_posix.h"
#include "lib/file/archive/archive_zip.h"
#include "vfs_tree.h"
#include "vfs.h"	// error codes

#include "lib/timer.h"
TIMER_ADD_CLIENT(tc_lookup);


//-----------------------------------------------------------------------------

static FileSystem_Posix s_fileSystemPosix;
static std::vector<const VfsFile*> s_looseFiles;
static size_t s_numArchivedFiles;

// helper class that allows breaking up the logic into sub-functions without
// always having to pass directory/realDirectory as parameters.
class PopulateHelper : boost::noncopyable
{
public:
	PopulateHelper(VfsDirectory* vfsDirectory, PRealDirectory realDirectory)
		: m_vfsDirectory(vfsDirectory), m_realDirectory(realDirectory)
	{
	}

	LibError AddEntries() const
	{
		FileInfos files; files.reserve(100);
		DirectoryNames subdirectoryNames; subdirectoryNames.reserve(20);
		RETURN_ERR(s_fileSystemPosix.GetDirectoryEntries(m_realDirectory->GetPath(), &files, &subdirectoryNames));
		RETURN_ERR(AddFiles(files));
		AddSubdirectories(subdirectoryNames);
		return INFO::OK;
	}

private:
	void AddFile(const FileInfo& fileInfo) const
	{
		const VfsFile file(fileInfo, m_realDirectory->Priority(), m_realDirectory);
		const VfsFile* pfile = m_vfsDirectory->AddFile(file);

		// notify archive builder that this file could be archived but
		// currently isn't; if there are too many of these, archive will
		// be rebuilt.
		// note: check if archivable to exclude stuff like screenshots
		// from counting towards the threshold.
		if(m_realDirectory->Flags() & VFS_MOUNT_ARCHIVABLE)
			s_looseFiles.push_back(pfile);
	}

	static void AddArchiveFile(const VfsPath& pathname, const FileInfo& fileInfo, PIArchiveFile archiveFile, uintptr_t cbData)
	{
		PopulateHelper* this_ = (PopulateHelper*)cbData;

		// (we have to create missing subdirectoryNames because archivers
		// don't always place directory entries before their files)
		const unsigned flags = VFS_LOOKUP_ADD;
		VfsDirectory* vfsDirectory;
		WARN_ERR(vfs_Lookup(pathname, this_->m_vfsDirectory, vfsDirectory, 0, flags));
		const VfsFile vfsFile(fileInfo, this_->m_realDirectory->Priority(), archiveFile);
		vfsDirectory->AddFile(vfsFile);
		s_numArchivedFiles++;
	}

	LibError AddFiles(const FileInfos& files) const
	{
		const Path path(m_realDirectory->GetPath());

		for(size_t i = 0; i < files.size(); i++)
		{
			const std::string& name = files[i].Name();

			const char* extension = path_extension(name.c_str());
			if(strcasecmp(extension, "zip") == 0)
			{
				PIArchiveReader archiveReader = CreateArchiveReader_Zip(path/name);
				RETURN_ERR(archiveReader->ReadEntries(AddArchiveFile, (uintptr_t)this));
			}
			else	// regular (non-archive) file
				AddFile(files[i]);
		}

		return INFO::OK;
	}

	void AddSubdirectories(const DirectoryNames& subdirectoryNames) const
	{
		for(size_t i = 0; i < subdirectoryNames.size(); i++)
		{
			// skip version control directories - this avoids cluttering the
			// VFS with hundreds of irrelevant files.
			if(strcasecmp(subdirectoryNames[i].c_str(), ".svn") == 0)
				continue;

			VfsDirectory* subdirectory = m_vfsDirectory->AddSubdirectory(subdirectoryNames[i]);
			subdirectory->Attach(CreateRealSubdirectory(m_realDirectory, subdirectoryNames[i]));
		}
	}

	VfsDirectory* const m_vfsDirectory;
	PRealDirectory m_realDirectory;
};


static LibError Populate(VfsDirectory* vfsDirectory)
{
	if(!vfsDirectory->ShouldPopulate())
		return INFO::OK;

	PRealDirectory realDirectory = vfsDirectory->AssociatedDirectory();

	if(realDirectory->Flags() & VFS_MOUNT_WATCH)
		realDirectory->Watch();

	PopulateHelper helper(vfsDirectory, realDirectory);
	RETURN_ERR(helper.AddEntries());

	return INFO::OK;
}


//-----------------------------------------------------------------------------

LibError vfs_Lookup(const VfsPath& pathname, VfsDirectory* vfsStartDirectory, VfsDirectory*& vfsDirectory, VfsFile** pvfsFile, unsigned flags)
{
TIMER_ACCRUE(tc_lookup);

	vfsDirectory = vfsStartDirectory;
	if(pvfsFile)
		*pvfsFile = 0;
	RETURN_ERR(Populate(vfsDirectory));

	if(pathname.empty())	// early out for root directory
		return INFO::OK;	// (prevents iterator error below)

	VfsPath currentPath;	// only used if flags & VFS_LOOKUP_CREATE
	VfsPath::iterator it;

	// for each directory component:
	for(it = pathname.begin(); it != --pathname.end(); ++it)
	{
		const std::string& subdirectoryName = *it;

		VfsDirectory* vfsSubdirectory = vfsDirectory->GetSubdirectory(subdirectoryName);
		if(!vfsSubdirectory)
		{
			if(!(flags & VFS_LOOKUP_ADD))
				return ERR::VFS_DIR_NOT_FOUND;	// NOWARN

			vfsSubdirectory = vfsDirectory->AddSubdirectory(subdirectoryName);

			if(flags & VFS_LOOKUP_CREATE)
			{
				currentPath /= subdirectoryName;

#if 0
				(void)mkdir(currentPath.external_directory_string().c_str(), S_IRWXO|S_IRWXU|S_IRWXG);

				PRealDirectory realDirectory(new RealDirectory(currentPath.string(), 0, 0));
				vfsSubdirectory->Attach(realDirectory);
#endif
			}
		}

		vfsDirectory = vfsSubdirectory;
		RETURN_ERR(Populate(vfsDirectory));
	}

	if(pvfsFile)
	{
		const std::string& filename = *it;
		debug_assert(filename != ".");	// asked for file but specified directory path
		*pvfsFile = vfsDirectory->GetFile(filename);
		if(!*pvfsFile)
			return ERR::VFS_FILE_NOT_FOUND;	// NOWARN
	}

	return INFO::OK;
}
