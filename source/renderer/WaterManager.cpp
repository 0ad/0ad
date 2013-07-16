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
	m_WaterColor = CColor(0.3f, 0.35f, 0.7f, 1.0f);
	m_WaterFullDepth = 5.0f;
	m_WaterMaxAlpha = 1.0f;
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
	m_SpecularStrength = 0.6f;
	m_Waviness = 8.0f;
	m_ReflectionTint = CColor(0.28f, 0.3f, 0.59f, 1.0f);
	m_ReflectionTintStrength = 0.0f;
	m_WaterTint = CColor(0.28f, 0.3f, 0.59f, 1.0f);
	m_Murkiness = 0.45f;
	m_RepeatPeriod = 16.0f;
	m_WaveX = NULL;
	m_WaveZ = NULL;
	m_DistanceToShore = NULL;
	m_FoamFactor = NULL;

	m_WaterNormal = false;
	m_WaterRealDepth = false;
	m_WaterFoam = false;
	m_WaterCoastalWaves = false;
	m_WaterRefraction = false;
	m_WaterReflection = false;
	m_WaterShadows = false;
	
	m_NeedsReloading = false;
	m_NeedsFullReloading = true;
	m_TerrainChangeThisTurn = false;
	
	m_VBWaves = NULL;
	m_VBWavesIndices = NULL;

	m_depthTT = 0;
	m_waveTT = 0;

}

