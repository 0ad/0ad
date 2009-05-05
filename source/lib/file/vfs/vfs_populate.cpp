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

/*
 * populate VFS directories with files
 */

#include "precompiled.h"
#include "vfs_populate.h"

#include "lib/path_util.h"
#include "lib/file/file_system_posix.h"
#include "lib/file/archive/archive_zip.h"
#include "vfs_tree.h"
#include "vfs_lookup.h"
#include "vfs.h"	// error codes


static FileSystem_Posix s_fileSystemPosix;
static std::vector<const VfsFile*> s_looseFiles;
static size_t s_numArchivedFiles;

// helper class that allows breaking up the logic into sub-functions without
// always having to pass directory/realDirectory as parameters.
class PopulateHelper
{
	NONCOPYABLE(PopulateHelper);
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
		const VfsFile file(fileInfo.Name(), (size_t)fileInfo.Size(), fileInfo.MTime(), m_realDirectory->Priority(), m_realDirectory);
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
		const size_t flags = VFS_LOOKUP_ADD;
		VfsDirectory* directory;
		WARN_ERR(vfs_Lookup(pathname, this_->m_directory, directory, 0, flags));
		const VfsFile file(fileInfo.Name(), (size_t)fileInfo.Size(), fileInfo.MTime(), this_->m_realDirectory->Priority(), archiveFile);
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
			PRealDirectory realDirectory = CreateRealSubdirectory(m_realDirectory, subdirectoryNames[i]);
			vfs_Attach(subdirectory, realDirectory);
		}
	}

	VfsDirectory* const m_directory;
	PRealDirectory m_realDirectory;
};


LibError vfs_Populate(VfsDirectory* directory)
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


LibError vfs_Attach(VfsDirectory* directory, const PRealDirectory& realDirectory)
{
	RETURN_ERR(vfs_Populate(directory));
	directory->SetAssociatedDirectory(realDirectory);
	return INFO::OK;
}
