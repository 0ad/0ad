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

#ifndef INCLUDED_CACHELOADER
#define INCLUDED_CACHELOADER

#include "lib/file/vfs/vfs.h"

class MD5;

/**
 * Helper class for systems that have an expensive cacheable conversion process
 * when loading files.
 *
 * Conversion output can be automatically cached as loose files, indexed by a hash
 * of the file's timestamp and size plus any other data the caller provides.
 * This allows developers and modders to easily produce new files, with the conversion
 * happening transparently.
 *
 * For release packages, files can be precached by appending ".cached.{extension}"
 * to their name, which will be used instead of doing runtime conversion.
 * These cache files will typically be packed into an archive for faster loading;
 * if no archive cache is available then the source file will be converted and stored
 * as a loose cache file instead.
 */
class CCacheLoader
{
public:
	CCacheLoader(PIVFS vfs, const std::wstring& fileExtension);

	/**
	 * Attempts to find a valid cached which can be loaded.
	 * Returns INFO::OK and sets loadPath to the cached file if there is one.
	 * Returns INFO::SKIPPED and sets loadPath to the desire loose cache name if there isn't one.
	 * Returns a value < 0 on error (e.g. the source file doesn't exist). No error is logged or thrown.
	 */
	Status TryLoadingCached(const VfsPath& sourcePath, const MD5& initialHash, u32 version, VfsPath& loadPath);

	/**
	 * Determines whether we can safely use the archived cache file, or need to
	 * re-convert the source file.
	 */
	bool CanUseArchiveCache(const VfsPath& sourcePath, const VfsPath& archiveCachePath);

	/**
	 * Return the path of the archive cache for the given source file.
	 */
	VfsPath ArchiveCachePath(const VfsPath& sourcePath) const;

	/**
	 * Return the path of the loose cache for the given source file.
	 */
	VfsPath LooseCachePath(const VfsPath& sourcePath, const MD5& initialHash, u32 version);

private:
	PIVFS m_VFS;
	std::wstring m_FileExtension;
};

#endif // INCLUDED_CACHELOADER
