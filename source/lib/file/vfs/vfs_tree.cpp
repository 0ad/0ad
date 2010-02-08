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
 * 'tree' of VFS directories and files
 */

#include "precompiled.h"
#include "vfs_tree.h"

#include <cstdio>

#include "lib/file/common/file_stats.h"
#include "lib/sysdep/cpu.h"


//-----------------------------------------------------------------------------

VfsFile::VfsFile(const std::wstring& name, size_t size, time_t mtime, size_t priority, const PIFileLoader& loader)
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
	std::pair<std::wstring, VfsFile> value = std::make_pair(file.Name(), file);
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
VfsDirectory* VfsDirectory::AddSubdirectory(const std::wstring& name)
{
	std::pair<std::wstring, VfsDirectory> value = std::make_pair(name, VfsDirectory());
	std::pair<VfsSubdirectories::iterator, bool> ret = m_subdirectories.insert(value);
	return &(*ret.first).second;
}


VfsFile* VfsDirectory::GetFile(const std::wstring& name)
{
	VfsFiles::iterator it = m_files.find(name);
	if(it == m_files.end())
		return 0;
	return &it->second;
}


VfsDirectory* VfsDirectory::GetSubdirectory(const std::wstring& name)
{
	VfsSubdirectories::iterator it = m_subdirectories.find(name);
	if(it == m_subdirectories.end())
		return 0;
	return &it->second;
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


void VfsDirectory::Invalidate(const std::wstring& name)
{
	m_files.erase(name);
	m_shouldPopulate = 1;
}


void VfsDirectory::Clear()
{
	m_files.clear();
	m_subdirectories.clear();
	m_realDirectory.reset();
	m_shouldPopulate = 0;
}


//-----------------------------------------------------------------------------

std::wstring FileDescription(const VfsFile& file)
{
	wchar_t timestamp[25];
	const time_t mtime = file.MTime();
	wcsftime(timestamp, ARRAY_SIZE(timestamp), L"%a %b %d %H:%M:%S %Y", localtime(&mtime));

	wchar_t buf[200];
	swprintf_s(buf, ARRAY_SIZE(buf), L"(%c; %6lu; %ls) %ls", file.LocationCode(), (unsigned long)file.Size(), timestamp, file.Name().c_str());
	return buf;
}


std::wstring FileDescriptions(const VfsDirectory& directory, size_t indentLevel)
{
	VfsDirectory::VfsFiles files = directory.Files();

	std::wstring descriptions;
	descriptions.reserve(100*files.size());

	const std::wstring indentation(4*indentLevel, ' ');
	for(VfsDirectory::VfsFiles::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		const VfsFile& file = it->second;
		descriptions += indentation;
		descriptions += FileDescription(file);
		descriptions += L"\n";
	}

	return descriptions;
}


void DirectoryDescriptionR(std::wstring& descriptions, const VfsDirectory& directory, size_t indentLevel)
{
	const std::wstring indentation(4*indentLevel, ' ');

	const VfsDirectory::VfsSubdirectories& subdirectories = directory.Subdirectories();
	for(VfsDirectory::VfsSubdirectories::const_iterator it = subdirectories.begin(); it != subdirectories.end(); ++it)
	{
		const std::wstring& name = it->first;
		const VfsDirectory& subdirectory = it->second;
		descriptions += indentation;
		descriptions += std::wstring(L"[") + name + L"]\n";
		descriptions += FileDescriptions(subdirectory, indentLevel+1);

		DirectoryDescriptionR(descriptions, subdirectory, indentLevel+1);
	}
}
