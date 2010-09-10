/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * populate VFS directories with files
 */

#include "precompiled.h"
#include "lib/file/vfs/vfs_populate.h"

#include "lib/path_util.h"
#include "lib/file/archive/archive_zip.h"
#include "lib/file/vfs/vfs_tree.h"
#include "lib/file/vfs/vfs_lookup.h"
#include "lib/file/vfs/vfs.h"	// error codes


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
		RETURN_ERR(GetDirectoryEntries(m_realDirectory->Path(), &files, &subdirectoryNames));
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
		const fs::wpath path(m_realDirectory->Path());

		for(size_t i = 0; i < files.size(); i++)
		{
			const fs::wpath pathname = path/files[i].Name();
			const std::wstring extension = fs::extension(pathname);
			if(wcscasecmp(extension.c_str(), L".zip") == 0)
			{
				PIArchiveReader archiveReader = CreateArchiveReader_Zip(pathname);
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
			if(wcscasecmp(subdirectoryNames[i].c_str(), L".svn") == 0)
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
