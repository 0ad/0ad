/* Copyright (C) 2011 Wildfire Games.
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
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpRangeManager.h"

/*

The LOS bitmap is computed with one value per map vertex, based on
CCmpRangeManager's visibility information.

The bitmap is then blurred using an NxN box filter (i.e. convolution
with (1 1 1; 1 1 1; 1 1 1) etc). To implement the blur efficiently without
using extra memory for a second copy of the bitmap, we generate the bitmap
with (N-1)/2 pixels of padding on each side, then the blur shifts the
image back into the corner.

The blurred bitmap is then uploaded into a GL texture for use by the renderer.

*/


// Blur with a NxN box filter, where N = g_BlurSize must be an odd number.
static const size_t g_BlurSize = 5;

CLOSTexture::CLOSTexture() :
	m_Dirty(true), m_Texture(0), m_MapSize(0), m_TextureSize(0)
{
}

CLOSTexture::~CLOSTexture()
{
	if (m_Texture)
	{
		glDeleteTextures(1, &m_Texture);
		m_Texture = 0;
	}
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

const float* CLOSTexture::GetTextureMatrix()
{
	debug_assert(!m_Dirty);
	return &m_TextureMatrix._11;
}

const float* CLOSTexture::GetMinimapTextureMatrix()
{
	debug_assert(!m_Dirty);
	return &m_MinimapTextureMatrix._11;
}

void CLOSTexture::ConstructTexture(int unit)
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	m_MapSize = terrain->GetVerticesPerSide();
	m_TextureSize = (GLsizei)round_up_to_pow2((size_t)m_MapSize + g_BlurSize - 1);

	glGenTextures(1, &m_Texture);
	g_Renderer.BindTexture(unit, m_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA8, m_TextureSize, m_TextureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
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

		float s = (m_MapSize-1) / (float)(m_TextureSize * (m_MapSize-1) * CELL_SIZE);
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
	if (!m_Texture)
		ConstructTexture(unit);

	PROFILE("recompute LOS texture");

	std::vector<u8> losData;
	losData.resize(GetBitmapSize(m_MapSize, m_MapSize));

	CmpPtr<ICmpRangeManager> cmpRangeManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpRangeManager.null())
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
			if (los.IsVisible(i, j))
				*dataPtr++ = 255;
			else if (los.IsExplored(i, j))
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
		u32 accum = 0; // sum of values in the sliding Nx1 window

		// Initialise the filter window for the first output pixel
		// (Start i at the first of the non-padding pixels, since the
		// padding was set to 0)
		for (size_t i = g_BlurSize/2; i < g_BlurSize-1; ++i)
			accum += losData[i+j*rowSize];

		// For each pixel, update the sliding window and store the average
		// of the window's sum
		for (size_t i = 0; i < w; ++i)
		{
			accum += losData[i+g_BlurSize-1+j*rowSize];

			u8 old = losData[i+j*rowSize];

			losData[i+j*rowSize] = accum / g_BlurSize;

			accum -= old;
		}
	}

	// Vertical blur:

	for (size_t i = 0; i < w; ++i)
	{
		u32 accum = 0;

		for (size_t j = g_BlurSize/2; j < g_BlurSize-1; ++j)
			accum += losData[i+j*rowSize];

		for (size_t j = 0; j < h; ++j)
		{
			accum += losData[i+(j+g_BlurSize-1)*rowSize];

			u8 old = losData[i+j*rowSize];

			losData[i+j*rowSize] = accum / g_BlurSize;

			accum -= old;
		}
	}
}
