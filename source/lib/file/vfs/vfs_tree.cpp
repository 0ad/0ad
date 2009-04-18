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

VfsFile::VfsFile(const std::string& name, off_t size, time_t mtime, size_t priority, const PIFileLoader& loader)
	: m_name(name), m_size(size), m_mtime(mtime), m_priority(priority), m_loader(loader)
{
}


bool VfsFile::IsSupersededBy(const VfsFile& file) const
{
	// 1) priority (override mods)
	if(file.m_priority < m_priority)	// lower priority
		return false;

	// 2) timestamp
	{
		const double howMuchNewer = difftime(file.MTime(), MTime());
		const double threshold = 2.0;	// [seconds]; resolution provided by FAT
		if(howMuchNewer > threshold)	// newer timestamp
			return true;
		if(howMuchNewer < threshold)	// older timestamp
			return false;
		// else: "equal" (tolerating small differences due to FAT's low
		// mtime resolution)
	}

	// 3) precedence (efficiency of file provider)
	if(file.m_loader->Precedence() < m_loader->Precedence())	// less efficient
		return false;

	return true;
}


void VfsFile::GenerateDescription(char* text, size_t maxChars) const
{
	char timestamp[25];
	const time_t mtime = MTime();
	strftime(timestamp, ARRAY_SIZE(timestamp), "%a %b %d %H:%M:%S %Y", localtime(&mtime));

	// build format string (set width of name field so that everything
	// lines up correctly)
	const char* fmt = "(%c; %6d; %s)\n";
	sprintf_s(text, maxChars, fmt, m_loader->LocationCode(), Size(), timestamp);
}


LibError VfsFile::Load(const shared_ptr<u8>& buf) const
{
	return m_loader->Load(Name(), buf, Size());
}


//-----------------------------------------------------------------------------

VfsDirectory::VfsDirectory()
: m_shouldPopulate(0)
{
}


VfsFile* VfsDirectory::AddFile(const VfsFile& file)
{
	std::pair<std::string, VfsFile> value = std::make_pair(file.Name(), file);
	std::pair<VfsFiles::iterator, bool> ret = m_files.insert(value);
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
// our map would be slower and less convenient for the caller.
VfsDirectory* VfsDirectory::AddSubdirectory(const std::string& name)
{
	std::pair<std::string, VfsDirectory> value = std::make_pair(name, VfsDirectory());
	std::pair<VfsSubdirectories::iterator, bool> ret = m_subdirectories.insert(value);
	return &(*ret.first).second;
}


VfsFile* VfsDirectory::GetFile(const std::string& name)
{
	VfsFiles::iterator it = m_files.find(name);
	if(it == m_files.end())
		return 0;
	return &it->second;
}


VfsDirectory* VfsDirectory::GetSubdirectory(const std::string& name)
{
	VfsSubdirectories::iterator it = m_subdirectories.find(name);
	if(it == m_subdirectories.end())
		return 0;
	return &it->second;
}


void VfsDirectory::GetEntries(FileInfos* files, DirectoryNames* subdirectoryNames) const
{
	if(files)
	{
		files->clear();
		files->reserve(m_files.size());
		for(VfsFiles::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
			files->push_back(FileInfo(it->second.Name(), it->second.Size(), it->second.MTime()));
	}

	if(subdirectoryNames)
	{
		subdirectoryNames->clear();
		subdirectoryNames->reserve(m_subdirectories.size());
		for(VfsSubdirectories::const_iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
			subdirectoryNames->push_back(it->first);
	}
}


void VfsDirectory::DisplayR(size_t depth) const
{
	static const char indent[] = "    ";

	const size_t maxNameChars = 80 - depth*(sizeof(indent)-1);
	char fmt[20];
	sprintf_s(fmt, ARRAY_SIZE(fmt), "%%-%d.%ds %%s", maxNameChars, maxNameChars);

	for(VfsFiles::const_iterator it = m_files.begin(); it != m_files.end(); ++it)
	{
		const std::string& name = it->first;
		const VfsFile& file = it->second;

		char description[100];
		file.GenerateDescription(description, ARRAY_SIZE(description));

		for(size_t i = 0; i < depth+1; i++)
			printf(indent);
		printf(fmt, name.c_str(), description);
	}

	for(VfsSubdirectories::const_iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
	{
		const std::string& name = it->first;
		const VfsDirectory& directory = it->second;

		for(size_t i = 0; i < depth+1; i++)
			printf(indent);
		printf("[%s/]\n", name.c_str());

		directory.DisplayR(depth+1);
	}
}


void VfsDirectory::ClearR()
{
	for(VfsSubdirectories::iterator it = m_subdirectories.begin(); it != m_subdirectories.end(); ++it)
		it->second.ClearR();

	m_files.clear();
	m_subdirectories.clear();
	m_realDirectory.reset();
	m_shouldPopulate = 0;
}


void VfsDirectory::SetAssociatedDirectory(const PRealDirectory& realDirectory)
{
	if(!cpu_CAS(&m_shouldPopulate, 0, 1))
		debug_assert(0);	// caller didn't check ShouldPopulate
	m_realDirectory = realDirectory;
}


bool VfsDirectory::ShouldPopulate()
{
	return cpu_CAS(&m_shouldPopulate, 1, 0);	// test and reset
}
