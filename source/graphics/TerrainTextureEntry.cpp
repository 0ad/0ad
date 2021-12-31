/* Copyright (C) 2021 Wildfire Games.
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

#include "TerrainTextureEntry.h"

#include "graphics/MaterialManager.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainProperties.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/Texture.h"
#include "lib/allocators/shared_ptr.h"
#include "lib/file/io/io.h"
#include "lib/ogl.h"
#include "lib/tex/tex.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"

#include <map>

CTerrainTextureEntry::CTerrainTextureEntry(CTerrainPropertiesPtr properties, const VfsPath& path):
	m_pProperties(properties),
	m_BaseColor(0),
	m_BaseColorValid(false)
{
	ENSURE(properties);

	CXeromyces XeroFile;
	if (XeroFile.Load(g_VFS, path, "terrain_texture") != PSRETURN_OK)
	{
		LOGERROR("Terrain xml not found (%s)", path.string8());
		return;
	}

	#define EL(x) int el_##x = XeroFile.GetElementID(#x)
	#define AT(x) int at_##x = XeroFile.GetAttributeID(#x)
	EL(tag);
	EL(terrain);
	EL(texture);
	EL(textures);
	EL(material);
	EL(props);
	EL(alphamap);
	AT(file);
	AT(name);
	#undef AT
	#undef EL

	XMBElement root = XeroFile.GetRoot();

	if (root.GetNodeName() != el_terrain)
	{
		LOGERROR("Invalid terrain format (unrecognised root element '%s')", XeroFile.GetElementString(root.GetNodeName()));
		return;
	}

	std::vector<std::pair<CStr, VfsPath> > samplers;
	VfsPath alphamap("standard");
	m_Tag = utf8_from_wstring(path.Basename().string());

	XERO_ITER_EL(root, child)
	{
		int child_name = child.GetNodeName();

		if (child_name == el_textures)
		{
			XERO_ITER_EL(child, textures_element)
			{
				ENSURE(textures_element.GetNodeName() == el_texture);

				CStr name;
				VfsPath terrainTexturePath;
				XERO_ITER_ATTR(textures_element, relativePath)
				{
					if (relativePath.Name == at_file)
						terrainTexturePath = VfsPath("art/textures/terrain") / relativePath.Value.FromUTF8();
					else if (relativePath.Name == at_name)
						name = relativePath.Value;
				}
				samplers.emplace_back(name, terrainTexturePath);
				if (name == str_baseTex.string())
					m_DiffuseTexturePath = terrainTexturePath;
			}

		}
		else if (child_name == el_material)
		{
			VfsPath mat = VfsPath("art/materials") / child.GetText().FromUTF8();
			if (CRenderer::IsInitialised())
				m_Material = g_Renderer.GetMaterialManager().LoadMaterial(mat);
		}
		else if (child_name == el_alphamap)
		{
			alphamap = child.GetText().FromUTF8();
		}
		else if (child_name == el_props)
		{
			CTerrainPropertiesPtr ret (new CTerrainProperties(properties));
			ret->LoadXml(child, &XeroFile, path);
			if (ret) m_pProperties = ret;
		}
		else if (child_name == el_tag)
		{
			m_Tag = child.GetText();
		}
	}

	for (size_t i = 0; i < samplers.size(); ++i)
	{
		CTextureProperties texture(samplers[i].second);
		texture.SetWrap(GL_REPEAT);

		// TODO: anisotropy should probably be user-configurable, but we want it to be
		// at least 2 for terrain else the ground looks very blurry when you tilt the
		// camera upwards
		texture.SetMaxAnisotropy(2.0f);

		if (CRenderer::IsInitialised())
		{
			CTexturePtr texptr = g_Renderer.GetTextureManager().CreateTexture(texture);
			m_Material.AddSampler(CMaterial::TextureSampler(samplers[i].first, texptr));
		}
	}

	if (CRenderer::IsInitialised())
		LoadAlphaMaps(alphamap);

	float texAngle = 0.f;
	float texSize = 1.f;

	if (m_pProperties)
	{
		m_Groups = m_pProperties->GetGroups();
		texAngle = m_pProperties->GetTextureAngle();
		texSize = m_pProperties->GetTextureSize();
	}

	m_TextureMatrix.SetZero();
	m_TextureMatrix._11 = cosf(texAngle) / texSize;
	m_TextureMatrix._13 = -sinf(texAngle) / texSize;
	m_TextureMatrix._21 = -sinf(texAngle) / texSize;
	m_TextureMatrix._23 = -cosf(texAngle) / texSize;
	m_TextureMatrix._44 = 1.f;

	GroupVector::iterator it=m_Groups.begin();
	for (;it!=m_Groups.end();++it)
		(*it)->AddTerrain(this);
}

CTerrainTextureEntry::~CTerrainTextureEntry()
{
	for (GroupVector::iterator it=m_Groups.begin();it!=m_Groups.end();++it)
		(*it)->RemoveTerrain(this);
}

// BuildBaseColor: calculate the root color of the texture, used for coloring minimap, and store
// in m_BaseColor member
void CTerrainTextureEntry::BuildBaseColor()
{
	// Use the explicit properties value if possible
	if (m_pProperties && m_pProperties->HasBaseColor())
	{
		m_BaseColor=m_pProperties->GetBaseColor();
		m_BaseColorValid = true;
		return;
	}

	// Use the texture color if available
	if (GetTexture()->TryLoad())
	{
		m_BaseColor = GetTexture()->GetBaseColor();
		m_BaseColorValid = true;
	}
}

const float* CTerrainTextureEntry::GetTextureMatrix() const
{
	return &m_TextureMatrix._11;
}

// LoadAlphaMaps: load the 14 default alpha maps, pack them into one composite texture and
// calculate the coordinate of each alphamap within this packed texture
void CTerrainTextureEntry::LoadAlphaMaps(const VfsPath& alphaMapType)
{
	const std::wstring key = L"(alpha map composite" + alphaMapType.string() + L")";

	CTerrainTextureManager::TerrainAlphaMap::iterator it = g_TexMan.m_TerrainAlphas.find(alphaMapType);

	if (it != g_TexMan.m_TerrainAlphas.end())
	{
		m_TerrainAlpha = it;
		return;
	}

	g_TexMan.m_TerrainAlphas[alphaMapType] = TerrainAlpha();
	it = g_TexMan.m_TerrainAlphas.find(alphaMapType);

	TerrainAlpha &result = it->second;

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
	// for convenience, we require all alpha maps to be of the same BPP
	// (avoids another ogl_tex_get_size call, and doesn't hurt)
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
			g_TexMan.m_TerrainAlphas.erase(it);
			LOGERROR("Failed to load alphamap: %s", alphaMapType.string8());

			const VfsPath standard("standard");
			if (path != standard)
				LoadAlphaMaps(standard);
			return;
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
		if(bytes_written > 0)
			ENSURE(bytes_written == (ssize_t)da.pos);
		//else
		//	ret = (Status)bytes_written;
	}

	ignore_result(da_free(&da));
#endif

	result.m_CompositeAlphaMap = Renderer::Backend::GL::CTexture::Create2D(
		Renderer::Backend::Format::A8, totalWidth, totalHeight,
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));

	// Upload the composite texture.
	g_Renderer.BindTexture(0, result.m_CompositeAlphaMap->GetHandle());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, totalWidth, totalHeight, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data.get());
	g_Renderer.BindTexture(0, 0);

	m_TerrainAlpha = it;
}
