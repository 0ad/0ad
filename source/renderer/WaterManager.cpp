/* Copyright (C) 2014 Wildfire Games.
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
#include "graphics/ShaderManager.h"
#include "graphics/ShaderProgram.h"

#include "lib/bits.h"
#include "lib/timer.h"
#include "lib/tex/tex.h"
#include "lib/res/graphics/ogl_tex.h"

#include "maths/MathUtil.h"
#include "maths/Vector2D.h"

#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/World.h"

#include "renderer/WaterManager.h"
#include "renderer/Renderer.h"

#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/components/ICmpRangeManager.h"


///////////////////////////////////////////////////////////////////////////////////////////////
// WaterManager implementation

struct CoastalPoint
{
	CoastalPoint(int idx, CVector2D pos) : index(idx), position(pos) {};
	int index;
	CVector2D position;
};

struct SWavesVertex {
	// vertex position
	CVector3D m_BasePosition;
	CVector3D m_ApexPosition;
	CVector3D m_SplashPosition;
	CVector3D m_RetreatPosition;

	CVector2D m_PerpVect;
	u8 m_UV[3];
};
cassert(sizeof(SWavesVertex) == 60);

struct WaveObject
{
	CVertexBuffer::VBChunk* m_VBvertices;
	CBoundingBoxAligned m_AABB;
	size_t m_Width;
	float m_TimeDiff;
};

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
	m_FancyEffectsFBO = 0;
	
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
	m_FancyTextureNormal = 0;
	m_FancyTextureOther = 0;
	m_FancyTextureDepth = 0;
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

	// TODO: when c++11 is around, use lambdas or something because short Korea is best Korea.
	for (size_t i = 0; i < m_ShoreWaves.size(); ++i)
		delete m_ShoreWaves[i];

	SAFE_ARRAY_DELETE(m_DistanceHeightmap);
	SAFE_ARRAY_DELETE(m_BlurredNormalMap);
	SAFE_ARRAY_DELETE(m_WindStrength);
	
	glDeleteTextures(1, &m_depthTT);
	glDeleteTextures(1, &m_FancyTextureNormal);
	glDeleteTextures(1, &m_FancyTextureOther);
	glDeleteTextures(1, &m_FancyTextureDepth);
	glDeleteTextures(1, &m_ReflFboDepthTexture);
	glDeleteTextures(1, &m_RefrFboDepthTexture);
	
	pglDeleteFramebuffersEXT(1, &m_FancyEffectsFBO);
	pglDeleteFramebuffersEXT(1, &m_RefractionFbo);
	pglDeleteFramebuffersEXT(1, &m_ReflectionFbo);
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
	
	// Load CoastalWaves
	{
		CTextureProperties textureProps(L"art/textures/terrain/types/water/coastalWave.png");
		textureProps.SetWrap(GL_REPEAT);
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_WaveTex = texture;
	}
	
	// Load Foam
	{
		CTextureProperties textureProps(L"art/textures/terrain/types/water/foam.png");
		textureProps.SetWrap(GL_REPEAT);
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_FoamTex = texture;
	}
	
	m_ReflectionTextureSize = g_Renderer.GetHeight() * 0.66;	// Higher settings give a better result
	m_RefractionTextureSize = g_Renderer.GetHeight() * 0.33;	// Lower settings actually sorta look better since it blurs.
	
	if (round_down_to_pow2(m_ReflectionTextureSize)/m_ReflectionTextureSize < 0.65)
		m_ReflectionTextureSize = round_up_to_pow2(m_ReflectionTextureSize);
	else
		m_ReflectionTextureSize = round_down_to_pow2(m_ReflectionTextureSize);
	
	if (round_down_to_pow2(m_RefractionTextureSize)/m_RefractionTextureSize < 0.7)
		m_RefractionTextureSize = round_up_to_pow2(m_RefractionTextureSize);
	else
		m_RefractionTextureSize = round_down_to_pow2(m_RefractionTextureSize);
	
	// Create reflection texture
	glGenTextures(1, &m_ReflectionTexture);
	glBindTexture(GL_TEXTURE_2D, m_ReflectionTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)m_ReflectionTextureSize, (GLsizei)m_ReflectionTextureSize, 0,  GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
	// Create refraction texture
	glGenTextures(1, &m_RefractionTexture);
	glBindTexture(GL_TEXTURE_2D, m_RefractionTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, (GLsizei)m_RefractionTextureSize, (GLsizei)m_RefractionTextureSize, 0,  GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Create depth textures
	glGenTextures(1, &m_ReflFboDepthTexture);
	glBindTexture(GL_TEXTURE_2D, m_ReflFboDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, (GLsizei)m_ReflectionTextureSize, (GLsizei)m_ReflectionTextureSize, 0,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
	
	glGenTextures(1, &m_RefrFboDepthTexture);
	glBindTexture(GL_TEXTURE_2D, m_RefrFboDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, (GLsizei)m_RefractionTextureSize, (GLsizei)m_RefractionTextureSize, 0,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);

	// Create the Fancy Effects texture
	glGenTextures(1, &m_FancyTextureNormal);
	glBindTexture(GL_TEXTURE_2D, m_FancyTextureNormal);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)g_Renderer.GetWidth(), (GLsizei)g_Renderer.GetHeight(), 0,  GL_RGBA, GL_UNSIGNED_SHORT, NULL);

	glGenTextures(1, &m_FancyTextureOther);
	glBindTexture(GL_TEXTURE_2D, m_FancyTextureOther);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)g_Renderer.GetWidth(), (GLsizei)g_Renderer.GetHeight(), 0,  GL_RGBA, GL_UNSIGNED_SHORT, NULL);

	glGenTextures(1, &m_FancyTextureDepth);
	glBindTexture(GL_TEXTURE_2D, m_FancyTextureDepth);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, (GLsizei)g_Renderer.GetWidth(), (GLsizei)g_Renderer.GetHeight(), 0,  GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
	
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
	
	pglGenFramebuffersEXT(1, &m_FancyEffectsFBO);
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_FancyEffectsFBO);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_FancyTextureNormal, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, m_FancyTextureOther, 0);
	pglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_FancyTextureDepth, 0);
	
	ogl_WarnIfError();
	
	status = pglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		LOGWARNING(L"Fancy Effects framebuffer object incomplete: 0x%04X", status);
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
	size_t SideSize = m_MapSize*2;
	if (m_DistanceHeightmap == NULL)
		m_DistanceHeightmap = new float[SideSize*SideSize];
		
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	
	// Create a manhattan-distance heightmap.
	// This is currently upsampled by a factor of 2 to get more precision
	// This could be refined to only be done near the coast itself, but it's probably not necessary.
	
	float level = SideSize;
	for (size_t z = 0; z < SideSize; ++z)
	{
		level = SideSize;
		for (size_t x = 0; x < SideSize; ++x)
			m_DistanceHeightmap[z*SideSize + x] = terrain->GetExactGroundLevel(x*2, z*2) >= m_WaterHeight ? level = 0.f : ++level;
		level = SideSize;
		for (size_t x = SideSize-1; x != (size_t)-1; --x)
		{
			if (terrain->GetExactGroundLevel(x*2, z*2) >= m_WaterHeight)
				level = 0.f;
			else
			{
				++level;
				if (level < m_DistanceHeightmap[z*SideSize + x])
					m_DistanceHeightmap[z*SideSize + x] = level;
			}
		}
	}
	for (size_t x = 0; x < SideSize; ++x)
	{
		level = SideSize;
		for (size_t z = 0; z < SideSize; ++z)
		{
			if (terrain->GetExactGroundLevel(x*2, z*2) >= m_WaterHeight)
				level = 0.f;
			else if (level > m_DistanceHeightmap[z*SideSize + x])
				level = m_DistanceHeightmap[z*SideSize + x];
			else
			{
				++level;
				if (level < m_DistanceHeightmap[z*SideSize + x])
					m_DistanceHeightmap[z*SideSize + x] = level;
			}
		}
		level = SideSize;
		for (size_t z = SideSize-1; z != (size_t)-1; --z)
		{
			if (terrain->GetExactGroundLevel(x*2, z*2) >= m_WaterHeight)
				level = 0.f;
			else if (level > m_DistanceHeightmap[z*SideSize + x])
				level = m_DistanceHeightmap[z*SideSize + x];
			else
			{
				++level;
				if (level < m_DistanceHeightmap[z*SideSize + x])
					m_DistanceHeightmap[z*SideSize + x] = level;
			}
		}
	}
}

// This requires m_DistanceHeightmap to be defined properly.
void WaterManager::CreateWaveMeshes()
{
	size_t SideSize = m_MapSize*2;
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	// TODO: when c++11 is around, use lambdas or something because short Korea is best Korea.
	for (size_t i = 0; i < m_ShoreWaves.size(); ++i)
		delete m_ShoreWaves[i];
	m_ShoreWaves.clear();

	if (m_Waviness < 5.0f && m_WaterType != L"ocean")
		return;
		
	// First step: get the points near the coast.
	std::set<int> CoastalPointsSet;
	for (size_t z = 1; z < SideSize-1; ++z)
		for (size_t x = 1; x < SideSize-1; ++x)
			if (abs(m_DistanceHeightmap[z*SideSize + x]-1.0f) < 0.2f)
				CoastalPointsSet.insert(z*SideSize + x);
	
	// Second step: create chains out of those coastal points.
	static const int around[8][2] = { { -1,-1 }, { -1,0 }, { -1,1 }, { 0,1 }, { 1,1 }, { 1,0 }, { 1,-1 }, { 0,-1 } };

	while (!CoastalPointsSet.empty())
	{
		int index = *(CoastalPointsSet.begin());
		int x = index % SideSize;
		int y = (index - x ) / SideSize;
		
		std::deque<CoastalPoint> Chain;
		
		Chain.push_front(CoastalPoint(index,CVector2D(x*2,y*2)));
		
		// Erase us.
		CoastalPointsSet.erase(CoastalPointsSet.begin());
		
		// We're our starter points. At most we can have 2 points close to us.
		// We'll pick the first one and look for its neighbors (he can only have one new)
		// Up until we either reach the end of the chain, or ourselves.
		// Then go down the other direction if there is any.
		int neighbours[2] = { -1, -1 };
		int nbNeighb = 0;
		for (int i = 0; i < 8; ++i)
		{
			if (CoastalPointsSet.count(x + around[i][0] + (y + around[i][1])*SideSize))
			{
				if (nbNeighb < 2)
					neighbours[nbNeighb] = x + around[i][0] + (y + around[i][1])*SideSize;
				++nbNeighb;
			}
		}
		if (nbNeighb > 2)
			continue;

		for (int i = 0; i < 2; ++i)
		{
			if (neighbours[i] == -1)
				continue;
			// Move to our neighboring point
			int xx = neighbours[i] % SideSize;
			int yy = (neighbours[i] - xx ) / SideSize;
			int indexx = xx + yy*SideSize;
			int endedChain = false;
			
			if (i == 0)
				Chain.push_back(CoastalPoint(indexx,CVector2D(xx*2,yy*2)));
			else
				Chain.push_front(CoastalPoint(indexx,CVector2D(xx*2,yy*2)));

			// If there's a loop we'll be the "other" neighboring point already so check for that.
			// We'll readd at the end/front the other one to have full squares.
			if (CoastalPointsSet.count(indexx) == 0)
				break;
						
			CoastalPointsSet.erase(indexx);

			// Start checking from there.
			while(!endedChain)
			{
				bool found = false;
				nbNeighb = 0;
				for (int p = 0; p < 8; ++p)
				{
					if (CoastalPointsSet.count(xx+around[p][0] + (yy + around[p][1])*SideSize))
					{
						if (nbNeighb >= 2)
						{
							CoastalPointsSet.erase(xx + yy*SideSize);
							continue;
						}
						++nbNeighb;
						// We've found a new point around us.
						// Move there
						xx = xx + around[p][0];
						yy = yy + around[p][1];
						indexx = xx + yy*SideSize;
						if (i == 0)
							Chain.push_back(CoastalPoint(indexx,CVector2D(xx*2,yy*2)));
						else
							Chain.push_front(CoastalPoint(indexx,CVector2D(xx*2,yy*2)));
						CoastalPointsSet.erase(xx + yy*SideSize);
						found = true;
						break;
					}
				}
				if (!found)
					endedChain = true;
			}
		}
		if (Chain.size() > 10)
			CoastalPointsChains.push_back(Chain);
	}
		
	// (optional) third step: Smooth chains out.
	// This is also really dumb.
	for (size_t i = 0; i < CoastalPointsChains.size(); ++i)
	{
		// Bump 1 for smoother.
		for (int p = 0; p < 3; ++p)
		{
			for (size_t j = 1; j < CoastalPointsChains[i].size()-1; ++j)
			{
				CVector2D realPos = CoastalPointsChains[i][j-1].position + CoastalPointsChains[i][j+1].position;
				
				CoastalPointsChains[i][j].position = (CoastalPointsChains[i][j].position + realPos/2.0f)/2.0f;
			}
		}
	}
	
	// Fourth step: create waves themselves, using those chains. We basically create subchains.
	size_t waveSizes = 14;	// maximal size in width.
	
	// Construct indices buffer (we can afford one for all of them)
	std::vector<GLushort> water_indices;
	for (size_t a = 0; a < waveSizes-1;++a)
	{
		for (size_t rect = 0; rect < 7; ++rect)
		{
			water_indices.push_back(a*9 + rect);
			water_indices.push_back(a*9 + 9 + rect);
			water_indices.push_back(a*9 + 1 + rect);
			water_indices.push_back(a*9 + 9 + rect);
			water_indices.push_back(a*9 + 10 + rect);
			water_indices.push_back(a*9 + 1 + rect);
		}
	}
	// Generic indexes, max-length
	m_ShoreWaves_VBIndices = g_VBMan.Allocate(sizeof(GLushort), water_indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
	m_ShoreWaves_VBIndices->m_Owner->UpdateChunkVertices(m_ShoreWaves_VBIndices, &water_indices[0]);
	
	float diff = (rand() % 50) / 5.0f;
	
	for (size_t i = 0; i < CoastalPointsChains.size(); ++i)
	{
		for (size_t j = 0; j < CoastalPointsChains[i].size()-waveSizes; ++j)
		{
			if (CoastalPointsChains[i].size()- 1 - j < waveSizes)
				break;
			
			size_t width = waveSizes;
			
			// First pass to get some parameters out.
			float outmost = 0.0f;	// how far to move on the shore.
			float avgDepth = 0.0f;
			int sign = 1;
			CVector2D firstPerp(0,0), perp(0,0), lastPerp(0,0);
			for (size_t a = 0; a < waveSizes;++a)
			{
				lastPerp = perp;
				perp = CVector2D(0,0);
				int nb = 0;
				CVector2D pos = CoastalPointsChains[i][j+a].position;
				CVector2D posPlus;
				CVector2D posMinus;
				if (a > 0)
				{
					++nb;
					posMinus = CoastalPointsChains[i][j+a-1].position;
					perp += pos-posMinus;
				}
				if (a < waveSizes-1)
				{
					++nb;
					posPlus = CoastalPointsChains[i][j+a+1].position;
					perp += posPlus-pos;
				}
				perp /= nb;
				perp = CVector2D(-perp.Y,perp.X).Normalized();
				
				if (a == 0)
					firstPerp = perp;
				
				if ( a > 1 && perp.Dot(lastPerp) < 0.90f && perp.Dot(firstPerp) < 0.70f)
				{
					width = a+1;
					break;
				}
				
				if (m_BlurredNormalMap[ (int)(pos.X/4) + (int)(pos.Y/4)*m_MapSize].Y < 0.9)
				{
					width = a-1;
					break;
				}
				
				if (terrain->GetExactGroundLevel(pos.X+perp.X*1.5f, pos.Y+perp.Y*1.5f) > m_WaterHeight)
					sign = -1;
				
				avgDepth += terrain->GetExactGroundLevel(pos.X+sign*perp.X*20.0f, pos.Y+sign*perp.Y*20.0f) - m_WaterHeight;
				
				float localOutmost = -2.0f;
				while (localOutmost < 0.0f)
				{
					float depth = terrain->GetExactGroundLevel(pos.X+sign*perp.X*localOutmost, pos.Y+sign*perp.Y*localOutmost) - m_WaterHeight;
					if (depth < 0.0f || depth > 0.6f)
						localOutmost += 0.2f;
					else
						break;
				}

				outmost += localOutmost;
			}
			if (width < 5)
			{
				j += 6;
				continue;
			}

			outmost /= width;
			
			if (outmost > -0.5f)
			{
				j += 3;
				continue;
			}
			outmost = -0.5f + outmost * m_Waviness/10.0f;

			avgDepth /= width;

			if (avgDepth > -1.3f)
			{
				j += 3;
				continue;
			}
			// we passed the checks, we can create a wave of size "width".
			
			WaveObject* shoreWave = new WaveObject;
			std::vector<SWavesVertex> vertices;

			shoreWave->m_Width = width;
			shoreWave->m_TimeDiff = diff;
			diff += (rand() % 100) / 25.0f + 4.0f;

			for (size_t a = 0; a < width;++a)
			{
				CVector2D perp = CVector2D(0,0);
				int nb = 0;
				CVector2D pos = CoastalPointsChains[i][j+a].position;
				CVector2D posPlus;
				CVector2D posMinus;
				if (a > 0)
				{
					++nb;
					posMinus = CoastalPointsChains[i][j+a-1].position;
					perp += pos-posMinus;
				}
				if (a < waveSizes-1)
				{
					++nb;
					posPlus = CoastalPointsChains[i][j+a+1].position;
					perp += posPlus-pos;
				}
				perp /= nb;
				perp = CVector2D(-perp.Y,perp.X).Normalized();
				
				SWavesVertex point[9];
				
				float baseHeight = 0.04f;
				
				float halfWidth = (width-1.0f)/2.0f;
				float sideNess = sqrtf(clamp( (halfWidth - fabsf(a-halfWidth))/3.0f, 0.0f,1.0f));
				
				point[0].m_UV[0] = a; point[0].m_UV[1] = 8;
				point[1].m_UV[0] = a; point[1].m_UV[1] = 7;
				point[2].m_UV[0] = a; point[2].m_UV[1] = 6;
				point[3].m_UV[0] = a; point[3].m_UV[1] = 5;
				point[4].m_UV[0] = a; point[4].m_UV[1] = 4;
				point[5].m_UV[0] = a; point[5].m_UV[1] = 3;
				point[6].m_UV[0] = a; point[6].m_UV[1] = 2;
				point[7].m_UV[0] = a; point[7].m_UV[1] = 1;
				point[8].m_UV[0] = a; point[8].m_UV[1] = 0;
				
				point[0].m_PerpVect = perp;
				point[1].m_PerpVect = perp;
				point[2].m_PerpVect = perp;
				point[3].m_PerpVect = perp;
				point[4].m_PerpVect = perp;
				point[5].m_PerpVect = perp;
				point[6].m_PerpVect = perp;
				point[7].m_PerpVect = perp;
				point[8].m_PerpVect = perp;
				
				static const float perpT1[9] = { 6.0f, 6.05f, 6.1f, 6.2f, 6.3f, 6.4f, 6.5f, 6.6f, 9.7f };
				static const float perpT2[9] = { 2.0f, 2.1f,  2.2f, 2.3f, 2.4f, 3.0f, 3.3f, 3.6f, 9.5f };
				static const float perpT3[9] = { 1.1f, 0.7f, -0.2f, 0.0f, 0.6f, 1.3f, 2.2f, 3.6f, 9.0f };
				static const float perpT4[9] = { 2.0f, 2.1f,  1.2f, 1.5f, 1.7f, 1.9f, 2.7f, 3.8f, 9.0f };
				
				static const float heightT1[9] = { 0.0f, 0.2f, 0.5f, 0.8f, 0.9f, 0.85f, 0.6f, 0.2f, 0.0 };
				static const float heightT2[9] = { -0.8f, -0.4f, 0.0f, 0.1f, 0.1f, 0.03f, 0.0f, 0.0f, 0.0 };
				static const float heightT3[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0 };

				for (size_t t = 0; t < 9; ++t)
				{
					float terrHeight = 0.05f + terrain->GetExactGroundLevel(pos.X+sign*perp.X*(perpT1[t]+outmost),
																			pos.Y+sign*perp.Y*(perpT1[t]+outmost));
					point[t].m_BasePosition = CVector3D(pos.X+sign*perp.X*(perpT1[t]+outmost), baseHeight + heightT1[t]*sideNess + std::max(m_WaterHeight,terrHeight),
														pos.Y+sign*perp.Y*(perpT1[t]+outmost));
				}
				for (size_t t = 0; t < 9; ++t)
				{
					float terrHeight = 0.05f + terrain->GetExactGroundLevel(pos.X+sign*perp.X*(perpT2[t]+outmost),
																			pos.Y+sign*perp.Y*(perpT2[t]+outmost));
					point[t].m_ApexPosition = CVector3D(pos.X+sign*perp.X*(perpT2[t]+outmost), baseHeight + heightT1[t]*sideNess + std::max(m_WaterHeight,terrHeight),
														pos.Y+sign*perp.Y*(perpT2[t]+outmost));
				}
				for (size_t t = 0; t < 9; ++t)
				{
					float terrHeight = 0.05f + terrain->GetExactGroundLevel(pos.X+sign*perp.X*(perpT3[t]+outmost*sideNess),
																			pos.Y+sign*perp.Y*(perpT3[t]+outmost*sideNess));
					point[t].m_SplashPosition = CVector3D(pos.X+sign*perp.X*(perpT3[t]+outmost*sideNess), baseHeight + heightT2[t]*sideNess + std::max(m_WaterHeight,terrHeight), pos.Y+sign*perp.Y*(perpT3[t]+outmost*sideNess));
				}
				for (size_t t = 0; t < 9; ++t)
				{
					float terrHeight = 0.05f + terrain->GetExactGroundLevel(pos.X+sign*perp.X*(perpT4[t]+outmost),
																			pos.Y+sign*perp.Y*(perpT4[t]+outmost));
					point[t].m_RetreatPosition = CVector3D(pos.X+sign*perp.X*(perpT4[t]+outmost), baseHeight + heightT3[t]*sideNess + std::max(m_WaterHeight,terrHeight),
														   pos.Y+sign*perp.Y*(perpT4[t]+outmost));
				}
				
				vertices.push_back(point[8]);
				vertices.push_back(point[7]);
				vertices.push_back(point[6]);
				vertices.push_back(point[5]);
				vertices.push_back(point[4]);
				vertices.push_back(point[3]);
				vertices.push_back(point[2]);
				vertices.push_back(point[1]);
				vertices.push_back(point[0]);
				
				shoreWave->m_AABB += point[8].m_SplashPosition;
				shoreWave->m_AABB += point[8].m_BasePosition;
				shoreWave->m_AABB += point[0].m_SplashPosition;
				shoreWave->m_AABB += point[0].m_BasePosition;
				shoreWave->m_AABB += point[4].m_ApexPosition;
			}

			if (sign == 1)
			{
				// Let's do some fancy reversing.
				std::vector<SWavesVertex> reversed;
				for (int a = width-1; a >= 0; --a)
				{
					for (size_t t = 0; t < 9; ++t)
						reversed.push_back(vertices[a*9+t]);
				}
				vertices = reversed;
			}
			j += width/2-1;
			
			shoreWave->m_VBvertices = g_VBMan.Allocate(sizeof(SWavesVertex), vertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
			shoreWave->m_VBvertices->m_Owner->UpdateChunkVertices(shoreWave->m_VBvertices, &vertices[0]);
			
			m_ShoreWaves.push_back(shoreWave);
		}
	}
}

void WaterManager::RenderWaves(const CFrustum& frustrum)
{
	if (g_Renderer.m_SkipSubmit || !m_WaterFancyEffects)
		return;
		
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_FancyEffectsFBO);
		
	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
	pglDrawBuffers(2, attachments);
		
	glClearColor(0.0f,0.0f, 0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	
	CShaderDefines none;
	CShaderProgramPtr shad = g_Renderer.GetShaderManager().LoadProgram("glsl/waves", none);
	
	shad->Bind();
	
	shad->BindTexture(str_waveTex, m_WaveTex);
	shad->BindTexture(str_foamTex, m_FoamTex);
	
	shad->Uniform(str_time, (float)m_WaterTexTimer);
	shad->Uniform(str_transform, g_Renderer.GetViewCamera().GetViewProjection());

	for (size_t a = 0; a < m_ShoreWaves.size(); ++a)
	{
		if (!frustrum.IsBoxVisible(CVector3D(0,0,0), m_ShoreWaves[a]->m_AABB))
			continue;
		
		CVertexBuffer::VBChunk* VBchunk = m_ShoreWaves[a]->m_VBvertices;
		SWavesVertex *base=(SWavesVertex *)VBchunk->m_Owner->Bind();
		
		// setup data pointers
		GLsizei stride = sizeof(SWavesVertex);
		shad->VertexPointer(3, GL_FLOAT, stride, &base[VBchunk->m_Index].m_BasePosition);
		shad->TexCoordPointer(GL_TEXTURE0, 2, GL_UNSIGNED_BYTE, stride, &base[VBchunk->m_Index].m_UV);
		//	NormalPointer(gl_FLOAT, stride, &base[m_VBWater->m_Index].m_UV)
		pglVertexAttribPointerARB(2, 2, GL_FLOAT, GL_TRUE, stride, &base[VBchunk->m_Index].m_PerpVect);	// replaces commented above because my normal is vec2
		shad->VertexAttribPointer(str_a_apexPosition, 3, GL_FLOAT, false, stride, &base[VBchunk->m_Index].m_ApexPosition);
		shad->VertexAttribPointer(str_a_splashPosition, 3, GL_FLOAT, false, stride, &base[VBchunk->m_Index].m_SplashPosition);
		shad->VertexAttribPointer(str_a_retreatPosition, 3, GL_FLOAT, false, stride, &base[VBchunk->m_Index].m_RetreatPosition);
		
		shad->AssertPointersBound();
		
		shad->Uniform(str_translation, m_ShoreWaves[a]->m_TimeDiff);
		shad->Uniform(str_width, (int)m_ShoreWaves[a]->m_Width);

		u8* indexBase = m_ShoreWaves_VBIndices->m_Owner->Bind();
		glDrawElements(GL_TRIANGLES, (GLsizei) (m_ShoreWaves[a]->m_Width-1)*(7*6),
					   GL_UNSIGNED_SHORT, indexBase + sizeof(u16)*(m_ShoreWaves_VBIndices->m_Index));
		
		shad->Uniform(str_translation, m_ShoreWaves[a]->m_TimeDiff + 6.0f);
		
		// TODO: figure out why this doesn't work.
		//g_Renderer.m_Stats.m_DrawCalls++;
		//g_Renderer.m_Stats.m_WaterTris += m_ShoreWaves_VBIndices->m_Count / 3;
		
		CVertexBuffer::Unbind();
	}
	shad->Unbind();
	pglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
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
