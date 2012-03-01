/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_COLLADAMANAGER
#define INCLUDED_COLLADAMANAGER

#include "lib/file/vfs/vfs.h"

class CStr8;
class CColladaManagerImpl;
class MD5;

class CColladaManager
{
public:
	enum FileType { PMD, PSA };

	CColladaManager(const PIVFS& vfs);
	~CColladaManager();

	/**
	 * Returns the VFS path to a PMD/PSA file for the given source file.
	 * Performs a (cached) conversion from COLLADA if necessary.
	 *
	 * @param pathnameNoExtension path and name, minus extension, of file to load.
	 *		  One of either "sourceName.pmd" or "sourceName.dae" should exist.
	 * @param type FileType, .pmd or .psa
	 *
	 * @return full VFS path (including extension) of file to load; or empty
	 * string if there was a problem and it could not be loaded. Doesn't knowingly
	 * return an invalid path.
	 */
	VfsPath GetLoadablePath(const VfsPath& pathnameNoExtension, FileType type);

	/**
	 * Converts DAE to archive cached .pmd/psa and outputs the resulting path
	 * (used by archive builder)
	 *
	 * @param[in] sourcePath path of the .dae to load
	 * @param[in] type FileType, .pmd or .psa
	 * @param[out] archiveCachePath output path of the cached file
	 *
	 * @return true if COLLADA converter completed successfully; or false if it failed
	 */
	bool GenerateCachedFile(const VfsPath& sourcePath, FileType type, VfsPath& archiveCachePath);

private:
	/**
	 * Creates MD5 hash key from skeletons.xml info and COLLADA converter version,
	 * used to invalidate cached .pmd/psas
	 *
	 * @param[out] hash resulting MD5 hash
	 * @param[out] version version passed to CCacheLoader, used if code change should force
	 *		  cache invalidation
	 */
	void PrepareCacheKey(MD5& hash, u32& version);

	CColladaManagerImpl* m;
	PIVFS m_VFS;
};

#endif // INCLUDED_COLLADAMANAGER
