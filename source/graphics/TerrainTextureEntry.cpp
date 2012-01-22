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

#include "precompiled.h"

#include "TerrainTextureEntry.h"

#include "lib/utf8.h"
#include "lib/ogl.h"
#include "lib/res/graphics/ogl_tex.h"

#include "graphics/Terrain.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerrainProperties.h"
#include "graphics/Texture.h"
#include "renderer/Renderer.h"

#include <map>

CTerrainTextureEntry::CTerrainTextureEntry(CTerrainPropertiesPtr props, const VfsPath& path):
	m_pProperties(props),
	m_BaseColor(0),
	m_BaseColorValid(false)
{
	ENSURE(props);

	CTextureProperties texture(path);
	texture.SetWrap(GL_REPEAT);

	// TODO: anisotropy should probably be user-configurable, but we want it to be
	// at least 2 for terrain else the ground looks very blurry when you tilt the
	// camera upwards
	texture.SetMaxAnisotropy(2.0f);

	if (CRenderer::IsInitialised())
		m_Texture = g_Renderer.GetTextureManager().CreateTexture(texture);

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
	
	m_Tag = utf8_from_wstring(path.Basename().string());
}

CTerrainTextureEntry::~CTerrainTextureEntry()
{
	for (GroupVector::iterator it=m_Groups.begin();it!=m_Groups.end();++it)
		(*it)->RemoveTerrain(this);
}

// BuildBaseColor: calculate the root colour of the texture, used for coloring minimap, and store
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

	// Use the texture colour if available
	if (m_Texture->TryLoad())
	{
		m_BaseColor = m_Texture->GetBaseColour();
		m_BaseColorValid = true;
	}
}

const float* CTerrainTextureEntry::GetTextureMatrix()
{
	return &m_TextureMatrix._11;
}