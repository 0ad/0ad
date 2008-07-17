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
class PopulateHelper : noncopyable
{
public:
	PopulateHelper(VfsDirectory* directory, const PRealDirectory& realDirectory)
		: m_directory(directory), m_realDirectory(realDirectory)
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
		const VfsFile file(fileInfo.Name(), fileInfo.Size(), fileInfo.MTime(), m_realDirectory->Priority(), m_realDirectory);
		const VfsFile* pfile = m_directory->AddFile(file);

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
		const int flags = VFS_LOOKUP_ADD;
		VfsDirectory* directory;
		WARN_ERR(vfs_Lookup(pathname, this_->m_directory, directory, 0, flags));
		const VfsFile file(fileInfo.Name(), fileInfo.Size(), fileInfo.MTime(), this_->m_realDirectory->Priority(), archiveFile);
		directory->AddFile(file);
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

			VfsDirectory* subdirectory = m_directory->AddSubdirectory(subdirectoryNames[i]);
			subdirectory->Attach(CreateRealSubdirectory(m_realDirectory, subdirectoryNames[i]));
		}
	}

	VfsDirectory* const m_directory;
	PRealDirectory m_realDirectory;
};


static LibError Populate(VfsDirectory* directory)
{
	if(!directory->ShouldPopulate())
		return INFO::OK;

	const PRealDirectory& realDirectory = directory->AssociatedDirectory();

	if(realDirectory->Flags() & VFS_MOUNT_WATCH)
		realDirectory->Watch();

	PopulateHelper helper(directory, realDirectory);
	RETURN_ERR(helper.AddEntries());

	return INFO::OK;
}


//-----------------------------------------------------------------------------

LibError vfs_Lookup(const VfsPath& pathname, VfsDirectory* startDirectory, VfsDirectory*& directory, VfsFile** pfile, int flags)
{
TIMER_ACCRUE(tc_lookup);

	// extract and validate flags (ensure no unknown bits are set)
	const bool addMissingDirectories    = (flags & VFS_LOOKUP_ADD) != 0;
	const bool createMissingDirectories = (flags & VFS_LOOKUP_CREATE) != 0;
	debug_assert((flags & ~(VFS_LOOKUP_ADD|VFS_LOOKUP_CREATE)) == 0);

	if(pfile)
		*pfile = 0;

	directory = startDirectory;
	RETURN_ERR(Populate(directory));

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
				subdirectory->Attach(realDirectory);
			}
		}

		RETURN_ERR(Populate(subdirectory));
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
