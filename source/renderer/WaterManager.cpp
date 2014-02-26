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
	m_NeedInfoUpdate = true;
	
	m_VBWaves = NULL;
	m_VBWavesIndices = NULL;

	m_depthTT = 0;
	m_waveTT = 0;

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
		textureProps.SetMaxAnisotropy(4);
		
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
	
	CmpPtr<ICmpWaterManager> cmpWaterManager(*simulation, SYSTEM_ENTITY);
	if (!cmpWaterManager)
		return;	// REALLY shouldn't happen and will most likely crash.

	// Using this to get some more optimization on circular maps
	CmpPtr<ICmpRangeManager> cmpRangeManager(*simulation, SYSTEM_ENTITY);
	if (!cmpRangeManager)
		return;
	bool circular = cmpRangeManager->GetLosCircular();
	float mSize = m_MapSize*m_MapSize;
	float halfSize = (m_MapSize/2.0);

	// Warning: this won't work with multiple water planes
	m_WaterHeight = cmpWaterManager->GetExactWaterLevel(0,0);
	
	// Get the square we want to work on.
	size_t Xstart = m_updatei0 >= m_MapSize ? m_MapSize-1 : m_updatei0;
	size_t Xend = m_updatei1 >= m_MapSize ? m_MapSize-1 : m_updatei1;
	size_t Zstart = m_updatej0 >= m_MapSize ? m_MapSize-1 : m_updatej0;
	size_t Zend = m_updatej1 >= m_MapSize ? m_MapSize-1 : m_updatej1;
	
	if (m_WaveX == NULL)
	{
		m_WaveX = new float[m_MapSize*m_MapSize];
		m_WaveZ = new float[m_MapSize*m_MapSize];
		m_DistanceToShore = new float[m_MapSize*m_MapSize];
		m_FoamFactor = new float[m_MapSize*m_MapSize];
	}

	u16* heightmap = terrain->GetHeightMap();
	
	// some temporary stuff for wave intensity
	// not really used too much right now.
	//u8* waveForceHQ = new u8[mapSize*mapSize];

	// used to cache terrain normals since otherwise we'd recalculate them a lot (I'm blurring the "normal" map).
	// this might be updated to actually cache in the terrain manager but that's not for now.
	CVector3D* normals = new CVector3D[m_MapSize*m_MapSize];

	// taken out of the bottom loop, blurs the normal map
	// To remove if below is reactivated
	size_t blurZstart = (int)(Zstart-4) < 0 ? 0 : Zstart - 4;
	size_t blurZend = Zend+4 >= m_MapSize ? m_MapSize-1 : Zend + 4;
	size_t blurXstart = (int)(Xstart-4) < 0 ? 0 : Xstart - 4;
	size_t blurXend = Xend+4 >= m_MapSize ? m_MapSize-1 : Xend + 4;
	
	float ii = blurXstart*4.0f, jj = blurXend*4.0f;
	for (size_t j = blurZstart; j < blurZend; ++j, jj += 4.0f)
	{
		for (size_t i = blurXstart; i < blurXend; ++i, ii += 4.0f)
		{
			normals[j*m_MapSize + i] = terrain->CalcExactNormal(ii,jj);
		}
	}
	// TODO: reactivate?
	/*
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
	 */

	// Cache some data to spiral-search for the closest tile that's either coastal or water depending on what we are.
	// this is insanely faster.
	// I use a define because it's more readable and C++11 doesn't like this otherwise
#define m_MapSize (ssize_t)m_MapSize
	ssize_t offset[24] = { -1,1,-m_MapSize,+m_MapSize, -1-m_MapSize,+1-m_MapSize,-1+m_MapSize,1+m_MapSize,
		-2,2,-2*m_MapSize,2*m_MapSize,-2-m_MapSize,-2+m_MapSize,2-m_MapSize,2+m_MapSize,
		-1-2*m_MapSize,+1-2*m_MapSize,-1+2*m_MapSize,1+2*m_MapSize,
		-2-2*m_MapSize,2+2*m_MapSize,-2+2*m_MapSize,2-2*m_MapSize };
	float dist[24] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.414f, 1.414f, 1.414f, 1.414f,
		2.0f, 2.0f, 2.0f, 2.0f, 2.236f, 2.236f, 2.236f, 2.236f,
		2.236f, 2.236f, 2.236f, 2.236f,
		2.828f, 2.828f, 2.828f, 2.828f };
