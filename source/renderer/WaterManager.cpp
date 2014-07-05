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

/*
 * Water settings (speed, height) and texture management
 */

#include "precompiled.h"

#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"

#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "maths/MathUtil.h"
#include "maths/Vector2D.h"

#include "ps/Game.h"
#include "ps/World.h"

#include "renderer/WaterManager.h"
#include "renderer/Renderer.h"

#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/components/ICmpRangeManager.h"


///////////////////////////////////////////////////////////////////////////////////////////////
// WaterManager implementation


///////////////////////////////////////////////////////////////////
// Construction/Destruction
WaterManager::WaterManager()
{
	// water
	m_RenderWater = false; // disabled until textures are successfully loaded
	m_WaterHeight = 5.0f;

	m_WaterCurrentTex = 0;
	
	m_ReflectionTexture = 0;
	m_RefractionTexture = 0;
	m_ReflectionTextureSize = 0;
	m_RefractionTextureSize = 0;
	
	m_ReflectionFbo = 0;
	m_RefractionFbo = 0;
	
	m_WaterTexTimer = 0.0;

	m_WindAngle = 0.0f;
	m_Waviness = 8.0f;
	m_WaterColor = CColor(0.3f, 0.35f, 0.7f, 1.0f);
	m_WaterTint = CColor(0.28f, 0.3f, 0.59f, 1.0f);
	m_Murkiness = 0.45f;
	m_RepeatPeriod = 16.0f;

	m_DistanceHeightmap = NULL;
	m_BlurredNormalMap = NULL;
	m_WindStrength = NULL;
	
	m_WaterUgly = false;
	m_WaterFancyEffects = false;
	m_WaterRealDepth = false;
	m_WaterRefraction = false;
	m_WaterReflection = false;
	m_WaterShadows = false;
	m_WaterType = L"ocean";
	
	m_NeedsReloading = false;
	m_NeedInfoUpdate = true;
	
	m_depthTT = 0;
	m_FancyTexture = 0;
	m_ReflFboDepthTexture = 0;
	m_RefrFboDepthTexture = 0;

	m_MapSize = 0;
	
	m_updatei0 = 0;
	m_updatej0 = 0;
	m_updatei1 = 0;
	m_updatej1 = 0;
}

WaterManager::~WaterManager()
{
	// Cleanup if the caller messed up
	UnloadWaterTextures();

	SAFE_ARRAY_DELETE(m_DistanceHeightmap);
	SAFE_ARRAY_DELETE(m_BlurredNormalMap);
	SAFE_ARRAY_DELETE(m_WindStrength);
	
	glDeleteTextures(1, &m_depthTT);
	glDeleteTextures(1, &m_FancyTexture);
	glDeleteTextures(1, &m_ReflFboDepthTexture);
	glDeleteTextures(1, &m_RefrFboDepthTexture);
}


