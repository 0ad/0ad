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
#include "lib/file/vfs/vfs_tree.h"

#include <cstdio>

#include "lib/file/common/file_stats.h"
#include "lib/sysdep/cpu.h"


//-----------------------------------------------------------------------------

VfsFile::VfsFile(const VfsPath& name, size_t size, time_t mtime, size_t priority, const PIFileLoader& loader)
	: m_name(name), m_size(size), m_mtime(mtime), m_priority(priority), m_loader(loader)
{
}



//-----------------------------------------------------------------------------

VfsDirectory::VfsDirectory()
	: m_shouldPopulate(0)
{
}


static bool ShouldReplaceWith(const VfsFile& previousFile, const VfsFile& newFile)
{
	// 1) priority (override mods)
	if(newFile.Priority() < previousFile.Priority())
		return false;

	// 2) timestamp
	{
		const double howMuchNewer = difftime(newFile.MTime(), previousFile.MTime());
		const double threshold = 2.0;	// FAT timestamp resolution [seconds]
		if(howMuchNewer > threshold)	// newer
			return true;
		if(howMuchNewer < -threshold)	// older
			return false;
		// else: "equal" (tolerating small differences due to FAT's low
		// mtime resolution)
	}

	// 3) precedence (efficiency of file provider)
	if(newFile.Loader()->Precedence() < previousFile.Loader()->Precedence())
		return false;

	return true;
}


VfsFile* VfsDirectory::AddFile(const VfsFile& file)
{
	std::pair<VfsPath, VfsFile> value = std::make_pair(file.Name(), file);
	std::pair<VfsFiles::iterator, bool> ret = m_files.insert(value);
	if(!ret.second)	// already existed
	{
		VfsFile& previousFile = ret.first->second;
		const VfsFile& newFile = value.second;
		if(ShouldReplaceWith(previousFile, newFile))
			previousFile = newFile;
	}
	else
	{
		stats_vfs_file_add(file.Size());
	}

	return &(*ret.first).second;
}


// rationale: passing in a pre-constructed VfsDirectory and copying that into
// our map would be slower and less convenient for the caller.
VfsDirectory* VfsDirectory::AddSubdirectory(const VfsPath& name)
{
	std::pair<VfsPath, VfsDirectory> value = std::make_pair(name.string(), VfsDirectory());
	std::pair<VfsSubdirectories::iterator, bool> ret = m_subdirectories.insert(value);
	return &(*ret.first).second;
}


VfsFile* VfsDirectory::GetFile(const VfsPath& name)
{
	VfsFiles::iterator it = m_files.find(name.string());
	if(it == m_files.end())
		return 0;
	return &it->second;
}


VfsDirectory* VfsDirectory::GetSubdirectory(const VfsPath& name)
{
	VfsSubdirectories::iterator it = m_subdirectories.find(name.string());
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


void VfsDirectory::Invalidate(const VfsPath& name)
{
	m_files.erase(name.string());
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
	swprintf_s(buf, ARRAY_SIZE(buf), L"(%c; %6lu; %ls) %ls", file.Loader()->LocationCode(), (unsigned long)file.Size(), timestamp, file.Name().string().c_str());
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
		const VfsPath& name = it->first;
		const VfsDirectory& subdirectory = it->second;
		descriptions += indentation;
		descriptions += std::wstring(L"[") + name.string() + L"]\n";
		descriptions += FileDescriptions(subdirectory, indentLevel+1);

		DirectoryDescriptionR(descriptions, subdirectory, indentLevel+1);
	}
}
