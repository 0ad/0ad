/* Copyright (C) 2022 Wildfire Games.
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

#include "TerrainTextureManager.h"

#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainProperties.h"
#include "graphics/TextureManager.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/bits.h"
#include "lib/tex/tex.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/VideoMode.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/Renderer.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <vector>

CTerrainTextureManager::CTerrainTextureManager()
	: m_LastGroupIndex(0)
{
	if (!VfsDirectoryExists(L"art/terrains/"))
		return;
	if (!CXeromyces::AddValidator(g_VFS, "terrain", "art/terrains/terrain.rng"))
		LOGERROR("CTerrainTextureManager: failed to load grammar file 'art/terrains/terrain.rng'");
	if (!CXeromyces::AddValidator(g_VFS, "terrain_texture", "art/terrains/terrain_texture.rng"))
		LOGERROR("CTerrainTextureManager: failed to load grammar file 'art/terrains/terrain_texture.rng'");
}

CTerrainTextureManager::~CTerrainTextureManager()
{
	UnloadTerrainTextures();

	for (std::pair<const VfsPath, TerrainAlpha>& ta : m_TerrainAlphas)
		ta.second.m_CompositeAlphaMap.reset();
}

void CTerrainTextureManager::UnloadTerrainTextures()
{
	for (CTerrainTextureEntry* const& te : m_TextureEntries)
		delete te;
	m_TextureEntries.clear();

	for (const std::pair<const CStr, CTerrainGroup*>& tg : m_TerrainGroups)
		delete tg.second;
	m_TerrainGroups.clear();

	m_LastGroupIndex = 0;
}

CTerrainTextureEntry* CTerrainTextureManager::FindTexture(const CStr& tag_) const
{
	CStr tag = tag_.BeforeLast("."); // Strip extension

	for (CTerrainTextureEntry* const& te : m_TextureEntries)
		if (te->GetTag() == tag)
			return te;

	LOGWARNING("CTerrainTextureManager: Couldn't find terrain %s", tag.c_str());
	return 0;
}

CTerrainTextureEntry* CTerrainTextureManager::AddTexture(const CTerrainPropertiesPtr& props, const VfsPath& path)
{
	CTerrainTextureEntry* entry = new CTerrainTextureEntry(props, path);
	m_TextureEntries.push_back(entry);
	return entry;
}

void CTerrainTextureManager::DeleteTexture(CTerrainTextureEntry* entry)
{
	std::vector<CTerrainTextureEntry*>::iterator it = std::find(m_TextureEntries.begin(), m_TextureEntries.end(), entry);
	if (it != m_TextureEntries.end())
		m_TextureEntries.erase(it);

	delete entry;
}

struct AddTextureCallbackData
{
	CTerrainTextureManager* self;
	CTerrainPropertiesPtr props;
};

static Status AddTextureDirCallback(const VfsPath& pathname, const uintptr_t cbData)
{
	AddTextureCallbackData& data = *(AddTextureCallbackData*)cbData;
	VfsPath path = pathname / L"terrains.xml";
	if (!VfsFileExists(path))
		LOGMESSAGE("'%s' does not exist. Using previous properties.", path.string8());
	else
		data.props = CTerrainProperties::FromXML(data.props, path);

	return INFO::OK;
}

static Status AddTextureCallback(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	AddTextureCallbackData& data = *(AddTextureCallbackData*)cbData;
	if (pathname.Basename() != L"terrains")
		data.self->AddTexture(data.props, pathname);

	return INFO::OK;
}

int CTerrainTextureManager::LoadTerrainTextures()
{
	AddTextureCallbackData data = {this, CTerrainPropertiesPtr(new CTerrainProperties(CTerrainPropertiesPtr()))};
	vfs::ForEachFile(g_VFS, L"art/terrains/", AddTextureCallback, (uintptr_t)&data, L"*.xml", vfs::DIR_RECURSIVE, AddTextureDirCallback, (uintptr_t)&data);
	return 0;
}

CTerrainGroup* CTerrainTextureManager::FindGroup(const CStr& name)
{
	TerrainGroupMap::const_iterator it = m_TerrainGroups.find(name);
	if (it != m_TerrainGroups.end())
		return it->second;
	else
		return m_TerrainGroups[name] = new CTerrainGroup(name, ++m_LastGroupIndex);
}

// LoadAlphaMaps: load the 14 default alpha maps, pack them into one composite texture and
// calculate the coordinate of each alphamap within this packed texture.
CTerrainTextureManager::TerrainAlphaMap::iterator
CTerrainTextureManager::LoadAlphaMap(const VfsPath& alphaMapType)
{
	const std::wstring key = L"(alpha map composite" + alphaMapType.string() + L")";

	CTerrainTextureManager::TerrainAlphaMap::iterator it = m_TerrainAlphas.find(alphaMapType);

	if (it != g_TexMan.m_TerrainAlphas.end())
		return it;

	m_TerrainAlphas[alphaMapType] = TerrainAlpha();
	it = m_TerrainAlphas.find(alphaMapType);

	TerrainAlpha& result = it->second;

	//
	// load all textures and store Handle in array
	//
	Tex textures[NUM_ALPHA_MAPS] = {};
	const VfsPath path = VfsPath("art/textures/terrain/alphamaps") / alphaMapType;

	const wchar_t* fnames[NUM_ALPHA_MAPS] =
	{
		L"blendcircle.png",
		L"blendlshape.png",
		L"blendedge.png",
		L"blendedgecorner.png",
		L"blendedgetwocorners.png",
		L"blendfourcorners.png",
		L"blendtwooppositecorners.png",
		L"blendlshapecorner.png",
		L"blendtwocorners.png",
		L"blendcorner.png",
		L"blendtwoedges.png",
		L"blendthreecorners.png",
		L"blendushape.png",
		L"blendbad.png"
	};
	size_t base = 0;	// texture width/height (see below)
	// For convenience, we require all alpha maps to be of the same BPP.
	size_t bpp = 0;
	for (size_t i = 0; i < NUM_ALPHA_MAPS; ++i)
	{
		// note: these individual textures can be discarded afterwards;
		// we cache the composite.
		std::shared_ptr<u8> fileData;
		size_t fileSize;
		if (g_VFS->LoadFile(path / fnames[i], fileData, fileSize) != INFO::OK ||
			textures[i].decode(fileData, fileSize) != INFO::OK)
		{
			m_TerrainAlphas.erase(it);
			LOGERROR("Failed to load alphamap: %s", alphaMapType.string8());

			const VfsPath standard("standard");
			if (path != standard)
				return LoadAlphaMap(standard);
			return m_TerrainAlphas.end();
		}

		// Get its size and make sure they are all equal.
		// (the packing algo assumes this).
		if (textures[i].m_Width != textures[i].m_Height)
			DEBUG_DISPLAY_ERROR(L"Alpha maps are not square");
		// .. first iteration: establish size
		if (i == 0)
		{
			base = textures[i].m_Width;
			bpp = textures[i].m_Bpp;
		}
		// .. not first: make sure texture size matches
		else if (base != textures[i].m_Width || bpp != textures[i].m_Bpp)
			DEBUG_DISPLAY_ERROR(L"Alpha maps are not identically sized (including pixel depth)");
	}

	//
	// copy each alpha map (tile) into one buffer, arrayed horizontally.
	//
	const size_t tileWidth = 2 + base + 2;	// 2 pixel border (avoids bilinear filtering artifacts)
	const size_t totalWidth = round_up_to_pow2(tileWidth * NUM_ALPHA_MAPS);
	const size_t totalHeight = base; ENSURE(is_pow2(totalHeight));
	std::shared_ptr<u8> data;
	AllocateAligned(data, totalWidth * totalHeight, maxSectorSize);
	// for each tile on row
	for (size_t i = 0; i < NUM_ALPHA_MAPS; ++i)
	{
		// get src of copy
		u8* src = textures[i].get_data();
		ENSURE(src);

		const size_t srcStep = bpp / 8;

		// get destination of copy
		u8* dst = data.get() + (i * tileWidth);

		// for each row of image
		for (size_t j = 0; j < base; ++j)
		{
			// duplicate first pixel
			*dst++ = *src;
			*dst++ = *src;

			// copy a row
			for (size_t k = 0; k < base; ++k)
			{
				*dst++ = *src;
				src += srcStep;
			}

			// duplicate last pixel
			*dst++ = *(src - srcStep);
			*dst++ = *(src - srcStep);

			// advance write pointer for next row
			dst += totalWidth - tileWidth;
		}

		result.m_AlphaMapCoords[i].u0 = static_cast<float>(i * tileWidth + 2) / totalWidth;
		result.m_AlphaMapCoords[i].u1 = static_cast<float>((i + 1) * tileWidth - 2) / totalWidth;
		result.m_AlphaMapCoords[i].v0 = 0.0f;
		result.m_AlphaMapCoords[i].v1 = 1.0f;
	}

	for (size_t i = 0; i < NUM_ALPHA_MAPS; ++i)
		textures[i].free();

	// Enable the following to save a png of the generated texture
	// in the public/ directory, for debugging.
#if 0
	Tex t;
	ignore_result(t.wrap(totalWidth, totalHeight, 8, TEX_GREY, data, 0));

	const VfsPath filename("blendtex.png");

	DynArray da;
	RETURN_STATUS_IF_ERR(tex_encode(&t, filename.Extension(), &da));

	// write to disk
	//Status ret = INFO::OK;
	{
		std::shared_ptr<u8> file = DummySharedPtr(da.base);
		const ssize_t bytes_written = g_VFS->CreateFile(filename, file, da.pos);
		if (bytes_written > 0)
			ENSURE(bytes_written == (ssize_t)da.pos);
		//else
		//	ret = (Status)bytes_written;
	}

	ignore_result(da_free(&da));
#endif

	result.m_CompositeAlphaMap = g_VideoMode.GetBackendDevice()->CreateTexture2D("CompositeAlphaMap",
		Renderer::Backend::Format::A8_UNORM, totalWidth, totalHeight,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

	result.m_CompositeDataToUpload = std::move(data);

	m_AlphaMapsToUpload.emplace_back(it);

	return it;
}

void CTerrainTextureManager::UploadResourcesIfNeeded(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	for (const CTerrainTextureManager::TerrainAlphaMap::iterator& it : m_AlphaMapsToUpload)
	{
		TerrainAlpha& alphaMap = it->second;
		if (!alphaMap.m_CompositeDataToUpload)
			continue;
		// Upload the composite texture.
		Renderer::Backend::GL::CTexture* texture = alphaMap.m_CompositeAlphaMap.get();
		deviceCommandContext->UploadTexture(
			texture, Renderer::Backend::Format::A8_UNORM, alphaMap.m_CompositeDataToUpload.get(),
			texture->GetWidth() * texture->GetHeight());
		alphaMap.m_CompositeDataToUpload.reset();
	}

	m_AlphaMapsToUpload.clear();
}

void CTerrainGroup::AddTerrain(CTerrainTextureEntry* pTerrain)
{
	m_Terrains.push_back(pTerrain);
}

void CTerrainGroup::RemoveTerrain(CTerrainTextureEntry* pTerrain)
{
	std::vector<CTerrainTextureEntry*>::iterator it = find(m_Terrains.begin(), m_Terrains.end(), pTerrain);
	if (it != m_Terrains.end())
		m_Terrains.erase(it);
}
