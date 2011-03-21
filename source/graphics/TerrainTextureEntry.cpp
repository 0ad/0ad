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

#include <map>

#include "lib/ogl.h"
#include "lib/path_util.h"
#include "lib/res/graphics/ogl_tex.h"

#include "TerrainTextureEntry.h"
#include "TerrainTextureManager.h"
#include "TerrainProperties.h"
#include "Texture.h"
#include "renderer/Renderer.h"

CTerrainTextureEntry::CTerrainTextureEntry(CTerrainPropertiesPtr props, const VfsPath& path):
	m_pProperties(props),
	m_BaseColor(0),
	m_BaseColorValid(false)
{
	debug_assert(props);

	CTextureProperties texture(path);
	texture.SetWrap(GL_REPEAT);

	// TODO: anisotropy should probably be user-configurable, but we want it to be
	// at least 2 for terrain else the ground looks very blurry when you tilt the
	// camera upwards
	texture.SetMaxAnisotropy(2.0f);

	if (CRenderer::IsInitialised())
		m_Texture = g_Renderer.GetTextureManager().CreateTexture(texture);

	if (m_pProperties)
		m_Groups = m_pProperties->GetGroups();
	
	GroupVector::iterator it=m_Groups.begin();
	for (;it!=m_Groups.end();++it)
		(*it)->AddTerrain(this);
	
	m_Tag = CStrW(Path::Basename(path)).ToUTF8();
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
