/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_ARCHIVEBUILDER
#define INCLUDED_ARCHIVEBUILDER

#include "lib/file/vfs/vfs.h"
#include "ps/CStr.h"

/**
 * Packages a mod's files into a distributable archive.
 * This includes various game-specific knowledge on how to convert
 * and cache certain files into more efficient formats (PNG -> DDS, etc).
 */
class CArchiveBuilder
{
public:
	/**
	 * Initialise the archive builder for processing the given mod.
	 * Assumes no graphics code (especially tex_codec) has been initialised yet.
	 *
	 * @param mod path to data/mods/foo directory, containing files for conversion
	 * @param tempdir path to a writable directory for temporary files
	 */
	CArchiveBuilder(const OsPath& mod, const OsPath& tempdir);

	~CArchiveBuilder();

	/**
	 * Add a mod which will be loaded but not archived, to provide
	 * files like textures.xml needed for the conversion.
	 * Added mods will be mounted with increasing priority.
	 * Typically this will be called with 'public', when packaging
	 * a user's mod.
	 * @param mod path to data/mods/foo directory, containing files for loading
	 */
	void AddBaseMod(const OsPath& mod);

	/**
	 * Do all the processing and packing of files into the archive.
	 * @param archive path of .zip file to generate (will be overwritten if it exists)
	 * @param compress whether to compress the contents of the .zip file
	 */
	void Build(const OsPath& archive, bool compress);

private:
	static Status CollectFileCB(const VfsPath& pathname, const CFileInfo& fileInfo, const uintptr_t cbData);

	PIVFS m_VFS;
	std::vector<VfsPath> m_Files;
	OsPath m_TempDir;
	size_t m_NumBaseMods;
};

#endif // INCLUDED_ARCHIVEBUILDER