#undef m_MapSize
	
	// this creates information for waves and stores it in float arrays. PatchRData then puts it in the vertex info for speed.
	CVector3D normal;
	for (size_t j = Zstart; j < Zend; ++j)
	{
		for (size_t i = Xstart; i < Xend; ++i)
		{
			ssize_t register index = j*m_MapSize + i;
			if (circular && (i-halfSize)*(i-halfSize)+(j-halfSize)*(j-halfSize) > mSize)
			{
				m_WaveX[index] = 0.0f;
				m_WaveZ[index] = 0.0f;
				m_DistanceToShore[index] = 100;
				m_FoamFactor[index] = 0.0f;
				continue;
			}
			float depth = m_WaterHeight - heightmap[index]*HEIGHT_SCALE;
			float register distanceToShore = 10000.0f;
			
			// calculation of the distance to the shore.
			if (i > 0 && i < m_MapSize-1 && j > 0 && j < m_MapSize-1)
			{
				// search a 5x5 array with us in the center (do not search me)
				// much faster since we spiral search and can just stop once we've found the shore.
				// also everything is precomputed and we get exact results instead.
				int max = 8;
				if (i > 1 && i < m_MapSize-2 && j > 1 && j < m_MapSize-2)
					max = 24;
				
				for(int lookupI = 0; lookupI < max;++lookupI)
				{
					float hereDepth = m_WaterHeight - heightmap[index+offset[lookupI]]*HEIGHT_SCALE;
					distanceToShore = hereDepth <= 0 && depth >= 0 ? dist[lookupI] : (depth < 0 ? 1 : distanceToShore);
					if (distanceToShore < 5000.0f)
						goto FoundShore;
				}
			} else {
				// revert to for and if-based because I can't be bothered to special case all that.
				for (int xx = -1; xx <= 1;++xx)
					for (int yy = -1; yy <= 1;++yy)
					{
						if ((int)(i+xx) >= 0 && i+xx < m_MapSize && (int)(j+yy) >= 0 && j+yy < m_MapSize)
						{
							float hereDepth = m_WaterHeight - heightmap[index+xx+yy*m_MapSize]*HEIGHT_SCALE;
							distanceToShore = (hereDepth < 0 && sqrt((double)xx*xx+yy*yy) < distanceToShore) ? sqrt((double)xx*xx+yy*yy) : distanceToShore;
						}
					}
			}

			// speedup with default values for land squares
			if (distanceToShore > 5000.0f)
			{
				m_WaveX[index] = 0.0f;
				m_WaveZ[index] = 0.0f;
				m_DistanceToShore[index] = 100.0f;
				m_FoamFactor[index] = 0.0f;
				continue;
			}
		FoundShore:
			// We'll compute the normals and the "water raise", to know about foam
			// Normals are a pretty good calculation but it's slow since we normalize so much.
			normal.X = normal.Y = normal.Z = 0.0f;
			int waterRaise = 0;
			for (size_t yy = (int(j-3) < 0 ? 0 : j-3); yy <= (j+3 < m_MapSize-1 ? 0 : j-3); yy += 2)
			{
				for (size_t xx = (int(i-3) < 0 ? 0 : i-3); xx <= (i+3 < m_MapSize-1 ? 0 : i+3); xx += 2)	// every 2 tile is good enough.
				{
					normal += normals[yy*m_MapSize + xx];
					waterRaise += (heightmap[index]*HEIGHT_SCALE - heightmap[yy*m_MapSize + xx]) > 0 ? (heightmap[index]*HEIGHT_SCALE - heightmap[yy*m_MapSize + xx]) : 0;
				}
			}
			// normalizes the terrain info to avoid foam moving at too different speeds.
			normal *= 0.08f;	// divide by about 11.
			normal[1] = 0.1f;
			normal = normal.Normalized();

			m_WaveX[index] = normal[0];
			m_WaveZ[index] = normal[2];
			// distance is /5.0 to be a [0,1] value.

			m_DistanceToShore[index] = distanceToShore;

			// computing the amount of foam I want
			depth = clamp(depth,0.0f,10.0f);
			float foamAmount = (waterRaise/255.0f) * (1.0f - depth/10.0f) /** (waveForceHQ[j*m_MapSize+i]/255.0f)*/ * (m_Waviness/8.0f);
			foamAmount += clamp(m_Waviness/2.0f,0.0f,m_Waviness/2.0f)/(m_Waviness/2.0f) * clamp(m_Waviness/9.0f,0.3f,1.0f);
			foamAmount *= (m_Waviness/4.0f - distanceToShore);
			foamAmount = foamAmount > 1.0f ? 1.0f: (foamAmount < 0.0f ? 0.0f : foamAmount);
			
			m_FoamFactor[index] = foamAmount;
		}
	}

	delete[] normals;
	//delete[] waveForceHQ;
	
	// TODO: reactivate this with something that looks good and is efficient.
