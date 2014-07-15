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

#include "LOSTexture.h"

#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "lib/bits.h"
#include "lib/config2.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/TimeManager.h"
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

// Alignment (in bytes) of the pixel data passed into glTexSubImage2D.
// This must be a multiple of GL_UNPACK_ALIGNMENT, which ought to be 1 (since
// that's what we set it to) but in some weird cases appears to have a different
// value. (See Trac #2594). Multiples of 4 are possibly good for performance anyway.
static const size_t g_SubTextureAlignment = 4;

CLOSTexture::CLOSTexture(CSimulation2& simulation) :
	m_Simulation(simulation), m_Dirty(true), m_Texture(0), m_smoothFbo(0), m_MapSize(0), m_TextureSize(0), whichTex(true)
{
	if (CRenderer::IsInitialised() && g_Renderer.m_Options.m_SmoothLOS)
	{
		m_smoothShader = g_Renderer.GetShaderManager().LoadEffect(str_los_interp);
		CShaderProgramPtr shader = m_smoothShader->GetShader();

		if (m_smoothShader && shader)
		{
			pglGenFramebuffersEXT(1, &m_smoothFbo);
		}
		else
		{
			LOGERROR(L"Failed to load SmoothLOS shader, disabling.");
			g_Renderer.m_Options.m_SmoothLOS = false;
		}
	}
}

CLOSTexture::~CLOSTexture()
{
	if (m_Texture)
		DeleteTexture();
}

void CLOSTexture::DeleteTexture()
{
	glDeleteTextures(1, &m_Texture);
	if (CRenderer::IsInitialised() && g_Renderer.m_Options.m_SmoothLOS)
	{
		glDeleteTextures(1, &m_TextureSmooth1);
		glDeleteTextures(1, &m_TextureSmooth2);
	}
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

GLuint CLOSTexture::GetTextureSmooth()
{
	if (CRenderer::IsInitialised() && !g_Renderer.m_Options.m_SmoothLOS)
		return GetTexture();
	else
		return whichTex ? m_TextureSmooth1 : m_TextureSmooth2;
}

void CLOSTexture::InterpolateLOS()
{
	if (CRenderer::IsInitialised() && !g_Renderer.m_Options.m_SmoothLOS)
		return;
	
	if (m_Dirty)
	{
		RecomputeTexture(0);
		m_Dirty = false;
	}
	
	GLint originalFBO;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &originalFBO);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_smoothFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
				   whichTex ? m_TextureSmooth2 : m_TextureSmooth1, 0);
	
	GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"LOS framebuffer object incomplete: 0x%04X", status);
	}

	m_smoothShader->BeginPass();
	CShaderProgramPtr shader = m_smoothShader->GetShader();
	
	glDisable(GL_BLEND);
	
	shader->Bind();
	
	shader->BindTexture(str_losTex1, m_Texture);
	shader->BindTexture(str_losTex2, whichTex ? m_TextureSmooth1 : m_TextureSmooth2);
	
	shader->Uniform(str_delta, (float)g_Renderer.GetTimeManager().GetFrameDelta() * 4.0f, 0.0f, 0.0f, 0.0f);
	
	const SViewPort oldVp = g_Renderer.GetViewport();
	const SViewPort vp = { 0, 0, m_TextureSize, m_TextureSize };
	g_Renderer.SetViewport(vp);
	
	float quadVerts[] = {
		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,

		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f
	};
	float quadTex[] = {
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,

		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f
	};
	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, quadTex);
	shader->VertexPointer(2, GL_FLOAT, 0, quadVerts);
	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	g_Renderer.SetViewport(oldVp);

	shader->Unbind();
	m_smoothShader->EndPass();
	
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 0, 0);
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, originalFBO);
	
	whichTex = !whichTex;
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

const CMatrix3D* CLOSTexture::GetMinimapTextureMatrix()
{
	ENSURE(!m_Dirty);
	return &m_MinimapTextureMatrix;
}