WaterManager::~WaterManager()
{
	// Cleanup if the caller messed up
	UnloadWaterTextures();
	delete[] m_WaveX;
	delete[] m_WaveZ;
	delete[] m_DistanceToShore;
	delete[] m_FoamFactor;
	
	glDeleteTextures(1, &m_depthTT);
	glDeleteTextures(1, &m_waveTT);
	
	if (m_VBWaves) g_VBMan.Release(m_VBWaves);
	if (m_VBWavesIndices) g_VBMan.Release(m_VBWavesIndices);

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
	// Load foam (for fancy water)
	{
		CTextureProperties textureProps("art/textures/terrain/types/water/foam.png");
		textureProps.SetWrap(GL_REPEAT);
		
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_Foam = texture;
	}
	// Load waves (for fancy water)
	{
		CTextureProperties textureProps("art/textures/terrain/types/water/shore_wave.png");
		textureProps.SetWrap(GL_REPEAT);
		
		CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(textureProps);
		texture->Prefetch();
		m_Wave = texture;
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

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

///////////////////////////////////////////////////////////////////
// Create information about the terrain and wave vertices.
void WaterManager::CreateSuperfancyInfo(CSimulation2* simulation)
{
	if (m_VBWaves)
	{
		g_VBMan.Release(m_VBWaves);
		m_VBWaves = NULL;
	}
	if (m_VBWavesIndices)
	{
		g_VBMan.Release(m_VBWavesIndices);
		m_VBWavesIndices = NULL;
	}

	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	ssize_t mapSize = terrain->GetVerticesPerSide();
	
	CmpPtr<ICmpWaterManager> cmpWaterManager(*simulation, SYSTEM_ENTITY);
	if (!cmpWaterManager)
		return;	// REALLY shouldn't happen and will most likely crash.

	// Using this to get some more optimization on circular maps
	CmpPtr<ICmpRangeManager> cmpRangeManager(*simulation, SYSTEM_ENTITY);
	if (!cmpRangeManager)
		return;
	bool circular = cmpRangeManager->GetLosCircular();
	float mSize = mapSize*mapSize;
	float halfSize = (mapSize/2.0);

	// Warning: this won't work with multiple water planes
	m_WaterHeight = cmpWaterManager->GetExactWaterLevel(0,0);
	
	// TODO: change this whenever we incrementally update because it's def. not too efficient
	delete[] m_WaveX;
	delete[] m_WaveZ;
	delete[] m_DistanceToShore;
	delete[] m_FoamFactor;
	
	m_WaveX = new float[mapSize*mapSize];
	m_WaveZ = new float[mapSize*mapSize];
	m_DistanceToShore = new float[mapSize*mapSize];
	m_FoamFactor = new float[mapSize*mapSize];

	u16* heightmap = terrain->GetHeightMap();
	
	// some temporary stuff for wave intensity
	// not really used too much right now.
	u8* waveForceHQ = new u8[mapSize*mapSize];
	u16 waterHeightInu16 = m_WaterHeight/HEIGHT_SCALE;

	// used to cache terrain normals since otherwise we'd recalculate them a lot (I'm blurring the "normal" map).
	// this might be updated to actually cache in the terrain manager but that's not for now.
	CVector3D* normals = new CVector3D[mapSize*mapSize];

	// calculate wave force (not really used right now)
	// and puts into "normals" the terrain normal at that point
	// so as to avoid recalculating terrain normals too often.
	for (ssize_t i = 0; i < mapSize; ++i)
	{
		for (ssize_t j = 0; j < mapSize; ++j)
		{
			normals[j*mapSize + i] = terrain->CalcExactNormal(((float)i)*4.0f,((float)j)*4.0f);
			if (circular && (i-halfSize)*(i-halfSize)+(j-halfSize)*(j-halfSize) > mSize)
			{
				waveForceHQ[j*mapSize + i] = 255;
				continue;
			}
			u8 color = 0;
			for (int v = 0; v <= 18; v += 3){
				if (j-v >= 0 && i-v >= 0 && heightmap[(j-v)*mapSize + i-v] > waterHeightInu16)
				{
					if (color == 0)
						color = 5;
					else
						color++;
				}
			}
			waveForceHQ[j*mapSize + i] = 255 - color * 40;
		}
	}
	// this creates information for waves and stores it in float arrays. PatchRData then puts it in the vertex info for speed.
	for (ssize_t i = 0; i < mapSize; ++i)
	{
		for (ssize_t j = 0; j < mapSize; ++j)
		{
			if (circular && (i-halfSize)*(i-halfSize)+(j-halfSize)*(j-halfSize) > mSize)
			{
				m_WaveX[j*mapSize + i] = 0.0f;
				m_WaveZ[j*mapSize + i] = 0.0f;
				m_DistanceToShore[j*mapSize + i] = 100;
				m_FoamFactor[j*mapSize + i] = 0.0f;
				continue;
			}
			float depth = m_WaterHeight - heightmap[j*mapSize + i]*HEIGHT_SCALE;
			int distanceToShore = 10000;
			
			// calculation of the distance to the shore.
			// TODO: this is fairly dumb, though it returns a good result
			// Could be sped up a fair bit.
			if (depth >= 0)
			{
				// check in the square around.
				for (int xx = -5; xx <= 5; ++xx)
				{
					for (int yy = -5; yy <= 5; ++yy)
					{
						if (i+xx >= 0 && i + xx < mapSize)
							if (j + yy >= 0 && j + yy < mapSize)
							{
								float hereDepth = m_WaterHeight - heightmap[(j+yy)*mapSize + (i+xx)]*HEIGHT_SCALE;
								if (hereDepth < 0 && xx*xx + yy*yy < distanceToShore)
									distanceToShore = xx*xx + yy*yy;
							}
					}
				}
				// refine the calculation if we're close enough
				if (distanceToShore < 9)
				{
					for (float xx = -2.5f; xx <= 2.5f; ++xx)
					{
						for (float yy = -2.5f; yy <= 2.5f; ++yy)
						{
							float hereDepth = m_WaterHeight - terrain->GetExactGroundLevel( (i+xx)*4, (j+yy)*4 );
							if (hereDepth < 0 && xx*xx + yy*yy < distanceToShore)
								distanceToShore = xx*xx + yy*yy;
						}
					}
				}
			}
			else
			{
				for (int xx = -2; xx <= 2; ++xx)
				{
					for (int yy = -2; yy <= 2; ++yy)
					{
						float hereDepth = m_WaterHeight - terrain->GetVertexGroundLevel(i+xx, j+yy);
						if (hereDepth > 0)
							distanceToShore = 0;
					}
				}
				
			}
			// speedup with default values for land squares
			if (distanceToShore == 10000)
			{
				m_WaveX[j*mapSize + i] = 0.0f;
				m_WaveZ[j*mapSize + i] = 0.0f;
				m_DistanceToShore[j*mapSize + i] = 100;
				m_FoamFactor[j*mapSize + i] = 0.0f;
				continue;
			}
			// We'll compute the normals and the "water raise", to know about foam
			// Normals are a pretty good calculation but it's slow since we normalize so much.
			CVector3D normal;
			int waterRaise = 0;
			for (int xx = -4; xx <= 4; xx += 2)	// every 2 tile is good enough.
			{
				for (int yy = -4; yy <= 4; yy += 2)
				{
					if (j+yy < mapSize && i+xx < mapSize && i+xx >= 0 && j+yy >= 0)
						normal += normals[(j+yy)*mapSize + (i+xx)];
					if (terrain->GetVertexGroundLevel(i+xx,j+yy) < heightmap[j*mapSize + i]*HEIGHT_SCALE)
						waterRaise += heightmap[j*mapSize + i]*HEIGHT_SCALE - terrain->GetVertexGroundLevel(i+xx,j+yy);
				}
			}
			// normalizes the terrain info to avoid foam moving at too different speeds.
			normal *= 0.012345679f;
			normal[1] = 0.1f;
			normal = normal.Normalized();

			m_WaveX[j*mapSize + i] = normal[0];
			m_WaveZ[j*mapSize + i] = normal[2];
			// distance is /5.0 to be a [0,1] value.

			m_DistanceToShore[j*mapSize + i] = sqrtf(distanceToShore)/5.0f; // TODO: this can probably be cached as I'm integer here.

			// computing the amount of foam I want

			depth = clamp(depth,0.0f,10.0f);
			float foamAmount = (waterRaise/255.0f) * (1.0f - depth/10.0f) * (waveForceHQ[j*mapSize+i]/255.0f) * (m_Waviness/8.0f);
			foamAmount += clamp(m_Waviness/2.0f - distanceToShore,0.0f,m_Waviness/2.0f)/(m_Waviness/2.0f) * clamp(m_Waviness/9.0f,0.3f,1.0f);
			foamAmount = foamAmount > 1.0f ? 1.0f: foamAmount;
			
			m_FoamFactor[j*mapSize + i] = foamAmount;
		}
	}

	delete[] normals;
	delete[] waveForceHQ;
	
	// TODO: The rest should be cleaned up
	
	// okay let's create the waves squares. i'll divide the map in arbitrary squares
	// For each of these squares, check if waves are needed.
	// If yes, look for the best positionning (in order to have a nice blending with the shore)
	// Then clean-up: remove squares that are too close to each other
	
	std::vector<CVector2D> waveSquares;
	
	int size = 8;	// I think this is the size of the squares.
	for (int i = 0; i < mapSize/size; ++i)
	{
		for (int j = 0; j < mapSize/size; ++j)
		{
			
			int landTexel = 0;
			int waterTexel = 0;
			CVector3D avnormal (0.0f,0.0f,0.0f);
			CVector2D landPosition(0.0f,0.0f);
			CVector2D waterPosition(0.0f,0.0f);
			for (int xx = 0; xx < size; ++xx)
			{
				for (int yy = 0; yy < size; ++yy)
				{
					if (terrain->GetVertexGroundLevel(i*size+xx,j*size+yy) > m_WaterHeight)
					{
						landTexel++;
						landPosition += CVector2D(i*size+xx,j*size+yy);
					}
					else
					{
						waterPosition += CVector2D(i*size+xx,j*size+yy);
						waterTexel++;
						avnormal += terrain->CalcExactNormal( (i*size+xx)*4.0f,(j*size+yy)*4.0f);
					}
				}
			}
			if (landTexel < size/2)
				continue;

			landPosition /= landTexel;
			waterPosition /= waterTexel;
			
			avnormal[1] = 1.0f;
			avnormal.Normalize();
			avnormal[1] = 0.0f;
			
			// this should help ensure that the shore is pretty flat.
			if (avnormal.Length() <= 0.2f)
				continue;
			
			// To get the best position for squares, I start at the mean "ocean" position
			// And step by step go to the mean "land" position. I keep the position where I change from water to land.
			// If this never happens, the square is scrapped.
			if (terrain->GetExactGroundLevel(waterPosition.X*4.0f,waterPosition.Y*4.0f) > m_WaterHeight)
				continue;
			
			CVector2D squarePos(-1,-1);
			for (u8 i = 0; i < 40; i++)
			{
				squarePos = landPosition * (i/40.0f) + waterPosition * (1.0f-(i/40.0f));
				if (terrain->GetExactGroundLevel(squarePos.X*4.0f,squarePos.Y*4.0f) > m_WaterHeight)
					break;
			}
			if (squarePos.X == -1)
				continue;
			
			u8 enter = 1;
			// okaaaaaay. Got a square. Check for proximity.
			for (unsigned long i = 0; i < waveSquares.size(); i++)
			{
				if ( CVector2D(waveSquares[i]-squarePos).LengthSquared() < 80) {
					enter = 0;
					break;
				}
			}
			if (enter == 1)
				waveSquares.push_back(squarePos);
		}
	}
	
	// Actually create the waves' meshes.
	std::vector<SWavesVertex> waves_vertex_data;
	std::vector<GLushort> waves_indices;
	
	// loop through each square point. Look in the square around it, calculate the normal
	// create the square.
	for (unsigned long i = 0; i < waveSquares.size(); i++)
	{
		CVector2D pos(waveSquares[i]);
		
		CVector3D avgnorm(0.0f,0.0f,0.0f);
		for (int xx = -size/2; xx < size/2; ++xx)
		{
			for (int yy = -size/2; yy < size/2; ++yy)
			{
				avgnorm += terrain->CalcExactNormal((pos.X+xx)*4.0f,(pos.Y+yy)*4.0f);
			}
		}
		avgnorm[1] = 0.1f;
		// okay crank out a square.
		// we have the direction of the square. We'll get the perpendicular vector too
		CVector2D perp(-avgnorm[2],avgnorm[0]);
		perp = perp.Normalized();
		avgnorm = avgnorm.Normalized();
		
		SWavesVertex vertex[4];
		vertex[0].m_Position = CVector3D(pos.X + perp.X*(size/2.2f) - avgnorm[0]*1.0f, 0.0f,pos.Y + perp.Y*(size/2.2f) - avgnorm[2]*1.0f);
		vertex[0].m_Position *= 4.0f;
		vertex[0].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[0].m_UV[1] = 1;
		vertex[0].m_UV[0] = 0;
		
		vertex[1].m_Position = CVector3D(pos.X - perp.X*(size/2.2f) - avgnorm[0]*1.0f, 0.0f,pos.Y - perp.Y*(size/2.2f) - avgnorm[2]*1.0f);
		vertex[1].m_Position *= 4.0f;
		vertex[1].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[1].m_UV[1] = 1;
		vertex[1].m_UV[0] = 1;
		
		vertex[3].m_Position = CVector3D(pos.X + perp.X*(size/2.2f) + avgnorm[0]*(size/1.5f), 0.0f,pos.Y + perp.Y*(size/2.2f) + avgnorm[2]*(size/1.5f));
		vertex[3].m_Position *= 4.0f;
		vertex[3].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[3].m_UV[1] = 0;
		vertex[3].m_UV[0] = 0;
		
		vertex[2].m_Position = CVector3D(pos.X - perp.X*(size/2.2f) + avgnorm[0]*(size/1.5f), 0.0f,pos.Y - perp.Y*(size/2.2f) + avgnorm[2]*(size/1.5f));
		vertex[2].m_Position *= 4.0f;
		vertex[2].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[2].m_UV[1] = 0;
		vertex[2].m_UV[0] = 1;
		
		waves_indices.push_back(waves_vertex_data.size());
		waves_vertex_data.push_back(vertex[0]);
		waves_indices.push_back(waves_vertex_data.size());
		waves_vertex_data.push_back(vertex[1]);
		waves_indices.push_back(waves_vertex_data.size());
		waves_vertex_data.push_back(vertex[2]);
		waves_indices.push_back(waves_vertex_data.size());
		waves_vertex_data.push_back(vertex[3]);
	}

	// no vertex buffers if no data generated
	if (waves_indices.empty())
		return;

	// waves
	// allocate vertex buffer
	m_VBWaves = g_VBMan.Allocate(sizeof(SWavesVertex), waves_vertex_data.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
	m_VBWaves->m_Owner->UpdateChunkVertices(m_VBWaves, &waves_vertex_data[0]);

	// Construct indices buffer
	m_VBWavesIndices = g_VBMan.Allocate(sizeof(GLushort), waves_indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
	m_VBWavesIndices->m_Owner->UpdateChunkVertices(m_VBWavesIndices, &waves_indices[0]);
}

////////////////////////////////////////////////////////////////////////
// This will set the bools properly
void WaterManager::updateQuality()
{
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERNORMAL) != m_WaterNormal) {
		m_WaterNormal = g_Renderer.GetOptionBool(CRenderer::OPT_WATERNORMAL);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERREALDEPTH) != m_WaterRealDepth) {
		m_WaterRealDepth = g_Renderer.GetOptionBool(CRenderer::OPT_WATERREALDEPTH);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERFOAM) != m_WaterFoam) {
		m_WaterFoam = g_Renderer.GetOptionBool(CRenderer::OPT_WATERFOAM);
		m_NeedsReloading = true;
		m_NeedsFullReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERCOASTALWAVES) != m_WaterCoastalWaves) {
		m_WaterCoastalWaves = g_Renderer.GetOptionBool(CRenderer::OPT_WATERCOASTALWAVES);
		m_NeedsReloading = true;
		m_NeedsFullReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFRACTION) != m_WaterRefraction) {
		m_WaterRefraction = g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFRACTION);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFLECTION) != m_WaterReflection) {
		m_WaterReflection = g_Renderer.GetOptionBool(CRenderer::OPT_WATERREFLECTION);
		m_NeedsReloading = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERSHADOW) != m_WaterShadows) {
		m_WaterShadows = g_Renderer.GetOptionBool(CRenderer::OPT_WATERSHADOW);
		m_NeedsReloading = true;
	}
}

bool WaterManager::WillRenderFancyWater()
{
	if (!g_Renderer.GetCapabilities().m_FragmentShader)
		return false;
	if (!m_RenderWater)
		return false;
	return true;
}
