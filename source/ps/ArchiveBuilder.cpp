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
#include "ps/XML/Xeromyces.h"

// Disable "'boost::algorithm::detail::is_classifiedF' : assignment operator could not be generated"
#if MSC_VERSION
#pragma warning(disable:4512)
#endif

#include <boost/algorithm/string.hpp>

CArchiveBuilder::CArchiveBuilder(const std::wstring& mod, const NativePath& tempdir) :
	m_TempDir(tempdir)
{
	tex_codec_register_all();

	m_VFS = CreateVfs(20*MiB);

	DeleteDirectory(Path::Join(m_TempDir, "_archivecache")); // clean up in case the last run failed

	m_VFS->Mount(L"cache/", Path::Join(m_TempDir, "_archivecache/"));

	m_VFS->Mount(L"", Path::AddSlash(mod), VFS_MOUNT_MUST_EXIST);

	// Collect the list of files before loading any base mods
	fs_util::ForEachFile(m_VFS, L"", &CollectFileCB, (uintptr_t)static_cast<void*>(this), 0, fs_util::DIR_RECURSIVE);
}

CArchiveBuilder::~CArchiveBuilder()
{
	m_VFS.reset();

	DeleteDirectory(Path::Join(m_TempDir, "_archivecache"));

	tex_codec_unregister_all();
}

void CArchiveBuilder::AddBaseMod(const NativePath& mod)
{
	m_VFS->Mount(L"", Path::AddSlash(mod), VFS_MOUNT_MUST_EXIST);
}

void CArchiveBuilder::Build(const NativePath& archive)
{
	// Disable zip compression because it significantly hurts download size
	// for releases (which re-compress all files with better compression
	// algorithms) - it's probably most important currently to optimise for
	// download size rather than install size or startup performance.
	// (See http://trac.wildfiregames.com/ticket/671)
	const bool noDeflate = true;

	PIArchiveWriter writer = CreateArchiveWriter_Zip(archive, noDeflate);

	// Use CTextureManager instead of CTextureConverter directly,
	// so it can deal with all the loading of settings.xml files
	CTextureManager texman(m_VFS, true, true);

	CXeromyces xero;

	for (size_t i = 0; i < m_Files.size(); ++i)
	{
		LibError ret;

		NativePath realPath;
		ret = m_VFS->GetRealPath(m_Files[i], realPath);
		debug_assert(ret == INFO::OK);

		// Compress textures and store the new cached version instead of the original
		if (boost::algorithm::starts_with(m_Files[i], L"art/textures/") &&
			tex_is_known_extension(m_Files[i]) &&
			// Skip some subdirectories where the engine doesn't use CTextureManager yet:
			!boost::algorithm::starts_with(m_Files[i], L"art/textures/cursors/") &&
			!boost::algorithm::starts_with(m_Files[i], L"art/textures/terrain/alphamaps/")
		)
		{
			VfsPath cachedPath;
			debug_printf(L"Converting texture %ls\n", realPath.c_str());
			bool ok = texman.GenerateCachedTexture(m_Files[i], cachedPath);
			debug_assert(ok);

			std::wstring cachedRealPath;
			ret = m_VFS->GetRealPath(Path::Join("cache", cachedPath), cachedRealPath);
			debug_assert(ret == INFO::OK);

			writer->AddFile(cachedRealPath, cachedPath);

			// We don't want to store the original file too (since it's a
			// large waste of space), so skip to the next file
			continue;
		}

		// TODO: should cache DAE->PMD and DAE->PSA conversions too

		debug_printf(L"Adding %ls\n", realPath.c_str());
		writer->AddFile(realPath, m_Files[i]);

		// Also cache XMB versions of all XML files
		if (Path::Extension(m_Files[i]) == L".xml")
		{
			VfsPath cachedPath;
			debug_printf(L"Converting XML file %ls\n", realPath.c_str());
			bool ok = xero.GenerateCachedXMB(m_VFS, m_Files[i], cachedPath);
			debug_assert(ok);

			NativePath cachedRealPath;
			ret = m_VFS->GetRealPath(Path::Join("cache", cachedPath), cachedRealPath);
			debug_assert(ret == INFO::OK);

			writer->AddFile(cachedRealPath, cachedPath);
		}
	}
}

LibError CArchiveBuilder::CollectFileCB(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	CArchiveBuilder* self = static_cast<CArchiveBuilder*>((void*)cbData);
	self->m_Files.push_back(pathname);

	return INFO::OK;
}
