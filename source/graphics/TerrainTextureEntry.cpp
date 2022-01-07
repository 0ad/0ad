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

#include "TerrainTextureEntry.h"

#include "graphics/MaterialManager.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainProperties.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TextureManager.h"
#include "lib/ogl.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"

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
				m_Material = g_Renderer.GetSceneRenderer().GetMaterialManager().LoadMaterial(mat);
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
		m_TerrainAlpha = g_TexMan.LoadAlphaMap(alphamap);

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
