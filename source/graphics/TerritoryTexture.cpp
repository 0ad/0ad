/* Copyright (C) 2013 Wildfire Games.
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

#include "TerritoryTexture.h"

#include "graphics/Terrain.h"
#include "lib/bits.h"
#include "ps/Overlay.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpTerritoryManager.h"

// TODO: There's a lot of duplication with CLOSTexture - might be nice to refactor a bit

CTerritoryTexture::CTerritoryTexture(CSimulation2& simulation) :
	m_Simulation(simulation), m_DirtyID(0), m_Texture(0), m_MapSize(0), m_TextureSize(0)
{
}

CTerritoryTexture::~CTerritoryTexture()
{
	if (m_Texture)
		DeleteTexture();
}

void CTerritoryTexture::DeleteTexture()
{
	glDeleteTextures(1, &m_Texture);
	m_Texture = 0;
}

bool CTerritoryTexture::UpdateDirty()
{
	CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerritoryManager)
		return false;

	return cmpTerritoryManager->NeedUpdate(&m_DirtyID);
}

void CTerritoryTexture::BindTexture(int unit)
{
	if (UpdateDirty())
		RecomputeTexture(unit);

	g_Renderer.BindTexture(unit, m_Texture);
}

GLuint CTerritoryTexture::GetTexture()
{
	if (UpdateDirty())
		RecomputeTexture(0);

	return m_Texture;
}

const float* CTerritoryTexture::GetTextureMatrix()
{
	ENSURE(!UpdateDirty());
	return &m_TextureMatrix._11;
}

const CMatrix3D* CTerritoryTexture::GetMinimapTextureMatrix()
{
	ENSURE(!UpdateDirty());
	return &m_MinimapTextureMatrix;
}

void CTerritoryTexture::ConstructTexture(int unit)
{
	CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return;

	m_MapSize = cmpTerrain->GetVerticesPerSide() - 1;

	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize);

	glGenTextures(1, &m_Texture);
	g_Renderer.BindTexture(unit, m_Texture);

	// Initialise texture with transparency, for the areas we don't
	// overwrite with glTexSubImage2D later
	u8* texData = new u8[m_TextureSize * m_TextureSize * 4];
	memset(texData, 0x00, m_TextureSize * m_TextureSize * 4);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_TextureSize, m_TextureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
	delete[] texData;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	{
		// Texture matrix: We want to map
		//   world pos (0, y, 0)  (i.e. bottom-left of first tile)
		//     onto texcoord (0, 0)  (i.e. bottom-left of first texel);
		//   world pos (mapsize*cellsize, y, mapsize*cellsize)  (i.e. top-right of last tile)
		//     onto texcoord (mapsize / texsize, mapsize / texsize)  (i.e. top-right of last texel)

		float s = 1.f / (float)(m_TextureSize * TERRAIN_TILE_SIZE);
		float t = 0.f;
		m_TextureMatrix.SetZero();
		m_TextureMatrix._11 = s;
		m_TextureMatrix._23 = s;
		m_TextureMatrix._14 = t;
		m_TextureMatrix._24 = t;
		m_TextureMatrix._44 = 1;
	}

	{
		// Minimap matrix: We want to map UV (0,0)-(1,1) onto (0,0)-(mapsize/texsize, mapsize/texsize)

		float s = m_MapSize / (float)m_TextureSize;
		m_MinimapTextureMatrix.SetZero();
		m_MinimapTextureMatrix._11 = s;
		m_MinimapTextureMatrix._22 = s;
		m_MinimapTextureMatrix._44 = 1;
	}
}

void CTerritoryTexture::RecomputeTexture(int unit)
{
	// If the map was resized, delete and regenerate the texture
	if (m_Texture)
	{
		CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
		if (cmpTerrain && m_MapSize != (ssize_t)cmpTerrain->GetVerticesPerSide())
			DeleteTexture();
	}

	if (!m_Texture)
		ConstructTexture(unit);

	PROFILE("recompute territory texture");

	std::vector<u8> bitmap;
	bitmap.resize(m_MapSize * m_MapSize * 4);

	CmpPtr<ICmpTerritoryManager> cmpTerritoryManager(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerritoryManager)
		return;

	const Grid<u8> territories = cmpTerritoryManager->GetTerritoryGrid();

	GenerateBitmap(territories, &bitmap[0], m_MapSize, m_MapSize);

	g_Renderer.BindTexture(unit, m_Texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize, m_MapSize, GL_RGBA, GL_UNSIGNED_BYTE, &bitmap[0]);
}

void CTerritoryTexture::GenerateBitmap(const Grid<u8>& territories, u8* bitmap, ssize_t w, ssize_t h)
{
	int alphaMax = 0xC0;
	int alphaFalloff = 0x20;

	CmpPtr<ICmpPlayerManager> cmpPlayerManager(m_Simulation, SYSTEM_ENTITY);

	std::vector<CColor> colors;
	i32 numPlayers = cmpPlayerManager->GetNumPlayers();
	for (i32 p = 0; p < numPlayers; ++p)
	{
		CColor color(1, 0, 1, 1);
		CmpPtr<ICmpPlayer> cmpPlayer(m_Simulation, cmpPlayerManager->GetPlayerByID(p));
		if (cmpPlayer)
			color = cmpPlayer->GetColour();
		colors.push_back(color);
	}

	u8* p = bitmap;
	for (ssize_t j = 0; j < h; ++j)
	{
		for (ssize_t i = 0; i < w; ++i)
		{
			u8 val = territories.get(i, j) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK;

			CColor color(1, 0, 1, 1);
			// Force neutral territories to pure white, so that later we can omit them from the texture
			if (val == 0)
				color = CColor(1, 1, 1, 0);
			else if (val < colors.size())
				color = colors[val];

			*p++ = (int)(color.r*255.f);
			*p++ = (int)(color.g*255.f);
			*p++ = (int)(color.b*255.f);

			if ((i > 0 && (territories.get(i-1, j) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK) != val)
			 || (i < w-1 && (territories.get(i+1, j) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK) != val)
			 || (j > 0 && (territories.get(i, j-1) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK) != val)
			 || (j < h-1 && (territories.get(i, j+1) & ICmpTerritoryManager::TERRITORY_PLAYER_MASK) != val)
			)
			{
				*p++ = alphaMax;
			}
			else
			{
				*p++ = 0x00;
			}
		}
	}

	// Do a low-quality cheap blur effect

	for (ssize_t j = 0; j < h; ++j)
	{
		int a;

		a = 0;
		for (ssize_t i = 0; i < w; ++i)
		{
			a = std::max(a - alphaFalloff, (int)bitmap[(j*w+i)*4 + 3]);
			bitmap[(j*w+i)*4 + 3] = a;
		}

		a = 0;
		for (ssize_t i = w-1; i >= 0; --i)
		{
			a = std::max(a - alphaFalloff, (int)bitmap[(j*w+i)*4 + 3]);
			bitmap[(j*w+i)*4 + 3] = a;
		}
	}

	for (ssize_t i = 0; i < w; ++i)
	{
		int a;

		a = 0;
		for (ssize_t j = 0; j < w; ++j)
		{
			a = std::max(a - alphaFalloff, (int)bitmap[(j*w+i)*4 + 3]);
			bitmap[(j*w+i)*4 + 3] = a;
		}

		a = 0;
		for (ssize_t j = w-1; j >= 0; --j)
		{
			a = std::max(a - alphaFalloff, (int)bitmap[(j*w+i)*4 + 3]);
			bitmap[(j*w+i)*4 + 3] = a;
		}
	}

	// Add a gap between the boundaries, by deleting the max-alpha tiles
	for (ssize_t j = 0; j < h; ++j)
	{
		for (ssize_t i = 0; i < w; ++i)
		{
			if (bitmap[(j*w+i)*4 + 3] == alphaMax)
				bitmap[(j*w+i)*4 + 3] = 0;
		}
	}

	// Don't show neutral territory boundaries
	for (ssize_t j = 0; j < h; ++j)
	{
		for (ssize_t i = 0; i < w; ++i)
		{
			ssize_t idx = (j*w+i)*4;
			if (bitmap[idx] == 255 && bitmap[idx+1] == 255 && bitmap[idx+2] == 255)
				bitmap[idx+3] = 0;
		}
	}
}
