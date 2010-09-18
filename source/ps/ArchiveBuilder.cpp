/* Copyright (C) 2010 Wildfire Games.
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

#include "ArchiveBuilder.h"

#include "graphics/TextureManager.h"
#include "lib/path_util.h"
#include "lib/tex/tex_codec.h"
#include "lib/file/archive/archive_zip.h"

#include <boost/algorithm/string.hpp>

CArchiveBuilder::CArchiveBuilder(const fs::wpath& mod, const fs::wpath& tempdir) :
	m_TempDir(tempdir)
{
	tex_codec_register_all();

	m_VFS = CreateVfs(20*MiB);

	DeleteDirectory(m_TempDir/L"_archivecache"); // clean up in case the last run failed

	m_VFS->Mount(L"cache/", m_TempDir/L"_archivecache/");

	m_VFS->Mount(L"", AddSlash(mod), VFS_MOUNT_MUST_EXIST);

	// Collect the list of files before loading any base mods
	fs_util::ForEachFile(m_VFS, L"", &CollectFileCB, (uintptr_t)static_cast<void*>(this), 0, fs_util::DIR_RECURSIVE);
}

CArchiveBuilder::~CArchiveBuilder()
{
	m_VFS.reset();

	DeleteDirectory(m_TempDir/L"_archivecache");

	tex_codec_unregister_all();
}

void CArchiveBuilder::AddBaseMod(const fs::wpath& mod)
{
	m_VFS->Mount(L"", AddSlash(mod), VFS_MOUNT_MUST_EXIST);
}

void CArchiveBuilder::Build(const fs::wpath& archive)
{
	PIArchiveWriter writer = CreateArchiveWriter_Zip(archive);

	// Use CTextureManager instead of CTextureConverter directly,
	// so it can deal with all the loading of settings.xml files
	CTextureManager texman(m_VFS, true);

	for (size_t i = 0; i < m_Files.size(); ++i)
	{
		LibError ret;

		fs::wpath realPath;
		ret = m_VFS->GetRealPath(m_Files[i], realPath);
		debug_assert(ret == INFO::OK);

		// Compress textures and store the new cached version instead of the original
		if (boost::algorithm::starts_with(m_Files[i].string(), L"art/textures/") &&
			tex_is_known_extension(m_Files[i]) &&
			// Skip some subdirectories where the engine doesn't use CTextureManager yet:
			!boost::algorithm::starts_with(m_Files[i].string(), L"art/textures/cursors/") &&
			!boost::algorithm::starts_with(m_Files[i].string(), L"art/textures/terrain/alphamaps/special/")
		)
		{
			VfsPath cachedPath;
			debug_printf(L"Converting texture %ls\n", realPath.string().c_str());
			bool ok = texman.GenerateCachedTexture(m_Files[i], cachedPath);
			debug_assert(ok);

			fs::wpath cachedRealPath;
			ret = m_VFS->GetRealPath(L"cache"/cachedPath, cachedRealPath);
			debug_assert(ret == INFO::OK);

			writer->AddFile(cachedRealPath, cachedPath.string());
		}
		// TODO: should cache XML->XMB and DAE->PMD and DAE->PSA conversions too
		else
		{
			debug_printf(L"Adding %ls\n", realPath.string().c_str());
			writer->AddFile(realPath, m_Files[i].string());
		}
	}
}

LibError CArchiveBuilder::CollectFileCB(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	CArchiveBuilder* self = static_cast<CArchiveBuilder*>((void*)cbData);
	self->m_Files.push_back(pathname);

	return INFO::OK;
}