///////////////////////////////////////////////////////////////////
// Progressive load of water textures
int WaterManager::LoadWaterTextures()
{
	// TODO: this doesn't need to be progressive-loading any more
	// (since texture loading is async now)

	wchar_t pathname[PATH_MAX];
	
	// Load diffuse grayscale images (for non-fancy water)
	for (size_t i = 0; i < ARRAY_SIZE(m_WaterTexture); ++i)
	{
		swprintf_s(pathname, ARRAY_SIZE(pathname), L"art/textures/animated/water/default/diffuse%02d.dds", (int)i+1);
		CTextureProperties textureProps(pathname);
		textureProps.SetWrap(GL_REPEAT);

		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_WaterTexture[i] = texture;
	}

	// Load normalmaps (for fancy water)
	for (size_t i = 0; i < ARRAY_SIZE(m_NormalMap); ++i)
	{
		swprintf_s(pathname, ARRAY_SIZE(pathname), L"art/textures/animated/water/%ls/normal00%02d.png", m_WaterType.c_str(), (int)i+1);
		CTextureProperties textureProps(pathname);
		textureProps.SetWrap(GL_REPEAT);
		textureProps.SetMaxAnisotropy(4);
		
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_NormalMap[i] = texture;
	}
	
	m_ReflectionTextureSize = g_Renderer.GetHeight() * 0.66;	// Higher settings give a better result
	m_RefractionTextureSize = g_Renderer.GetHeight() * 0.33;	// Lower settings actually sorta look better since it blurs.

	// Create reflection texture
	glGenTextures(1, &m_ReflectionTexture);
	glBindTexture(GL_TEXTURE_2D, m_ReflectionTexture);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
		(GLsizei)m_ReflectionTextureSize, (GLsizei)m_ReflectionTextureSize,
		0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	
	// Create refraction texture
	glGenTextures(1, &m_RefractionTexture);
	glBindTexture(GL_TEXTURE_2D, m_RefractionTexture);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 
		(GLsizei)m_RefractionTextureSize, (GLsizei)m_RefractionTextureSize,
		0,  GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	// Create depth textures
	glGenTextures(1, &m_ReflFboDepthTexture);
	glBindTexture(GL_TEXTURE_2D, m_ReflFboDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (GLsizei)m_ReflectionTextureSize, (GLsizei)m_ReflectionTextureSize, 0,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
	
	glGenTextures(1, &m_RefrFboDepthTexture);
	glBindTexture(GL_TEXTURE_2D, m_RefrFboDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, (GLsizei)m_RefractionTextureSize, (GLsizei)m_RefractionTextureSize, 0,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
	
	// Create the water framebuffers
	
	GLint currentFbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &currentFbo);

	m_ReflectionFbo = 0;
	pglGenFramebuffersEXT(1, &m_ReflectionFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_ReflectionFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_ReflectionTexture, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_ReflFboDepthTexture, 0);

	ogl_WarnIfError();
	
	GLenum status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Reflection framebuffer object incomplete: 0x%04X", status);
		g_Renderer.m_Options.m_WaterReflection = false;
	}

	m_RefractionFbo = 0;
	pglGenFramebuffersEXT(1, &m_RefractionFbo);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_RefractionFbo);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_RefractionTexture, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_RefrFboDepthTexture, 0);

	ogl_WarnIfError();
	
	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Refraction framebuffer object incomplete: 0x%04X", status);
		g_Renderer.m_Options.m_WaterRefraction = false;
	}
	
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, currentFbo);

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
	glDeleteTextures(1, &m_ReflectionTexture);
	glDeleteTextures(1, &m_RefractionTexture);
	pglDeleteFramebuffersEXT(1, &m_RefractionFbo);
	pglDeleteFramebuffersEXT(1, &m_ReflectionFbo);
}

