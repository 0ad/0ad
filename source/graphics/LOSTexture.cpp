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

#include "LOSTexture.h"

#include "graphics/Terrain.h"
#include "lib/bits.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTerrain.h"

/*

The LOS bitmap is computed with one value per map vertex, based on
CCmpRangeManager's visibility information.

The bitmap is then blurred using an NxN filter (in particular a
7-tap Binomial filter as an efficient integral approximation of a Gaussian).
To implement the blur efficiently without using extra memory for a second copy
of the bitmap, we generate the bitmap with (N-1)/2 pixels of padding on each side,
then the blur shifts the image back into the corner.

The blurred bitmap is then uploaded into a GL texture for use by the renderer.

*/


// Blur with a NxN filter, where N = g_BlurSize must be an odd number.
static const size_t g_BlurSize = 7;

CLOSTexture::CLOSTexture(CSimulation2& simulation) :
	m_Simulation(simulation), m_Dirty(true), m_Texture(0), m_MapSize(0), m_TextureSize(0)
{
}

CLOSTexture::~CLOSTexture()
{
	if (m_Texture)
		DeleteTexture();
}

void CLOSTexture::DeleteTexture()
{
	glDeleteTextures(1, &m_Texture);
	m_Texture = 0;
}

void CLOSTexture::MakeDirty()
{
	m_Dirty = true;
}

void CLOSTexture::BindTexture(int unit)
{
	if (m_Dirty)
	{
		RecomputeTexture(unit);
		m_Dirty = false;
	}

	g_Renderer.BindTexture(unit, m_Texture);
}

GLuint CLOSTexture::GetTexture()
{
	if (m_Dirty)
	{
		RecomputeTexture(0);
		m_Dirty = false;
	}

	return m_Texture;
}

const CMatrix3D& CLOSTexture::GetTextureMatrix()
{
	ENSURE(!m_Dirty);
	return m_TextureMatrix;
}

const float* CLOSTexture::GetMinimapTextureMatrix()
{
	ENSURE(!m_Dirty);
	return &m_MinimapTextureMatrix._11;
}

void CLOSTexture::ConstructTexture(int unit)
{
	CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return;

	m_MapSize = cmpTerrain->GetVerticesPerSide();

	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize + g_BlurSize - 1);

	glGenTextures(1, &m_Texture);
	g_Renderer.BindTexture(unit, m_Texture);

	// Initialise texture with SoD colour, for the areas we don't
	// overwrite with glTexSubImage2D later
	u8* texData = new u8[m_TextureSize * m_TextureSize];
	memset(texData, 0x00, m_TextureSize * m_TextureSize);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, m_TextureSize, m_TextureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texData);
	delete[] texData;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	{
		// Texture matrix: We want to map
		//   world pos (0, y, 0)  (i.e. first vertex)
		//     onto texcoord (0.5/texsize, 0.5/texsize)  (i.e. middle of first texel);
		//   world pos ((mapsize-1)*cellsize, y, (mapsize-1)*cellsize)  (i.e. last vertex)
		//     onto texcoord ((mapsize-0.5) / texsize, (mapsize-0.5) / texsize)  (i.e. middle of last texel)

		float s = (m_MapSize-1) / (float)(m_TextureSize * (m_MapSize-1) * TERRAIN_TILE_SIZE);
		float t = 0.5f / m_TextureSize;
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

void CLOSTexture::RecomputeTexture(int unit)
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

	PROFILE("recompute LOS texture");

	std::vector<u8> losData;
	losData.resize(GetBitmapSize(m_MapSize, m_MapSize));

	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	if (!cmpRangeManager)
		return;

	ICmpRangeManager::CLosQuerier los (cmpRangeManager->GetLosQuerier(g_Game->GetPlayerID()));

	GenerateBitmap(los, &losData[0], m_MapSize, m_MapSize);

	g_Renderer.BindTexture(unit, m_Texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_MapSize + g_BlurSize - 1, m_MapSize + g_BlurSize - 1, GL_ALPHA, GL_UNSIGNED_BYTE, &losData[0]);
}

size_t CLOSTexture::GetBitmapSize(size_t w, size_t h)
{
	return (w + g_BlurSize - 1) * (h + g_BlurSize - 1);
}

void CLOSTexture::GenerateBitmap(ICmpRangeManager::CLosQuerier los, u8* losData, size_t w, size_t h)
{
	const size_t rowSize = w + g_BlurSize-1; // size of losData rows

	u8 *dataPtr = losData;

	// Initialise the top padding
	for (size_t j = 0; j < g_BlurSize/2; ++j)
		for (size_t i = 0; i < rowSize; ++i)
			*dataPtr++ = 0;

	for (size_t j = 0; j < h; ++j)
	{
		// Initialise the left padding
		for (size_t i = 0; i < g_BlurSize/2; ++i)
			*dataPtr++ = 0;

		// Fill in the visibility data
		for (size_t i = 0; i < w; ++i)
		{
			if (los.IsVisible_UncheckedRange(i, j))
				*dataPtr++ = 255;
			else if (los.IsExplored_UncheckedRange(i, j))
				*dataPtr++ = 127;
			else
				*dataPtr++ = 0;
		}

		// Initialise the right padding
		for (size_t i = 0; i < g_BlurSize/2; ++i)
			*dataPtr++ = 0;
	}

	// Initialise the bottom padding
	for (size_t j = 0; j < g_BlurSize/2; ++j)
		for (size_t i = 0; i < rowSize; ++i)
			*dataPtr++ = 0;

	// Horizontal blur:

	for (size_t j = g_BlurSize/2; j < h + g_BlurSize/2; ++j)
	{
		for (size_t i = 0; i < w; ++i)
		{
			u8* d = &losData[i+j*rowSize];
			*d = (
				1*d[0] +
				6*d[1] +
				15*d[2] +
				20*d[3] +
				15*d[4] +
				6*d[5] +
				1*d[6]
			) / 64;
		}
	}

	// Vertical blur:

	for (size_t j = 0; j < h; ++j)
	{
		for (size_t i = 0; i < w; ++i)
		{
			u8* d = &losData[i+j*rowSize];
			*d = (
				1*d[0*rowSize] +
				6*d[1*rowSize] +
				15*d[2*rowSize] +
				20*d[3*rowSize] +
				15*d[4*rowSize] +
				6*d[5*rowSize] +
				1*d[6*rowSize]
			) / 64;
		}
	}
}
