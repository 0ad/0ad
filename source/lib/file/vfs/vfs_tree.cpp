/**
 * =========================================================================
 * File        : vfs_tree.cpp
 * Project     : 0 A.D.
 * Description : the actual 'filesystem' and its tree of directories.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_tree.h"

#include "lib/file/posix/fs_posix.h"
#include "lib/file/archive/archive.h"

static Filesystem_Posix fsPosix;

static const char* svnName = path_Pool()->UniqueCopy(".svn");


LibError VfsFile::Load(u8* buf) const
{
	if(m_archiveEntry)
		RETURN_ERR(m_archiveEntry->archiveReader->LoadFile(*m_archiveEntry, buf));
	else
	{
		const char* pathname = path_append2(m_path, Name());
		File_Posix file;
		RETURN_ERR(file.Open(pathname, 'r'));
		RETURN_ERR(io_Read(file, 0, buf, Size()));
	}

	return INFO::OK;
}

VfsDirectory::VfsDirectory(const char* vfsPath, const char* path)
	: m_vfsPath(vfsPath), m_path(0)
{
	if(path)
		AssociateWithRealDirectory(path);
}


VfsDirectory::~VfsDirectory()
{
}



static bool ShouldReplace(VfsFile& vf1, VfsFile& vf2)
{
	// 1) keep old if new priority is lower.
	if(vf2.Priority() < vf1.Priority())
		return false;

	// assume they're the same if size and last-modified time match.
	// note: FAT timestamp only has 2 second resolution
	const double diff = difftime(vf1.MTime(), vf2.MTime());
	const bool identical = (vf1.Size() == vf2.Size()) && fabs(diff) <= 2.0;

	// 3) go with more efficient source (if files are identical)
	//
	// since priority is not less, we really ought to always go with m_new.
	// however, there is one special case we handle for performance reasons:
	// if the file contents are the same, prefer the more efficient source.
	// note that priority doesn't automatically take care of this,
	// especially if set incorrectly.
	if(identical && vf1.Precedence() > vf2.Precedence())
		return false;

	// 4) don't replace "old" file if modified more recently than "new".
	// (still provide for 2 sec. FAT tolerance - see above)
	if(diff > 2.0)
		return false;

	return true;
}


void VfsDirectory::AddFile(const FileInfo& fileInfo)
{
	std::pair<const char*, VfsFile> value = std::make_pair(fileInfo.Name(), VfsFile(fileInfo));
	std::pair<Files::iterator, bool> ret = m_files.insert(value);
	if(!ret.second)	// already existed
	{
		VfsFile& previousFile = ret.first.second;
		if(!ShouldReplace(previousFile, value.second))
			return INFO::ALREADY_EXISTS;
		replace_in_map
	}

	stats_vfs_file_add(size);
}


void VfsDirectory::AddSubdirectory(const char* name)
{
	const char* vfsPath = path_append2(m_vfsPath, name);
	std::pair<const char*, VfsDirectory> value = std::make_pair(name, VfsDirectory(vfsPath));
	(void)m_subdirectories.insert(value);
}


VfsFile* VfsDirectory::GetFile(const char* name) const
{
	Files::const_iterator it = m_files.find(name);
	if(it == m_files.end())
		return 0;
	return &it->second;
}


VfsDirectory* VfsDirectory::GetSubdirectory(const char* name) const
{
	Subdirectories::const_iterator it = m_subdirectories.find(name);
	if(it == m_subdirectories.end())
		return 0;
	return &it->second;
}


void VfsDirectory::GetEntries(std::vector<FileInfo>* files, std::vector<const char*>* subdirectories) const
{
	if(files)
	{
		files->reserve(m_files.size());
		for(Files::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
			files->push_back(it->second.m_fileInfo);
	}

	if(subdirectories)
	{
		subdirectories->reserve(m_subdirectories.size());
		for(Subdirectories::const_iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
			subdirectories->push_back(it->second.m_name);
	}
}


void VfsDirectory::DisplayR(unsigned depth) const
{
	const char indent[] = "    ";

	// build format string (set width of name field so that everything
	// lines up correctly)
	char fmt[25];
	const int maxNameChars = 80 - depth*(sizeof(indent)-1);
	sprintf(fmt, "%%-%d.%ds (%%c; %%6d; %%s)\n", maxNameChars, maxNameChars);

	for(Files::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
	{
		VfsFile& file = it->second;

		char timestamp[25];
		const time_t mtime = file.MTime();
		strftime(timestamp, ARRAY_SIZE(timestamp), "%a %b %d %H:%M:%S %Y", localtime(&mtime));

		for(int i = 0; i < depth; i++)
			printf(indent);
		printf(fmt, file.Name(), file.LocationCode(), file.Size(), timestamp);
	}

	for(Subdirectories::iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
	{
		const char* name = it->first;
		const VfsDirectory& directory = it->second;

		for(int i = 0; i < depth+1; i++)
			printf(indent);
		printf("[%s/]\n", name);
		directory.DisplayR(depth+1);
	}
}


void VfsDirectory::ClearR()
{
	for(Subdirectories::iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
		it->second.ClearR();

	m_files.Clear();
	m_subdirectories.Clear();
	m_watch.Unregister();
}



static const char* AMBIGUOUS = (const char*)(intptr_t)-1;


void VfsDirectory::AssociateWithRealDirectory(const char* path)
{
	// mounting the same directory twice is probably a bug
	debug_assert(m_path != path);

	if(!m_path)
	{
		m_path = path;
		m_watch.Register(path);
	}
	else
		m_path = AMBIGUOUS;
}


void VfsDirectory::Populate()
{
	std::vector<FileInfo> files; files.reserve(100);
	std::vector<const char*> subdirectories; subdirectories.reserve(20);
	fsPosix.GetDirectoryEntries(m_path, &files, &subdirectories);

	for(size_t i = 0; i < files.size(); i++)
	{
		AddFile(files[i]);

		// note: check if archivable to exclude stuff like screenshots
		// from counting towards the threshold.
		if(m->IsArchivable())
		{
			// notify archive builder that this file could be archived but
			// currently isn't; if there are too many of these, archive will
			// be rebuilt.
			const char* pathname = path_append2(m_path, files[i].Name());
			vfs_opt_notify_loose_file(pathname);
		}
	}

	for(size_t i = 0; i < subdirectories.size(); i++)
	{
		// skip version control directories - this avoids cluttering the
		// VFS with hundreds of irrelevant files.
		if(subdirectories[i] == svnName)
			continue;

		const char* path = path_append2(m_path, subdirectories[i]);
		AddSubdirectory(subdirectories[i], path);
	}
}


LibError VfsDirectory::CreateFile(const char* name, const u8* buf, size_t size, uint flags = 0)
{
	debug_assert(m_path != 0 && m_path != AMBIGUOUS);

	const char* pathname = path_append2(pathname, m_path, name);
	File_Posix file;
	RETURN_ERR(file.Open(pathname, 'w'));
	RETURN_ERR(io_Write(file, 0, buf, size));

	AddFile(FileInfo(name, size, time()));
	return INFO::OK;
}


//-----------------------------------------------------------------------------


VfsTree::VfsTree()
	: m_rootDirectory("")
{
}


void VfsTree::Display() const
{
	m_rootDirectory.DisplayR();
}


void VfsTree::Clear()
{
	m_rootDirectory.ClearR();
}
