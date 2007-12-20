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

#include "lib/file/common/file_stats.h"
#include "lib/sysdep/cpu.h"


//-----------------------------------------------------------------------------

VfsFile::VfsFile(const FileInfo& fileInfo, unsigned priority, PIFileLoader loader)
	: m_fileInfo(fileInfo), m_priority(priority), m_loader(loader)
{
}


bool VfsFile::IsSupersededBy(const VfsFile& vfsFile) const
{
	// 1) priority is lower => no.
	if(vfsFile.m_priority < m_priority)
		return false;

	// 2) timestamp is older => no.
	// (note: we need to account for FAT's 2 sec. resolution)
	if(difftime(vfsFile.m_fileInfo.MTime(), m_fileInfo.MTime()) < -2.0)
		return false;

	// 3) provider is less efficient => no.
	if(vfsFile.m_loader->Precedence() < m_loader->Precedence())
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
	sprintf_s(text, maxChars, fmt, m_loader->LocationCode(), m_fileInfo.Size(), timestamp);
}


LibError VfsFile::Load(shared_ptr<u8> buf) const
{
	return m_loader->Load(m_fileInfo.Name(), buf, m_fileInfo.Size());
}


//-----------------------------------------------------------------------------

VfsDirectory::VfsDirectory()
: m_shouldPopulate(0)
{
}


VfsFile* VfsDirectory::AddFile(const VfsFile& vfsFile)
{
	std::pair<std::string, VfsFile> value = std::make_pair(vfsFile.Name(), vfsFile);
	std::pair<VfsFiles::iterator, bool> ret = m_vfsFiles.insert(value);
	if(!ret.second)	// already existed
	{
		VfsFile& vfsPreviousFile = ret.first->second;
		const VfsFile& vfsNewFile = value.second;
		if(vfsPreviousFile.IsSupersededBy(vfsNewFile))
			vfsPreviousFile = vfsNewFile;
	}
	else
		stats_vfs_file_add(vfsFile.Size());

	return &(*ret.first).second;
}


// rationale: passing in a pre-constructed VfsDirectory and copying that into
// our map would be slower and less convenient for the caller.
VfsDirectory* VfsDirectory::AddSubdirectory(const std::string& name)
{
	std::pair<std::string, VfsDirectory> value = std::make_pair(name, VfsDirectory());
	std::pair<VfsSubdirectories::iterator, bool> ret = m_vfsSubdirectories.insert(value);
	return &(*ret.first).second;
}


VfsFile* VfsDirectory::GetFile(const std::string& name)
{
	VfsFiles::iterator it = m_vfsFiles.find(name);
	if(it == m_vfsFiles.end())
		return 0;
	return &it->second;
}


VfsDirectory* VfsDirectory::GetSubdirectory(const std::string& name)
{
	VfsSubdirectories::iterator it = m_vfsSubdirectories.find(name);
	if(it == m_vfsSubdirectories.end())
		return 0;
	return &it->second;
}


void VfsDirectory::GetEntries(FileInfos* files, DirectoryNames* subdirectoryNames) const
{
	if(files)
	{
		// (note: VfsFile doesn't return a pointer to FileInfo; instead,
		// we have it write directly into the files container)
		files->resize(m_vfsFiles.size());
		size_t i = 0;
		for(VfsFiles::const_iterator it = m_vfsFiles.begin(); it != m_vfsFiles.end(); ++it)
			it->second.GetFileInfo(&(*files)[i++]);
	}

	if(subdirectoryNames)
	{
		subdirectoryNames->clear();
		subdirectoryNames->reserve(m_vfsSubdirectories.size());
		for(VfsSubdirectories::const_iterator it = m_vfsSubdirectories.begin(); it != m_vfsSubdirectories.end(); ++it)
			subdirectoryNames->push_back(it->first);
	}
}


void VfsDirectory::DisplayR(unsigned depth) const
{
	static const char indent[] = "    ";

	const int maxNameChars = 80 - depth*(sizeof(indent)-1);
	char fmt[20];
	sprintf_s(fmt, ARRAY_SIZE(fmt), "%%-%d.%ds %%s", maxNameChars, maxNameChars);

	for(VfsFiles::const_iterator it = m_vfsFiles.begin(); it != m_vfsFiles.end(); ++it)
	{
		const std::string& name = it->first;
		const VfsFile& vfsFile = it->second;

		char description[100];
		vfsFile.GenerateDescription(description, ARRAY_SIZE(description));

		for(unsigned i = 0; i < depth+1; i++)
			printf(indent);
		printf(fmt, name.c_str(), description);
	}

	for(VfsSubdirectories::const_iterator it = m_vfsSubdirectories.begin(); it != m_vfsSubdirectories.end(); ++it)
	{
		const std::string& name = it->first;
		const VfsDirectory& vfsDirectory = it->second;

		for(unsigned i = 0; i < depth+1; i++)
			printf(indent);
		printf("[%s/]\n", name.c_str());

		vfsDirectory.DisplayR(depth+1);
	}
}


void VfsDirectory::ClearR()
{
	for(VfsSubdirectories::iterator it = m_vfsSubdirectories.begin(); it != m_vfsSubdirectories.end(); ++it)
		it->second.ClearR();

	m_vfsFiles.clear();
	m_vfsSubdirectories.clear();
	m_realDirectory.reset();
	m_shouldPopulate = 0;
}


void VfsDirectory::Attach(PRealDirectory realDirectory)
{
debug_printf("ATTACH %s\n", realDirectory->GetPath().string().c_str());

	if(!cpu_CAS(&m_shouldPopulate, 0, 1))
	{
		debug_assert(0);	// multiple Attach() calls without an intervening ShouldPopulate()
		return;
	}

	m_realDirectory = realDirectory;
}


bool VfsDirectory::ShouldPopulate()
{
	return cpu_CAS(&m_shouldPopulate, 1, 0);	// test and reset
}
