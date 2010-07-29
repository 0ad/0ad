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

#ifndef INCLUDED_COLLADAMANAGER
#define INCLUDED_COLLADAMANAGER

#include "lib/file/vfs/vfs_path.h"

class CStr8;
class CColladaManagerImpl;

class CColladaManager
{
public:
	enum FileType { PMD, PSA };

	CColladaManager();
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
	 * string if there was a problem and it could not be loaded.
	 */
	VfsPath GetLoadableFilename(const VfsPath& pathnameNoExtension, FileType type);

private:
	CColladaManagerImpl* m;
};

#endif // INCLUDED_COLLADAMANAGER