void CLOSTexture::ConstructTexture(int unit)
{
	CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return;

	m_MapSize = cmpTerrain->GetVerticesPerSide();

	m_TextureSize = (GLsizei)round_up_to_pow2(round_up((size_t)m_MapSize + g_BlurSize - 1, g_SubTextureAlignment));

	glGenTextures(1, &m_Texture);

	// Initialise texture with SoD colour, for the areas we don't
	// overwrite with glTexSubImage2D later
	u8* texData = new u8[m_TextureSize * m_TextureSize * 4];
	memset(texData, 0x00, m_TextureSize * m_TextureSize * 4);
	
	if (CRenderer::IsInitialised() && g_Renderer.m_Options.m_SmoothLOS)
	{
		glGenTextures(1, &m_TextureSmooth1);
		glGenTextures(1, &m_TextureSmooth2);
		
		g_Renderer.BindTexture(unit, m_TextureSmooth1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_TextureSize, m_TextureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		g_Renderer.BindTexture(unit, m_TextureSmooth2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_TextureSize, m_TextureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	
	g_Renderer.BindTexture(unit, m_Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_TextureSize, m_TextureSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, texData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	delete[] texData;	
	
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

	bool recreated = false;
	if (!m_Texture)
	{
		ConstructTexture(unit);
		recreated = true;
	}

	PROFILE("recompute LOS texture");

	std::vector<u8> losData;
	size_t pitch;
	losData.resize(GetBitmapSize(m_MapSize, m_MapSize, &pitch));

	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	if (!cmpRangeManager)
		return;

	ICmpRangeManager::CLosQuerier los(cmpRangeManager->GetLosQuerier(g_Game->GetPlayerID()));

	GenerateBitmap(los, &losData[0], m_MapSize, m_MapSize, pitch);

	if (CRenderer::IsInitialised() && g_Renderer.m_Options.m_SmoothLOS && recreated)
	{
		g_Renderer.BindTexture(unit, m_TextureSmooth1);		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pitch, m_MapSize, GL_ALPHA, GL_UNSIGNED_BYTE, &losData[0]);
		g_Renderer.BindTexture(unit, m_TextureSmooth2);		
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pitch, m_MapSize, GL_ALPHA, GL_UNSIGNED_BYTE, &losData[0]);
	}

	g_Renderer.BindTexture(unit, m_Texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pitch, m_MapSize, GL_ALPHA, GL_UNSIGNED_BYTE, &losData[0]);
}

size_t CLOSTexture::GetBitmapSize(size_t w, size_t h, size_t* pitch)
{
	*pitch = round_up(w + g_BlurSize - 1, g_SubTextureAlignment);
	return *pitch * (h + g_BlurSize - 1);
}

void CLOSTexture::GenerateBitmap(ICmpRangeManager::CLosQuerier los, u8* losData, size_t w, size_t h, size_t pitch)
{
	u8 *dataPtr = losData;

	// Initialise the top padding
	for (size_t j = 0; j < g_BlurSize/2; ++j)
		for (size_t i = 0; i < pitch; ++i)
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
		for (size_t i = 0; i < pitch - w - g_BlurSize/2; ++i)
			*dataPtr++ = 0;
	}

	// Initialise the bottom padding
	for (size_t j = 0; j < g_BlurSize/2; ++j)
		for (size_t i = 0; i < pitch; ++i)
			*dataPtr++ = 0;

	// Horizontal blur:

	for (size_t j = g_BlurSize/2; j < h + g_BlurSize/2; ++j)
	{
		for (size_t i = 0; i < w; ++i)
		{
			u8* d = &losData[i+j*pitch];
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
			u8* d = &losData[i+j*pitch];
			*d = (
				1*d[0*pitch] +
				6*d[1*pitch] +
				15*d[2*pitch] +
				20*d[3*pitch] +
				15*d[4*pitch] +
				6*d[5*pitch] +
				1*d[6*pitch]
			) / 64;
		}
	}
}