/*
	// okay let's create the waves squares. i'll divide the map in arbitrary squares
	// For each of these squares, check if waves are needed.
	// If yes, look for the best positionning (in order to have a nice blending with the shore)
	// Then clean-up: remove squares that are too close to each other
	
	std::vector<CVector2D> waveSquares;
	
	int size = 8;	// I think this is the size of the squares.
	for (size_t j = 0; j < m_MapSize/size; ++j)
	{
		for (size_t i = 0; i < m_MapSize/size; ++i)
		{
			int landTexel = 0;
			int waterTexel = 0;
			CVector3D avnormal (0.0f,0.0f,0.0f);
			CVector2D landPosition(0.0f,0.0f);
			CVector2D waterPosition(0.0f,0.0f);
			for (int yy = 0; yy < size; ++yy)
			{
				for (int xx = 0; xx < size; ++xx)
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
		for (int yy = -size/2; yy < size/2; ++yy)
		{
			for (int xx = -size/2; xx < size/2; ++xx)
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
		
		GLushort index[4];
		SWavesVertex vertex[4];
		vertex[0].m_Position = CVector3D(pos.X + perp.X*(size/2.2f) - avgnorm[0]*1.0f, 0.0f,pos.Y + perp.Y*(size/2.2f) - avgnorm[2]*1.0f);
		vertex[0].m_Position *= 4.0f;
		vertex[0].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[0].m_UV[1] = 1;
		vertex[0].m_UV[0] = 0;
		index[0] = waves_vertex_data.size();
		waves_vertex_data.push_back(vertex[0]);
		
		vertex[1].m_Position = CVector3D(pos.X - perp.X*(size/2.2f) - avgnorm[0]*1.0f, 0.0f,pos.Y - perp.Y*(size/2.2f) - avgnorm[2]*1.0f);
		vertex[1].m_Position *= 4.0f;
		vertex[1].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[1].m_UV[1] = 1;
		vertex[1].m_UV[0] = 1;
		index[1] = waves_vertex_data.size();
		waves_vertex_data.push_back(vertex[1]);
		
		vertex[3].m_Position = CVector3D(pos.X + perp.X*(size/2.2f) + avgnorm[0]*(size/1.5f), 0.0f,pos.Y + perp.Y*(size/2.2f) + avgnorm[2]*(size/1.5f));
		vertex[3].m_Position *= 4.0f;
		vertex[3].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[3].m_UV[1] = 0;
		vertex[3].m_UV[0] = 0;
		index[3] = waves_vertex_data.size();
		waves_vertex_data.push_back(vertex[3]);
		
		vertex[2].m_Position = CVector3D(pos.X - perp.X*(size/2.2f) + avgnorm[0]*(size/1.5f), 0.0f,pos.Y - perp.Y*(size/2.2f) + avgnorm[2]*(size/1.5f));
		vertex[2].m_Position *= 4.0f;
		vertex[2].m_Position.Y = m_WaterHeight + 1.0f;
		vertex[2].m_UV[1] = 0;
		vertex[2].m_UV[0] = 1;
		index[2] = waves_vertex_data.size();
		waves_vertex_data.push_back(vertex[2]);

		waves_indices.push_back(index[0]);
		waves_indices.push_back(index[1]);
		waves_indices.push_back(index[2]);

		waves_indices.push_back(index[2]);
		waves_indices.push_back(index[3]);
		waves_indices.push_back(index[0]);
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
 */
}

////////////////////////////////////////////////////////////////////////
// This will always recalculate for now
void WaterManager::SetMapSize(size_t size)
{
	// TODO: Im' blindly trusting the user here.
	m_MapSize = size;
	m_NeedInfoUpdate = true;
	m_updatei0 = 0;
	m_updatei1 = size;
	m_updatej0 = 0;
	m_updatej1 = size;
	
	SAFE_ARRAY_DELETE(m_WaveX);
	SAFE_ARRAY_DELETE(m_WaveZ);
	SAFE_ARRAY_DELETE(m_DistanceToShore);
	SAFE_ARRAY_DELETE(m_FoamFactor);
}

////////////////////////////////////////////////////////////////////////
// This will set the bools properly
void WaterManager::UpdateQuality()
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
		m_NeedInfoUpdate = true;
	}
	if (g_Renderer.GetOptionBool(CRenderer::OPT_WATERCOASTALWAVES) != m_WaterCoastalWaves) {
		m_WaterCoastalWaves = g_Renderer.GetOptionBool(CRenderer::OPT_WATERCOASTALWAVES);
		m_NeedsReloading = true;
		m_NeedInfoUpdate = true;
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
