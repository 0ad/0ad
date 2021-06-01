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

#include "MiniMapTexture.h"

#include "graphics/MiniPatch.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerritoryTexture.h"
#include "lib/bits.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/Object.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpMinimap.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/system/ParamNode.h"

namespace
{

unsigned int ScaleColor(unsigned int color, float x)
{
	unsigned int r = unsigned(float(color & 0xff) * x);
	unsigned int g = unsigned(float((color>>8) & 0xff) * x);
	unsigned int b = unsigned(float((color>>16) & 0xff) * x);
	return (0xff000000 | b | g<<8 | r<<16);
}

} // anonymous namespace

CMiniMapTexture::CMiniMapTexture(CSimulation2& simulation)
	: m_Simulation(simulation)
{
	// Register Relax NG validator.
	CXeromyces::AddValidator(g_VFS, "pathfinder", "simulation/data/pathfinder.rng");

	m_ShallowPassageHeight = GetShallowPassageHeight();
}

CMiniMapTexture::~CMiniMapTexture()
{
	DestroyTextures();
}

void CMiniMapTexture::Update(const float UNUSED(deltaRealTime))
{
	if (m_WaterHeight != g_Renderer.GetWaterManager()->m_WaterHeight)
		m_Dirty = true;
}

void CMiniMapTexture::Render()
{
	if (!m_Dirty)
		return;

	const CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	if (!terrain)
		return;

	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);

	m_MapSize = terrain->GetVerticesPerSide();
	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize);

	if (!m_TerrainTexture)
		CreateTextures();

	RebuildTerrainTexture(terrain);
}

void CMiniMapTexture::CreateTextures()
{
	DestroyTextures();

	// Create terrain texture
	glGenTextures(1, &m_TerrainTexture);
	g_Renderer.BindTexture(0, m_TerrainTexture);

	// Initialise texture with solid black, for the areas we don't
	// overwrite with glTexSubImage2D later
	u32* texData = new u32[m_TextureSize * m_TextureSize];
	for (ssize_t i = 0; i < m_TextureSize * m_TextureSize; ++i)
		texData[i] = 0xFF000000;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_TextureSize, m_TextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
	delete[] texData;

	m_TerrainData = new u32[(m_MapSize - 1) * (m_MapSize - 1)];
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void CMiniMapTexture::DestroyTextures()
{
	if (m_TerrainTexture)
	{
		glDeleteTextures(1, &m_TerrainTexture);
		m_TerrainTexture = 0;
	}

	SAFE_ARRAY_DELETE(m_TerrainData);
}

void CMiniMapTexture::RebuildTerrainTexture(const CTerrain* terrain)
{
	u32 x = 0;
	u32 y = 0;
	u32 w = m_MapSize - 1;
	u32 h = m_MapSize - 1;
	m_WaterHeight = g_Renderer.GetWaterManager()->m_WaterHeight;

	m_Dirty = false;

	for (u32 j = 0; j < h; ++j)
	{
		u32* dataPtr = m_TerrainData + ((y + j) * (m_MapSize - 1)) + x;
		for (u32 i = 0; i < w; ++i)
		{
			float avgHeight = ( terrain->GetVertexGroundLevel((int)i, (int)j)
					+ terrain->GetVertexGroundLevel((int)i+1, (int)j)
					+ terrain->GetVertexGroundLevel((int)i, (int)j+1)
					+ terrain->GetVertexGroundLevel((int)i+1, (int)j+1)
				) / 4.0f;

			if (avgHeight < m_WaterHeight && avgHeight > m_WaterHeight - m_ShallowPassageHeight)
			{
				// shallow water
				*dataPtr++ = 0xffc09870;
			}
			else if (avgHeight < m_WaterHeight)
			{
				// Set water as constant color for consistency on different maps
				*dataPtr++ = 0xffa07850;
			}
			else
			{
				int hmap = ((int)terrain->GetHeightMap()[(y + j) * m_MapSize + x + i]) >> 8;
				int val = (hmap / 3) + 170;

				u32 color = 0xFFFFFFFF;

				CMiniPatch* mp = terrain->GetTile(x + i, y + j);
				if (mp)
				{
					CTerrainTextureEntry* tex = mp->GetTextureEntry();
					if (tex)
					{
						// If the texture can't be loaded yet, set the dirty flags
						// so we'll try regenerating the terrain texture again soon
						if(!tex->GetTexture()->TryLoad())
							m_Dirty = true;

						color = tex->GetBaseColor();
					}
				}

				*dataPtr++ = ScaleColor(color, float(val) / 255.0f);
			}
		}
	}

	// Upload the texture
	g_Renderer.BindTexture(0, m_TerrainTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize - 1, m_MapSize - 1, GL_RGBA, GL_UNSIGNED_BYTE, m_TerrainData);
	g_Renderer.BindTexture(0, 0);
}

// static
float CMiniMapTexture::GetShallowPassageHeight()
{
	float shallowPassageHeight = 0.0f;
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml", "pathfinder");
	const CParamNode pathingSettings = externalParamNode.GetChild("Pathfinder").GetChild("PassabilityClasses");
	if (pathingSettings.GetChild("default").IsOk() && pathingSettings.GetChild("default").GetChild("MaxWaterDepth").IsOk())
		shallowPassageHeight = pathingSettings.GetChild("default").GetChild("MaxWaterDepth").ToFloat();
	return shallowPassageHeight;
}
