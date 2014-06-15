/* Copyright (c) 2013 Wildfire Games
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

#include "lib/file/archive/archive_zip.h"
#include "lib/file/vfs/vfs_tree.h"
#include "lib/file/vfs/vfs_lookup.h"
#include "lib/file/vfs/vfs.h"	// error codes


struct CompareFileInfoByName
{
	bool operator()(const CFileInfo& a, const CFileInfo& b)
	{
		return a.Name() < b.Name();
	}
};

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

	Status AddEntries() const
	{
		CFileInfos files; files.reserve(500);
		DirectoryNames subdirectoryNames; subdirectoryNames.reserve(50);
		RETURN_STATUS_IF_ERR(GetDirectoryEntries(m_realDirectory->Path(), &files, &subdirectoryNames));

		// to support .DELETED files inside archives safely, we need to load
		// archives and loose files in a deterministic order in case they add
		// and then delete the same file (or vice versa, depending on loading
		// order). GetDirectoryEntries has undefined order so sort its output
		std::sort(files.begin(), files.end(), CompareFileInfoByName());
		std::sort(subdirectoryNames.begin(), subdirectoryNames.end());

		RETURN_STATUS_IF_ERR(AddFiles(files));
		AddSubdirectories(subdirectoryNames);

		return INFO::OK;
	}

private:
	void AddFile(const CFileInfo& fileInfo) const
	{
		const VfsPath name = fileInfo.Name();
		if(name.Extension() == L".DELETED")
		{
			m_directory->RemoveFile(name.Basename());
			if(!(m_realDirectory->Flags() & VFS_MOUNT_KEEP_DELETED))
				return;
		}

		const VfsFile file(name, (size_t)fileInfo.Size(), fileInfo.MTime(), m_realDirectory->Priority(), m_realDirectory);
		m_directory->AddFile(file);
	}

	static void AddArchiveFile(const VfsPath& pathname, const CFileInfo& fileInfo, PIArchiveFile archiveFile, uintptr_t cbData)
	{
		PopulateHelper* this_ = (PopulateHelper*)cbData;

		// (we have to create missing subdirectoryNames because archivers
		// don't always place directory entries before their files)
		const size_t flags = VFS_LOOKUP_ADD|VFS_LOOKUP_SKIP_POPULATE;
		VfsDirectory* directory;
		WARN_IF_ERR(vfs_Lookup(pathname, this_->m_directory, directory, 0, flags));

		const VfsPath name = fileInfo.Name();
		if(name.Extension() == L".DELETED")
		{
			directory->RemoveFile(name.Basename());
			if(!(this_->m_realDirectory->Flags() & VFS_MOUNT_KEEP_DELETED))
				return;
		}

		const VfsFile file(name, (size_t)fileInfo.Size(), fileInfo.MTime(), this_->m_realDirectory->Priority(), archiveFile);
		directory->AddFile(file);
	}

	Status AddFiles(const CFileInfos& files) const
	{
		const OsPath path(m_realDirectory->Path());

		for(size_t i = 0; i < files.size(); i++)
		{
			const OsPath pathname = path / files[i].Name();
			if(pathname.Extension() == L".zip")
			{
				PIArchiveReader archiveReader = CreateArchiveReader_Zip(pathname);
				// archiveReader == nullptr if file could not be opened (e.g. because
				// archive is currently open in another program)
				if(archiveReader)
					RETURN_STATUS_IF_ERR(archiveReader->ReadEntries(AddArchiveFile, (uintptr_t)this));
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
			if(subdirectoryNames[i] == L".svn" || subdirectoryNames[i] == L".git")
				continue;

			VfsDirectory* subdirectory = m_directory->AddSubdirectory(subdirectoryNames[i]);
			PRealDirectory realDirectory = CreateRealSubdirectory(m_realDirectory, subdirectoryNames[i]);
			vfs_Attach(subdirectory, realDirectory);
		}
	}

	VfsDirectory* const m_directory;
	PRealDirectory m_realDirectory;
};


Status vfs_Populate(VfsDirectory* directory)
{
	if(!directory->ShouldPopulate())
		return INFO::OK;

	const PRealDirectory& realDirectory = directory->AssociatedDirectory();

	if(realDirectory->Flags() & VFS_MOUNT_WATCH)
		realDirectory->Watch();

	PopulateHelper helper(directory, realDirectory);
	RETURN_STATUS_IF_ERR(helper.AddEntries());

	return INFO::OK;
}


Status vfs_Attach(VfsDirectory* directory, const PRealDirectory& realDirectory)
{
	RETURN_STATUS_IF_ERR(vfs_Populate(directory));
	directory->SetAssociatedDirectory(realDirectory);
	return INFO::OK;
}
