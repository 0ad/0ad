// skip version control directories - this avoids cluttering the
// VFS with hundreds of irrelevant files.
static const char* const svnName = path_Pool()->UniqueCopy(".svn");


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

private:
	LibError Next(const char* pathname, const ArchiveEntry& archiveEntry)
	{
		vfs_opt_notify_archived_file(pathname);

		const char* name = path_name_only(pathname);
		const char* path = path_dir_only2(pathname);

		// into which directory should the file be inserted?
		VfsDirectory* directory = m_previousDirectory;
		if(path != m_previousPath)
		{
			// (we have to create missing subdirectories because archivers
			// don't always place directory entries before their files)
			VfsFile* file;
			TraverseAndCreate(path, m_startDirectory, directory, file);
			debug_assert(file == 0);	// should not be a file on the path

			m_previousPath = path;
			m_previousDirectory = directory;
		}

		const FileInfo fileInfo(name, archiveEntry.usize, archiveEntry.mtime);
		directory->AddFile(fileInfo, pri, );

		return INFO::CB_CONTINUE;
	}

	static LibError Callback(const char* pathname, const ArchiveEntry& archiveEntry, uintptr_t cbData)
	{
		ArchiveEnumerator* archiveEnumerator = (ArchiveEnumerator*)cbData;
		return ArchiveEnumerator->Next(pathname, archiveEntry);
	}

	VfsDirectory* m_startDirectory;

	// optimization: looking up each full path is rather slow, so we
	// cache the previous directory and use it if the path string
	// addresses match.
	const char* m_previousPath;
	VfsDirectory* m_previousDirectory;
};

static LibError AddArchiveFiles(VfsDirectory* directory, PIArchiveReader archiveReader)
{
	ArchiveEnumerator archiveEnumerator(directory);
	RETURN_ERR(archiveReader->ReadEntries(&Callback, (uintptr_t)&archiveEnumerator));
}


/**
 * search for archives and add their contents into VFS.
 *
 * @param directory already populated VFS directory into which to
 * add the archives' contents.
 *
 * note: we only support archives in the mount point and not its
 * subdirectories. this simplifies Populate() and avoids having to
 * check the extension of every single VFS file.
 **/
LibError AddArchives(VfsDirectory* directory)
{
	std::vector<FileInfo> files;
	directory->GetEntries(&files, 0);
	for(size_t i = 0; i < files.size(); i++)
	{
		FileInfo& fileInfo = files[i];
		const char* extension = path_extension(fileInfo.Name());
		const char* pathname = path_append2(m_path, fileInfo.Name());
		PIArchiveReader archiveReader;
		if(strcasecmp(extension, "zip") == 0)
			archiveReader = CreateArchiveReader_Zip(pathname);
		else
			continue;	// not a (supported) archive file

		AddArchiveFiles(archiveReader, directory);
		m_archiveReaders.push_back(archiveReader);
	}

	return INFO::OK;
}

LibError RealDirectory::CreateFile(const char* name)
{
	const char* pathname = path_append2(pathname, m_path, name);
	File_Posix file;
	RETURN_ERR(file.Open(pathname, 'w'));
	RETURN_ERR(io_Write(file, 0, buf, size));
}


		RETURN_ERR(AddArchives(directory, m_archiveReaders));