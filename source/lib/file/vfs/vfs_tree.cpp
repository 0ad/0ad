/**
 * =========================================================================
 * File        : vfs_tree.cpp
 * Project     : 0 A.D.
 * Description : 'tree' of VFS directories and files
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "vfs_tree.h"

#include "lib/file/file_stats.h"


//-----------------------------------------------------------------------------

VfsFile::VfsFile(const FileInfo& fileInfo, unsigned priority, PIFileProvider provider, const void* location)
: m_fileInfo(fileInfo), m_priority(priority)
, m_provider(provider), m_location(location)
{
}


bool VfsFile::IsSupersededBy(const VfsFile& file) const
{
	// 1) priority is lower => no.
	if(file.m_priority < m_priority)
		return false;

	// 2) timestamp is older => no.
	// (note: we need to account for FAT's 2 sec. resolution)
	if(difftime(file.m_fileInfo.MTime(), m_fileInfo.MTime()) < -2.0)
		return false;

	// 3) provider is less efficient => no.
	if(file.m_provider.get()->Precedence() < m_provider.get()->Precedence())
		return false;

	return true;
}


void VfsFile::GenerateDescription(char* text, size_t maxChars) const
{
	char timestamp[25];
	const time_t mtime = m_fileInfo.MTime();
	strftime(timestamp, ARRAY_SIZE(timestamp), "%a %b %d %H:%M:%S %Y", localtime(&mtime));

	// build format string (set width of name field so that everything
	// lines up correctly)
	const char* fmt = "(%c; %6d; %s)\n";
	sprintf_s(text, maxChars, fmt, m_provider.get()->LocationCode(), m_fileInfo.Size(), timestamp);
}


LibError VfsFile::Store(const u8* fileContents, size_t size) const
{
	RETURN_ERR(m_provider.get()->Store(m_fileInfo.Name(), m_location, fileContents, size));

	// update size and mtime
	m_fileInfo = FileInfo(m_fileInfo.Name(), (off_t)size, time(0));

	return INFO::OK;
}


LibError VfsFile::Load(u8* fileContents) const
{
	return m_provider.get()->Load(m_fileInfo.Name(), m_location, fileContents, m_fileInfo.Size());
}


//-----------------------------------------------------------------------------

VfsDirectory::VfsDirectory()
{
}


VfsFile* VfsDirectory::AddFile(const VfsFile& file)
{
	std::pair<const char*, VfsFile> value = std::make_pair(file.Name(), file);
	std::pair<Files::iterator, bool> ret = m_files.insert(value);
	if(!ret.second)	// already existed
	{
		VfsFile& previousFile = ret.first->second;
		const VfsFile& newFile = value.second;
		if(previousFile.IsSupersededBy(newFile))
			previousFile = newFile;
	}
	else
		stats_vfs_file_add(file.Size());

	return &(*ret.first).second;
}


// rationale: passing in a pre-constructed VfsDirectory and copying that into
// our map would be less efficient than this approach.
VfsDirectory* VfsDirectory::AddSubdirectory(const char* name)
{
	std::pair<const char*, VfsDirectory> value = std::make_pair(name, VfsDirectory());
	std::pair<Subdirectories::iterator, bool> ret = m_subdirectories.insert(value);
	return &(*ret.first).second;
}


VfsFile* VfsDirectory::GetFile(const char* name)
{
	Files::iterator it = m_files.find(name);
	if(it == m_files.end())
		return 0;
	return &it->second;
}


VfsDirectory* VfsDirectory::GetSubdirectory(const char* name)
{
	Subdirectories::iterator it = m_subdirectories.find(name);
	if(it == m_subdirectories.end())
		return 0;
	return &it->second;
}


void VfsDirectory::GetEntries(FileInfos* files, Directories* subdirectories) const
{
	if(files)
	{
		// (note: VfsFile doesn't return a pointer to FileInfo; instead,
		// we have it write directly into the files container)
		files->resize(m_files.size());
		size_t i = 0;
		for(Files::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
			it->second.GetFileInfo((*files)[i++]);
	}

	if(subdirectories)
	{
		subdirectories->clear();
		subdirectories->reserve(m_subdirectories.size());
		for(Subdirectories::const_iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
			subdirectories->push_back(it->first);
	}
}


void VfsDirectory::DisplayR(unsigned depth) const
{
	const char indent[] = "    ";

	const int maxNameChars = 80 - depth*(sizeof(indent)-1);
	char fmt[20];
	sprintf_s(fmt, ARRAY_SIZE(fmt), "%%-%d.%ds %s", maxNameChars, maxNameChars);

	for(Files::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
	{
		const char* name = it->first;
		const VfsFile& file = it->second;

		char description[100];
		file.GenerateDescription(description, ARRAY_SIZE(description));

		for(unsigned i = 0; i < depth; i++)
			printf(indent);
		printf(fmt, name, description);
	}

	for(Subdirectories::const_iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
	{
		const char* name = it->first;
		const VfsDirectory& directory = it->second;

		for(unsigned i = 0; i < depth+1; i++)
			printf(indent);
		printf("[%s/]\n", name);

		directory.DisplayR(depth+1);
	}
}


void VfsDirectory::ClearR()
{
	for(Subdirectories::iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
		it->second.ClearR();

	m_files.clear();
	m_subdirectories.clear();
	m_attachedDirectories.clear();
}


void VfsDirectory::Attach(const RealDirectory& realDirectory)
{
	m_attachedDirectories.push_back(realDirectory);
}
