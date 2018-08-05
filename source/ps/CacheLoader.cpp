/* Copyright (C) 2018 Wildfire Games.
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

#include "precompiled.h"

#include "CacheLoader.h"

#include "ps/CLogger.h"
#include "maths/MD5.h"

#include <iomanip>

CCacheLoader::CCacheLoader(PIVFS vfs, const std::wstring& fileExtension) :
	m_VFS(vfs), m_FileExtension(fileExtension)
{
}

Status CCacheLoader::TryLoadingCached(const VfsPath& sourcePath, const MD5& initialHash, u32 version, VfsPath& loadPath)
{
	VfsPath archiveCachePath = ArchiveCachePath(sourcePath);

	// Try the archive cache file first
	if (CanUseArchiveCache(sourcePath, archiveCachePath))
	{
		loadPath = archiveCachePath;
		return INFO::OK;
	}

	// Fail if no source or archive cache
	// Note: this is not always an error case, because for instance there
	//	are some uncached .pmd/psa files in the game with no source .dae.
	//	This test fails (correctly) in that valid situation, so it seems
	//	best to leave the error handling to the caller.
	Status err = m_VFS->GetFileInfo(sourcePath, NULL);
	if (err < 0)
	{
		return err;
	}

	// Look for loose cache of source file

	VfsPath looseCachePath = LooseCachePath(sourcePath, initialHash, version);

	// If the loose cache file exists, use it
	if (m_VFS->GetFileInfo(looseCachePath, NULL) >= 0)
	{
		loadPath = looseCachePath;
		return INFO::OK;
	}

	// No cache - we'll need to regenerate it

	loadPath = looseCachePath;
	return INFO::SKIPPED;
}

bool CCacheLoader::CanUseArchiveCache(const VfsPath& sourcePath, const VfsPath& archiveCachePath)
{
	// We want to use the archive cache whenever possible,
	// unless it's superseded by a source file that the user has edited

	size_t archiveCachePriority = 0;
	size_t sourcePriority = 0;

	bool archiveCacheExists = (m_VFS->GetFilePriority(archiveCachePath, &archiveCachePriority) >= 0);

	// Can't use it if there's no cache
	if (!archiveCacheExists)
		return false;

	bool sourceExists = (m_VFS->GetFilePriority(sourcePath, &sourcePriority) >= 0);

	// Must use the cache if there's no source
	if (!sourceExists)
		return true;

	// If source file is from a higher-priority mod than archive cache,
	// don't use the old cache
	if (archiveCachePriority < sourcePriority)
		return false;

	// If source file is more recent than the archive cache (i.e. the user has edited it),
	// don't use the old cache
	CFileInfo sourceInfo, archiveCacheInfo;
	if (m_VFS->GetFileInfo(sourcePath, &sourceInfo) >= 0 &&
		m_VFS->GetFileInfo(archiveCachePath, &archiveCacheInfo) >= 0)
	{
		const double howMuchNewer = difftime(sourceInfo.MTime(), archiveCacheInfo.MTime());
		const double threshold = 2.0;	// FAT timestamp resolution [seconds]
		if (howMuchNewer > threshold)
			return false;
	}

	// Otherwise we can use the cache
	return true;
}

VfsPath CCacheLoader::ArchiveCachePath(const VfsPath& sourcePath) const
{
	return sourcePath.ChangeExtension(sourcePath.Extension().string() + L".cached" + m_FileExtension);
}

VfsPath CCacheLoader::LooseCachePath(const VfsPath& sourcePath, const MD5& initialHash, u32 version)
{
	CFileInfo fileInfo;
	if (m_VFS->GetFileInfo(sourcePath, &fileInfo) < 0)
	{
		debug_warn(L"source file disappeared"); // this should never happen
		return VfsPath();
	}

	u64 mtime = (u64)fileInfo.MTime() & ~1; // skip lowest bit, since zip and FAT don't preserve it
	u64 size = (u64)fileInfo.Size();

	// Construct a hash of the file data and settings.

	MD5 hash = initialHash;
	hash.Update((const u8*)&mtime, sizeof(mtime));
	hash.Update((const u8*)&size, sizeof(size));
	hash.Update((const u8*)&version, sizeof(version));
	// these are local cached files, so we don't care about endianness etc

	// Use a short prefix of the full hash (we don't need high collision-resistance),
	// converted to hex
	u8 digest[MD5::DIGESTSIZE];
	hash.Final(digest);
	std::wstringstream digestPrefix;
	digestPrefix << std::hex;
	for (size_t i = 0; i < 8; ++i)
		digestPrefix << std::setfill(L'0') << std::setw(2) << (int)digest[i];

	// Get the mod path
	OsPath path;
	m_VFS->GetRealPath(sourcePath, path);

	// Construct the final path
	return VfsPath("cache") / path_name_only(path.BeforeCommon(sourcePath).Parent().string().c_str()) / sourcePath.ChangeExtension(sourcePath.Extension().string() + L"." + digestPrefix.str() + m_FileExtension);
}