///////////////////////////////////////////////////////////////////
// Calculate our binary heightmap from the terrain heightmap.
void WaterManager::RecomputeDistanceHeightmap()
{
	if (m_DistanceHeightmap == NULL)
		m_DistanceHeightmap = new u8[m_MapSize*m_MapSize];
	
	// Custom copy the heightmap.
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	
	u16 waterLevel = m_WaterHeight/HEIGHT_SCALE;

	u16* heightmap = terrain->GetHeightMap();
	
	// We will "expand" the heightmap. That is we'll set each vertex on land as "3", and "bleed" that onto neighboring pixels.
	// So 3 is "on land", 2 is "close", 1 "somewhat close" and 0 is "water".
	// This gives a basic manhattan approximation of how close to the coast we are.
	// I have a heathen fondness for ternary operators so there are some below.
	u8 level = 0;
	for (size_t z = 0; z < m_MapSize; ++z)
	{
		level = 0;
		for (size_t x = 0; x < m_MapSize; ++x)
			m_DistanceHeightmap[z*m_MapSize + x] = heightmap[z*m_MapSize + x] >= waterLevel ? level = 3
												 : level > 0 ? --level : 0;
		level = 0;
		for (size_t x = m_MapSize-1; x != (size_t)-1; --x)
		{
			if (heightmap[z*m_MapSize + x] >= waterLevel)
				level = 3;	// no need to set m_distanceHeightmap, it's already been done by the other loop.
			else
			{
				level > 0 ? --level : 0;
				if (level > m_DistanceHeightmap[z*m_MapSize + x])
					m_DistanceHeightmap[z*m_MapSize + x] = level;
			}
		}
	}
	for (size_t x = 0; x < m_MapSize; ++x)
	{
		level = 0;
		for (size_t z = 0; z < m_MapSize; ++z)
		{
			if (heightmap[z*m_MapSize + x] >= waterLevel)
				level = 3;
			else
			{
				level > 0 ? --level : 0;
				if (level > m_DistanceHeightmap[z*m_MapSize + x])
					m_DistanceHeightmap[z*m_MapSize + x] = level;
			}
		}
		level = 0;
		for (size_t z = m_MapSize-1; z != (size_t)-1; --z)
		{
			if (heightmap[z*m_MapSize + x] >= waterLevel)
				level = 3;
			else
			{
				level > 0 ? --level : 0;
				if (level > m_DistanceHeightmap[z*m_MapSize + x])
					m_DistanceHeightmap[z*m_MapSize + x] = level;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////
// Calculate The blurred normal map to get an idea of where water ought to go.
void WaterManager::RecomputeBlurredNormalMap()
{
	// used to cache terrain normals since otherwise we'd recalculate them a lot (I'm blurring the "normal" map).
	// this might be updated to actually cache in the terrain manager but that's not for now.
	if (m_BlurredNormalMap == NULL)
		m_BlurredNormalMap = new CVector3D[m_MapSize*m_MapSize];

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	
	// It's really slow to calculate normals so cache them first.
	CVector3D* normals = new CVector3D[m_MapSize*m_MapSize];
	
	// Not the edges, we won't care about them.
	float ii = 8.0f, jj = 8.0f;
	for (size_t j = 2; j < m_MapSize-2; ++j, jj += 4.0f)
		for (size_t i = 2; i < m_MapSize-2; ++i, ii += 4.0f)
		{
			CVector3D norm;
			terrain->CalcNormal(i,j,norm);
			normals[j*m_MapSize + i] = norm;
		}
	
	// We could be way fancier (and faster) for our blur but we probably don't need the complexity.
	// Two pass filter, nothing complicated here.
	CVector3D blurValue;
	ii = 8.0f; jj = 8.0f;
	size_t idx = 2;
	for (size_t j = 2; j < m_MapSize-2; ++j, jj += 4.0f)
		for (size_t i = 2; i < m_MapSize-2; ++i, ii += 4.0f,++idx)
		{
			blurValue = normals[idx-2];
			blurValue += normals[idx-1];
			blurValue += normals[idx];
			blurValue += normals[idx+1];
			blurValue += normals[idx+2];
			m_BlurredNormalMap[idx] = blurValue * 0.2f;
		}
	// y direction, probably slower because of cache misses but I don't see an easy way around that.
	ii = 8.0f; jj = 8.0f;
	for (size_t i = 2; i < m_MapSize-2; ++i, ii += 4.0f)
	{
		for (size_t j = 2; j < m_MapSize-2; ++j, jj += 4.0f)
		{
			blurValue = normals[(j-2)*m_MapSize + i];
			blurValue += normals[(j-1)*m_MapSize + i];
			blurValue += normals[j*m_MapSize + i];
			blurValue += normals[(j+1)*m_MapSize + i];
			blurValue += normals[(j+2)*m_MapSize + i];
			m_BlurredNormalMap[j*m_MapSize + i] = blurValue * 0.2f;
		}
	}
	
	delete[] normals;
}

///////////////////////////////////////////////////////////////////
// Calculate the strength of the wind at a given point on the map.
// This is too slow and should support limited recomputation.
void WaterManager::RecomputeWindStrength()
{
	if (m_WindStrength == NULL)
		m_WindStrength = new float[m_MapSize*m_MapSize];
		
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	float waterLevel = m_WaterHeight;
	
	CVector2D windDir = CVector2D(cos(m_WindAngle),sin(m_WindAngle));
	CVector2D perp = CVector2D(-windDir.Y, windDir.X);

	// Our kernel will sample 5 points going towards the wind (generally).
	int kernel[5][2] = { {(int)windDir.X*2,(int)windDir.Y*2}, {(int)windDir.X*5,(int)windDir.Y*5}, {(int)windDir.X*9,(int)windDir.Y*9}, {(int)windDir.X*16,(int)windDir.Y*16}, {(int)windDir.X*25,(int)windDir.Y*25} };
	
	float* Temp = new float[m_MapSize*m_MapSize];
	std::fill(Temp, Temp + m_MapSize*m_MapSize, 1.0f);

	for (size_t j = 0; j < m_MapSize; ++j)
		for (size_t i = 0; i < m_MapSize; ++i)
		{
			float curHeight = terrain->GetVertexGroundLevel(i,j);
			if (curHeight >= waterLevel)
			{
				Temp[j*m_MapSize + i] = 0.3f;	// blurs too strong otherwise
				continue;
			}
			if (terrain->GetVertexGroundLevel(i + ceil(windDir.X),j + ceil(windDir.Y)) < waterLevel)
				continue;
			
			// Calculate how dampened our waves should be.
			float tendency = 0.0f;
			float oldHeight = std::max(waterLevel,terrain->GetVertexGroundLevel(i+kernel[4][0],j+kernel[4][1]));
			float currentHeight = std::max(waterLevel,terrain->GetVertexGroundLevel(i+kernel[3][0],j+kernel[3][1]));
			float avgheight = oldHeight + currentHeight;
			tendency = currentHeight - oldHeight;
			oldHeight = currentHeight;
			currentHeight = std::max(waterLevel,terrain->GetVertexGroundLevel(i+kernel[2][0],j+kernel[2][1]));
			avgheight += currentHeight;
			tendency += currentHeight - oldHeight;
			oldHeight = currentHeight;
			currentHeight = std::max(waterLevel,terrain->GetVertexGroundLevel(i+kernel[1][0],j+kernel[1][1]));
			avgheight += currentHeight;
			tendency += currentHeight - oldHeight;
			oldHeight = currentHeight;
			currentHeight = std::max(waterLevel,terrain->GetVertexGroundLevel(i+kernel[0][0],j+kernel[0][1]));
			avgheight += currentHeight;
			tendency += currentHeight - oldHeight;
			
			float baseLevel = std::max(0.0f,1.0f - (avgheight/5.0f-waterLevel)/20.0f);
			baseLevel *= baseLevel;
			tendency /= 15.0f;
			baseLevel -= tendency;	// if the terrain was sloping downwards, increase baselevel. Otherwise reduce.
			baseLevel = clamp(baseLevel,0.0f,1.0f);
			
			// Draw on map. This is pretty slow.
			float length = 35.0f * (1.0f-baseLevel/1.8f);
			for (float y = 0; y < length; y += 0.6f)
				{
					int xx = clamp(i - y * windDir.X,0.0f,(float)(m_MapSize-1));
					int yy = clamp(j - y * windDir.Y,0.0f,(float)(m_MapSize-1));
					Temp[yy*m_MapSize + xx] = Temp[yy*m_MapSize + xx] < (0.0f+baseLevel/1.5f) * (1.0f-y/length) + y/length * 1.0f ?
												Temp[yy*m_MapSize + xx] : (0.0f+baseLevel/1.5f) * (1.0f-y/length) + y/length * 1.0f;
				}
		}
	
	int blurKernel[4][2] = { {(int)ceil(windDir.X),(int)ceil(windDir.Y)}, {(int)windDir.X*3,(int)windDir.Y*3}, {(int)ceil(perp.X),(int)ceil(perp.Y)}, {(int)-ceil(perp.X),(int)-ceil(perp.Y)} };
	float blurValue;
	for (size_t j = 2; j < m_MapSize-2; ++j)
		for (size_t i = 2; i < m_MapSize-2; ++i)
		{
			blurValue = Temp[(j+blurKernel[0][1])*m_MapSize + i+blurKernel[0][0]];
			blurValue += Temp[(j+blurKernel[0][1])*m_MapSize + i+blurKernel[0][0]];
			blurValue += Temp[(j+blurKernel[0][1])*m_MapSize + i+blurKernel[0][0]];
			blurValue += Temp[(j+blurKernel[0][1])*m_MapSize + i+blurKernel[0][0]];
			m_WindStrength[j*m_MapSize + i] = blurValue * 0.25f;
		}
	delete[] Temp;
}

////////////////////////////////////////////////////////////////////////
// TODO: This will always recalculate for now
void WaterManager::SetMapSize(size_t size)
{
	// TODO: Im' blindly trusting the user here.
	m_MapSize = size;
	m_NeedInfoUpdate = true;
	m_updatei0 = 0;
	m_updatei1 = size;
	m_updatej0 = 0;
	m_updatej1 = size;
	
	SAFE_ARRAY_DELETE(m_DistanceHeightmap);
	SAFE_ARRAY_DELETE(m_BlurredNormalMap);
	SAFE_ARRAY_DELETE(m_WindStrength);
}

////////////////////////////////////////////////////////////////////////
// This will set the bools properly
void WaterManager::UpdateQuality()
{
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERUGLY) != m_WaterUgly) {
		m_WaterUgly = g_Renderer.GetOptionBool(CRenderer::OPT_WATERUGLY);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERFANCYEFFECTS) != m_WaterFancyEffects) {
		m_WaterFancyEffects = g_Renderer.GetOptionBool(CRenderer::OPT_WATERFANCYEFFECTS);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERREALDEPTH) != m_WaterRealDepth) {
		m_WaterRealDepth = g_Renderer.GetOptionBool(CRenderer::OPT_WATERREALDEPTH);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFRACTION) != m_WaterRefraction) {
		m_WaterRefraction = g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFRACTION);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFLECTION) != m_WaterReflection) {
		m_WaterReflection = g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFLECTION);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_SHADOWSONWATER) != m_WaterShadows) {
		m_WaterShadows = g_Renderer.GetOptionBool(CRenderer::OPT_SHADOWSONWATER);
		m_NeedsReloading = true;
	}
}

bool WaterManager::WillRenderFancyWater()
{
	if (!g_Renderer.GetCapabilities().m_FragmentShader)
		return false;
	if (!m_RenderWater || m_WaterUgly)
		return false;
	return true;
}
