/* Copyright (C) 2009 Wildfire Games.
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

/*
 * Water settings (speed, height) and texture management
 */

#include "precompiled.h"

#include "graphics/TextureManager.h"

#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "maths/MathUtil.h"

#include "renderer/WaterManager.h"
#include "renderer/Renderer.h"


///////////////////////////////////////////////////////////////////////////////////////////////
// WaterManager implementation


///////////////////////////////////////////////////////////////////
// Construction/Destruction
WaterManager::WaterManager()
{
	// water
	m_RenderWater = false; // disabled until textures are successfully loaded
	m_WaterHeight = 5.0f;
	m_WaterColor = CColor(0.3f, 0.35f, 0.7f, 1.0f);
	m_WaterFullDepth = 4.0f;
	m_WaterMaxAlpha = 0.85f;
	m_WaterAlphaOffset = -0.05f;
	m_SWaterTrans = 0;
	m_TWaterTrans = 0;
	m_SWaterSpeed = 0.0015f;
	m_TWaterSpeed = 0.0015f;
	m_SWaterScrollCounter = 0;
	m_TWaterScrollCounter = 0;
	m_WaterCurrentTex = 0;
	m_ReflectionTexture = 0;
	m_RefractionTexture = 0;
	m_ReflectionTextureSize = 0;
	m_RefractionTextureSize = 0;
	m_WaterTexTimer = 0.0;
	m_Shininess = 150.0f;
	m_SpecularStrength = 0.4f;
	m_Waviness = 8.0f;
	m_ReflectionTint = CColor(0.28f, 0.3f, 0.59f, 1.0f);
	m_ReflectionTintStrength = 0.0f;
	m_WaterTint = CColor(0.28f, 0.3f, 0.59f, 1.0f);
	m_Murkiness = 0.45f;
	m_RepeatPeriod = 16.0f;
}

WaterManager::~WaterManager()
{
	// Cleanup if the caller messed up
	UnloadWaterTextures();
}


///////////////////////////////////////////////////////////////////
// Progressive load of water textures
int WaterManager::LoadWaterTextures()
{
	// TODO: this doesn't need to be progressive-loading any more
	// (since texture loading is async now)

	// TODO: add a member variable and setter for this. (can't make this
	// a parameter because this function is called via delay-load code)
	static const wchar_t* const water_type = L"default";

	wchar_t pathname[PATH_MAX];

	// Load diffuse grayscale images (for non-fancy water)
	for (size_t i = 0; i < ARRAY_SIZE(m_WaterTexture); ++i)
	{
		swprintf_s(pathname, ARRAY_SIZE(pathname), L"art/textures/animated/water/%ls/diffuse%02d.dds", water_type, (int)i+1);
		CTextureProperties textureProps(pathname);
		textureProps.SetWrap(GL_REPEAT);

		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_WaterTexture[i] = texture;
	}

	// Load normalmaps (for fancy water)
	for (size_t i = 0; i < ARRAY_SIZE(m_NormalMap); ++i)
	{
		swprintf_s(pathname, ARRAY_SIZE(pathname), L"art/textures/animated/water/%ls/normal%02d.dds", water_type, (int)i+1);
		CTextureProperties textureProps(pathname);
		textureProps.SetWrap(GL_REPEAT);

		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_NormalMap[i] = texture;
	}

	// Set the size to the largest power of 2 that is <= to the window height, so
	// the reflection/refraction images will fit within the window
	// (alternative: use FBO's, which can have arbitrary size - but do we need
	// the reflection/refraction textures to be that large?)
	int size = (int)round_up_to_pow2((unsigned)g_Renderer.GetHeight());
	if(size > g_Renderer.GetHeight()) size /= 2;
	m_ReflectionTextureSize = size;
	m_RefractionTextureSize = size;

	// Create reflection texture
	glGenTextures(1, &m_ReflectionTexture);
	glBindTexture(GL_TEXTURE_2D, m_ReflectionTexture);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB,
		(GLsizei)m_ReflectionTextureSize, (GLsizei)m_ReflectionTextureSize,
		0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Create refraction texture
	glGenTextures(1, &m_RefractionTexture);
	glBindTexture(GL_TEXTURE_2D, m_RefractionTexture);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 
		(GLsizei)m_RefractionTextureSize, (GLsizei)m_RefractionTextureSize,
		0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Enable rendering, now that we've succeeded this far
	m_RenderWater = true;

	return 0;
}


///////////////////////////////////////////////////////////////////
// Unload water textures
void WaterManager::UnloadWaterTextures()
{
	for(size_t i = 0; i < ARRAY_SIZE(m_WaterTexture); i++)
	{
		m_WaterTexture[i].reset();
	}


	for(size_t i = 0; i < ARRAY_SIZE(m_NormalMap); i++)
	{
		m_NormalMap[i].reset();
	}
}


bool WaterManager::WillRenderFancyWater()
{
	return (g_Renderer.GetCapabilities().m_FragmentShader && g_Renderer.GetOptionBool(CRenderer::OPT_FANCYWATER));
}
