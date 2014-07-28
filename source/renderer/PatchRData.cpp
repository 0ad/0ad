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

#include <set>
#include <algorithm>
#include <numeric>

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Patch.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextRenderer.h"
#include "lib/alignment.h"
#include "lib/allocators/arena.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/World.h"
#include "ps/GameSetup/Config.h"
#include "renderer/AlphaMapCalculator.h"
#include "renderer/PatchRData.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"

const ssize_t BlendOffsets[9][2] = {
	{  0, -1 },
	{ -1, -1 },
	{ -1,  0 },
	{ -1,  1 },
	{  0,  1 },
	{  1,  1 },
	{  1,  0 },
	{  1, -1 },
	{  0,  0 }
};

///////////////////////////////////////////////////////////////////
// CPatchRData constructor
CPatchRData::CPatchRData(CPatch* patch, CSimulation2* simulation) :
	m_Patch(patch), m_VBSides(0),
	m_VBBase(0), m_VBBaseIndices(0),
	m_VBBlends(0), m_VBBlendIndices(0),
	m_VBWater(0), m_VBWaterIndices(0),
	m_VBWaterShore(0), m_VBWaterIndicesShore(0),
	m_Simulation(simulation)
{
	ENSURE(patch);
	Build();
}

///////////////////////////////////////////////////////////////////
// CPatchRData destructor
CPatchRData::~CPatchRData()
{
	// release vertex buffer chunks
	if (m_VBSides) g_VBMan.Release(m_VBSides);
	if (m_VBBase) g_VBMan.Release(m_VBBase);
	if (m_VBBaseIndices) g_VBMan.Release(m_VBBaseIndices);
	if (m_VBBlends) g_VBMan.Release(m_VBBlends);
	if (m_VBBlendIndices) g_VBMan.Release(m_VBBlendIndices);
	if (m_VBWater) g_VBMan.Release(m_VBWater);
	if (m_VBWaterIndices) g_VBMan.Release(m_VBWaterIndices);
	if (m_VBWaterShore) g_VBMan.Release(m_VBWaterShore);
	if (m_VBWaterIndicesShore) g_VBMan.Release(m_VBWaterIndicesShore);
}

/**
 * Represents a blend for a single tile, texture and shape.
 */
struct STileBlend
{
	CTerrainTextureEntry* m_Texture;
	int m_Priority;
	u16 m_TileMask; // bit n set if this blend contains neighbour tile BlendOffsets[n]

	struct DecreasingPriority
	{
		bool operator()(const STileBlend& a, const STileBlend& b) const
		{
			if (a.m_Priority > b.m_Priority)
				return true;
			if (a.m_Priority < b.m_Priority)
				return false;
			if (a.m_Texture && b.m_Texture)
				return a.m_Texture->GetTag() > b.m_Texture->GetTag();
			return false;
		}
	};

	struct CurrentTile
	{
		bool operator()(const STileBlend& a) const
		{
			return (a.m_TileMask & (1 << 8)) != 0;
		}
	};
};

/**
 * Represents the ordered collection of blends drawn on a particular tile.
 */
struct STileBlendStack
{
	u8 i, j;
	std::vector<STileBlend> blends; // back of vector is lowest-priority texture
};

/**
 * Represents a batched collection of blends using the same texture.
 */
struct SBlendLayer
{
	struct Tile
	{
		u8 i, j;
		u8 shape;
	};

	CTerrainTextureEntry* m_Texture;
	std::vector<Tile> m_Tiles;
};

void CPatchRData::BuildBlends()
{
	PROFILE3("build blends");

	m_BlendSplats.clear();

	std::vector<SBlendVertex> blendVertices;
	std::vector<u16> blendIndices;

	CTerrain* terrain = m_Patch->m_Parent;

	std::vector<STileBlendStack> blendStacks;
	blendStacks.reserve(PATCH_SIZE*PATCH_SIZE);

	// For each tile in patch ..
	for (ssize_t j = 0; j < PATCH_SIZE; ++j)
	{
		for (ssize_t i = 0; i < PATCH_SIZE; ++i)
		{
			ssize_t gx = m_Patch->m_X * PATCH_SIZE + i;
			ssize_t gz = m_Patch->m_Z * PATCH_SIZE + j;

			std::vector<STileBlend> blends;
			blends.reserve(9);

			// Compute a blend for every tile in the 3x3 square around this tile
			for (size_t n = 0; n < 9; ++n)
			{
				ssize_t ox = gx + BlendOffsets[n][1];
				ssize_t oz = gz + BlendOffsets[n][0];

				CMiniPatch* nmp = terrain->GetTile(ox, oz);
				if (!nmp)
					continue;

				STileBlend blend;
				blend.m_Texture = nmp->GetTextureEntry();
				blend.m_Priority = nmp->GetPriority();
				blend.m_TileMask = 1 << n;
				blends.push_back(blend);
			}

			// Sort the blends, highest priority first
			std::sort(blends.begin(), blends.end(), STileBlend::DecreasingPriority());

			STileBlendStack blendStack;
			blendStack.i = i;
			blendStack.j = j;

			// Put the blends into the tile's stack, merging any adjacent blends with the same texture
			for (size_t k = 0; k < blends.size(); ++k)
			{
				if (!blendStack.blends.empty() && blendStack.blends.back().m_Texture == blends[k].m_Texture)
					blendStack.blends.back().m_TileMask |= blends[k].m_TileMask;
				else
					blendStack.blends.push_back(blends[k]);
			}

			// Remove blends that are after (i.e. lower priority than) the current tile
			// (including the current tile), since we don't want to render them on top of
			// the tile's base texture
			blendStack.blends.erase(
				std::find_if(blendStack.blends.begin(), blendStack.blends.end(), STileBlend::CurrentTile()),
				blendStack.blends.end());

			blendStacks.push_back(blendStack);
		}
	}

	// Given the blend stack per tile, we want to batch together as many blends as possible.
	// Group them into a series of layers (each of which has a single texture):
	// (This is effectively a topological sort / linearisation of the partial order induced
	// by the per-tile stacks, preferring to make tiles with equal textures adjacent.)

	std::vector<SBlendLayer> blendLayers;

	while (true)
	{
		if (!blendLayers.empty())
		{
			// Try to grab as many tiles as possible that match our current layer,
			// from off the blend stacks of all the tiles

			CTerrainTextureEntry* tex = blendLayers.back().m_Texture;

			for (size_t k = 0; k < blendStacks.size(); ++k)
			{
				if (!blendStacks[k].blends.empty() && blendStacks[k].blends.back().m_Texture == tex)
				{
					SBlendLayer::Tile t = { blendStacks[k].i, blendStacks[k].j, (u8)blendStacks[k].blends.back().m_TileMask };
					blendLayers.back().m_Tiles.push_back(t);
					blendStacks[k].blends.pop_back();
				}
				// (We've already merged adjacent entries of the same texture in each stack,
				// so we don't need to bother looping to check the next entry in this stack again)
			}
		}

		// We've grabbed as many tiles as possible; now we need to start a new layer.
		// The new layer's texture could come from the back of any non-empty stack;
		// choose the longest stack as a heuristic to reduce the number of layers
		CTerrainTextureEntry* bestTex = NULL;
		size_t bestStackSize = 0;

		for (size_t k = 0; k < blendStacks.size(); ++k)
		{
			if (blendStacks[k].blends.size() > bestStackSize)
			{
				bestStackSize = blendStacks[k].blends.size();
				bestTex = blendStacks[k].blends.back().m_Texture;
			}
		}

		// If all our stacks were empty, we're done
		if (bestStackSize == 0)
			break;

		// Otherwise add the new layer, then loop back and start filling it in

		SBlendLayer layer;
		layer.m_Texture = bestTex;
		blendLayers.push_back(layer);
	}

	// Now build outgoing splats
	m_BlendSplats.resize(blendLayers.size());

	for (size_t k = 0; k < blendLayers.size(); ++k)
	{
		SSplat& splat = m_BlendSplats[k];
		splat.m_IndexStart = blendIndices.size();
		splat.m_Texture = blendLayers[k].m_Texture;

		for (size_t t = 0; t < blendLayers[k].m_Tiles.size(); ++t)
		{
			SBlendLayer::Tile& tile = blendLayers[k].m_Tiles[t];
			AddBlend(blendVertices, blendIndices, tile.i, tile.j, tile.shape, splat.m_Texture);
		}

		splat.m_IndexCount = blendIndices.size() - splat.m_IndexStart;
	}

	// Release existing vertex buffer chunks
	if (m_VBBlends)
	{
		g_VBMan.Release(m_VBBlends);
		m_VBBlends = 0;
	}

	if (m_VBBlendIndices)
	{
		g_VBMan.Release(m_VBBlendIndices);
		m_VBBlendIndices = 0;
	}

	if (blendVertices.size())
	{
		// Construct vertex buffer

		m_VBBlends = g_VBMan.Allocate(sizeof(SBlendVertex), blendVertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends, &blendVertices[0]);

		// Update the indices to include the base offset of the vertex data
		for (size_t k = 0; k < blendIndices.size(); ++k)
			blendIndices[k] += m_VBBlends->m_Index;

		m_VBBlendIndices = g_VBMan.Allocate(sizeof(u16), blendIndices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
		m_VBBlendIndices->m_Owner->UpdateChunkVertices(m_VBBlendIndices, &blendIndices[0]);
	}
}

void CPatchRData::AddBlend(std::vector<SBlendVertex>& blendVertices, std::vector<u16>& blendIndices, 
			   u16 i, u16 j, u8 shape, CTerrainTextureEntry* texture)
{
	CTerrain* terrain = m_Patch->m_Parent;

	ssize_t gx = m_Patch->m_X * PATCH_SIZE + i;
	ssize_t gz = m_Patch->m_Z * PATCH_SIZE + j;

	// uses the current neighbour texture
	BlendShape8 shape8;
	for (size_t m = 0; m < 8; ++m)
		shape8[m] = (shape & (1 << m)) ? 0 : 1;

	// calculate the required alphamap and the required rotation of the alphamap from blendshape
	unsigned int alphamapflags;
	int alphamap = CAlphaMapCalculator::Calculate(shape8, alphamapflags);

	// now actually render the blend tile (if we need one)
	if (alphamap == -1)
		return;
	
	float u0 = texture->m_TerrainAlpha->second.m_AlphaMapCoords[alphamap].u0;
	float u1 = texture->m_TerrainAlpha->second.m_AlphaMapCoords[alphamap].u1;
	float v0 = texture->m_TerrainAlpha->second.m_AlphaMapCoords[alphamap].v0;
	float v1 = texture->m_TerrainAlpha->second.m_AlphaMapCoords[alphamap].v1;

	if (alphamapflags & BLENDMAP_FLIPU)
		std::swap(u0, u1);

	if (alphamapflags & BLENDMAP_FLIPV)
		std::swap(v0, v1);

	int base = 0;
	if (alphamapflags & BLENDMAP_ROTATE90)
		base = 1;
	else if (alphamapflags & BLENDMAP_ROTATE180)
		base = 2;
	else if (alphamapflags & BLENDMAP_ROTATE270)
		base = 3;

	SBlendVertex vtx[4];
	vtx[(base + 0) % 4].m_AlphaUVs[0] = u0;
	vtx[(base + 0) % 4].m_AlphaUVs[1] = v0;
	vtx[(base + 1) % 4].m_AlphaUVs[0] = u1;
	vtx[(base + 1) % 4].m_AlphaUVs[1] = v0;
	vtx[(base + 2) % 4].m_AlphaUVs[0] = u1;
	vtx[(base + 2) % 4].m_AlphaUVs[1] = v1;
	vtx[(base + 3) % 4].m_AlphaUVs[0] = u0;
	vtx[(base + 3) % 4].m_AlphaUVs[1] = v1;

	SBlendVertex dst;

	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();
	CVector3D normal;

	bool cpuLighting = (g_Renderer.GetRenderPath() == CRenderer::RP_FIXED);

	size_t index = blendVertices.size();

	terrain->CalcPosition(gx, gz, dst.m_Position);
	terrain->CalcNormal(gx, gz, normal);
	dst.m_Normal = normal;
	dst.m_DiffuseColor = cpuLighting ? lightEnv.EvaluateTerrainDiffuseScaled(normal) : lightEnv.EvaluateTerrainDiffuseFactor(normal);
	dst.m_AlphaUVs[0] = vtx[0].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[0].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	terrain->CalcPosition(gx + 1, gz, dst.m_Position);
	terrain->CalcNormal(gx + 1, gz, normal);
	dst.m_Normal = normal;
	dst.m_DiffuseColor = cpuLighting ? lightEnv.EvaluateTerrainDiffuseScaled(normal) : lightEnv.EvaluateTerrainDiffuseFactor(normal);
	dst.m_AlphaUVs[0] = vtx[1].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[1].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	terrain->CalcPosition(gx + 1, gz + 1, dst.m_Position);
	terrain->CalcNormal(gx + 1, gz + 1, normal);
	dst.m_Normal = normal;
	dst.m_DiffuseColor = cpuLighting ? lightEnv.EvaluateTerrainDiffuseScaled(normal) : lightEnv.EvaluateTerrainDiffuseFactor(normal);
	dst.m_AlphaUVs[0] = vtx[2].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[2].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	terrain->CalcPosition(gx, gz + 1, dst.m_Position);
	terrain->CalcNormal(gx, gz + 1, normal);
	dst.m_Normal = normal;
	dst.m_DiffuseColor = cpuLighting ? lightEnv.EvaluateTerrainDiffuseScaled(normal) : lightEnv.EvaluateTerrainDiffuseFactor(normal);
	dst.m_AlphaUVs[0] = vtx[3].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[3].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	bool dir = terrain->GetTriangulationDir(gx, gz);
	if (dir)
	{
		blendIndices.push_back(index+0);
		blendIndices.push_back(index+1);
		blendIndices.push_back(index+3);

		blendIndices.push_back(index+1);
		blendIndices.push_back(index+2);
		blendIndices.push_back(index+3);
	}
	else
	{
		blendIndices.push_back(index+0);
		blendIndices.push_back(index+1);
		blendIndices.push_back(index+2);

		blendIndices.push_back(index+2);
		blendIndices.push_back(index+3);
		blendIndices.push_back(index+0);
	}
}

void CPatchRData::BuildIndices()
{
	PROFILE3("build indices");

	CTerrain* terrain = m_Patch->m_Parent;

	ssize_t px = m_Patch->m_X * PATCH_SIZE;
	ssize_t pz = m_Patch->m_Z * PATCH_SIZE;

	// must have allocated some vertices before trying to build corresponding indices
	ENSURE(m_VBBase);

	// number of vertices in each direction in each patch
	ssize_t vsize=PATCH_SIZE+1;

	std::vector<unsigned short> indices;
	indices.reserve(PATCH_SIZE * PATCH_SIZE * 4);

	// release existing splats
	m_Splats.clear();

	// build grid of textures on this patch
	std::vector<CTerrainTextureEntry*> textures;
	CTerrainTextureEntry* texgrid[PATCH_SIZE][PATCH_SIZE];
	for (ssize_t j=0;j<PATCH_SIZE;j++) {
		for (ssize_t i=0;i<PATCH_SIZE;i++) {
			CTerrainTextureEntry* tex=m_Patch->m_MiniPatches[j][i].GetTextureEntry();
			texgrid[j][i]=tex;
			if (std::find(textures.begin(),textures.end(),tex)==textures.end()) {
				textures.push_back(tex);
			}
		}
	}

	// now build base splats from interior textures
	m_Splats.resize(textures.size());
	// build indices for base splats
	size_t base=m_VBBase->m_Index;
	ENSURE(base + vsize*vsize < 65536); // mustn't overflow u16 indexes
	for (size_t i=0;i<m_Splats.size();i++) {
		CTerrainTextureEntry* tex=textures[i];

		SSplat& splat=m_Splats[i];
		splat.m_Texture=tex;
		splat.m_IndexStart=indices.size();

		for (ssize_t j = 0; j < PATCH_SIZE; j++)
		{
			for (ssize_t i = 0; i < PATCH_SIZE; i++)
			{
				if (texgrid[j][i] == tex)
				{
					bool dir = terrain->GetTriangulationDir(px+i, pz+j);
					if (dir)
					{
						indices.push_back(u16(((j+0)*vsize+(i+0))+base));
						indices.push_back(u16(((j+0)*vsize+(i+1))+base));
						indices.push_back(u16(((j+1)*vsize+(i+0))+base));

						indices.push_back(u16(((j+0)*vsize+(i+1))+base));
						indices.push_back(u16(((j+1)*vsize+(i+1))+base));
						indices.push_back(u16(((j+1)*vsize+(i+0))+base));
					}
					else
					{
						indices.push_back(u16(((j+0)*vsize+(i+0))+base));
						indices.push_back(u16(((j+0)*vsize+(i+1))+base));
						indices.push_back(u16(((j+1)*vsize+(i+1))+base));

						indices.push_back(u16(((j+1)*vsize+(i+1))+base));
						indices.push_back(u16(((j+1)*vsize+(i+0))+base));
						indices.push_back(u16(((j+0)*vsize+(i+0))+base));
					}
				}
			}
		}
		splat.m_IndexCount=indices.size()-splat.m_IndexStart;
	}

	// Release existing vertex buffer chunk
	if (m_VBBaseIndices)
	{
		g_VBMan.Release(m_VBBaseIndices);
		m_VBBaseIndices = 0;
	}

	ENSURE(indices.size());

	// Construct vertex buffer
	m_VBBaseIndices = g_VBMan.Allocate(sizeof(u16), indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
	m_VBBaseIndices->m_Owner->UpdateChunkVertices(m_VBBaseIndices, &indices[0]);
}


void CPatchRData::BuildVertices()
{
	PROFILE3("build vertices");

	// create both vertices and lighting colors

	// number of vertices in each direction in each patch
	ssize_t vsize=PATCH_SIZE+1;

	std::vector<SBaseVertex> vertices;
	vertices.resize(vsize*vsize);

	// get index of this patch
	ssize_t px=m_Patch->m_X;
	ssize_t pz=m_Patch->m_Z;

	CTerrain* terrain=m_Patch->m_Parent;
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	bool cpuLighting = (g_Renderer.GetRenderPath() == CRenderer::RP_FIXED);

	// build vertices
	for (ssize_t j=0;j<vsize;j++) {
		for (ssize_t i=0;i<vsize;i++) {
			ssize_t ix=px*PATCH_SIZE+i;
			ssize_t iz=pz*PATCH_SIZE+j;
			ssize_t v=(j*vsize)+i;

			// calculate vertex data
			terrain->CalcPosition(ix,iz,vertices[v].m_Position);

			// Calculate diffuse lighting for this vertex
			// Ambient is added by the lighting pass (since ambient is the same
			// for all vertices, it need not be stored in the vertex structure)
			CVector3D normal;
			terrain->CalcNormal(ix,iz,normal);
			
			vertices[v].m_Normal = normal;

			vertices[v].m_DiffuseColor = cpuLighting ? lightEnv.EvaluateTerrainDiffuseScaled(normal) : lightEnv.EvaluateTerrainDiffuseFactor(normal);
		}
	}

	// upload to vertex buffer
	if (!m_VBBase)
		m_VBBase = g_VBMan.Allocate(sizeof(SBaseVertex), vsize * vsize, GL_STATIC_DRAW, GL_ARRAY_BUFFER);

	m_VBBase->m_Owner->UpdateChunkVertices(m_VBBase, &vertices[0]);
}

void CPatchRData::BuildSide(std::vector<SSideVertex>& vertices, CPatchSideFlags side)
{
	ssize_t vsize = PATCH_SIZE + 1;
	CTerrain* terrain = m_Patch->m_Parent;
	CmpPtr<ICmpWaterManager> cmpWaterManager(*m_Simulation, SYSTEM_ENTITY);

	for (ssize_t k = 0; k < vsize; k++)
	{
		ssize_t gx = m_Patch->m_X * PATCH_SIZE;
		ssize_t gz = m_Patch->m_Z * PATCH_SIZE;
		switch (side)
		{
		case CPATCH_SIDE_NEGX: gz += k; break;
		case CPATCH_SIDE_POSX: gx += PATCH_SIZE; gz += PATCH_SIZE-k; break;
		case CPATCH_SIDE_NEGZ: gx += PATCH_SIZE-k; break;
		case CPATCH_SIDE_POSZ: gz += PATCH_SIZE; gx += k; break;
		}

		CVector3D pos;
		terrain->CalcPosition(gx, gz, pos);

		// Clamp the height to the water level
		float waterHeight = 0.f;
		if (cmpWaterManager)
			waterHeight = cmpWaterManager->GetExactWaterLevel(pos.X, pos.Z);
		pos.Y = std::max(pos.Y, waterHeight);

		SSideVertex v0, v1;
		v0.m_Position = pos;
		v1.m_Position = pos;
		v1.m_Position.Y = 0;

		// If this is the start of this tristrip, but we've already got a partial
		// tristrip, add a couple of degenerate triangles to join the strips properly
		if (k == 0 && !vertices.empty())
		{
			vertices.push_back(vertices.back());
			vertices.push_back(v1);
		}

		// Now add the new triangles
		vertices.push_back(v1);
		vertices.push_back(v0);
	}
}

void CPatchRData::BuildSides()
{
	PROFILE3("build sides");

	std::vector<SSideVertex> sideVertices;

	int sideFlags = m_Patch->GetSideFlags();

	// If no sides are enabled, we don't need to do anything
	if (!sideFlags)
		return;

	// For each side, generate a tristrip by adding a vertex at ground/water
	// level and a vertex underneath at height 0.

	if (sideFlags & CPATCH_SIDE_NEGX)
		BuildSide(sideVertices, CPATCH_SIDE_NEGX);

	if (sideFlags & CPATCH_SIDE_POSX)
		BuildSide(sideVertices, CPATCH_SIDE_POSX);

	if (sideFlags & CPATCH_SIDE_NEGZ)
		BuildSide(sideVertices, CPATCH_SIDE_NEGZ);

	if (sideFlags & CPATCH_SIDE_POSZ)
		BuildSide(sideVertices, CPATCH_SIDE_POSZ);

	if (sideVertices.empty())
		return;

	if (!m_VBSides)
		m_VBSides = g_VBMan.Allocate(sizeof(SSideVertex), sideVertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
	m_VBSides->m_Owner->UpdateChunkVertices(m_VBSides, &sideVertices[0]);
}

void CPatchRData::Build()
{
	BuildVertices();
	BuildSides();
	BuildIndices();
	BuildBlends();
	BuildWater();
}

void CPatchRData::Update(CSimulation2* simulation)
{
	m_Simulation = simulation;
	if (m_UpdateFlags!=0) {
		// TODO,RC 11/04/04 - need to only rebuild necessary bits of renderdata rather
		// than everything; it's complicated slightly because the blends are dependent
		// on both vertex and index data
		BuildVertices();
		BuildSides();
		BuildIndices();
		BuildBlends();
		BuildWater();

		m_UpdateFlags=0;
	}
}

// Types used for glMultiDrawElements batching:

// To minimise the cost of memory allocations, everything used for computing
// batches uses a arena allocator. (All allocations are short-lived so we can
// just throw away the whole arena at the end of each frame.)

// std::map types with appropriate arena allocators and default comparison operator
#define POOLED_BATCH_MAP(Key, Value) \
	std::map<Key, Value, std::less<Key>, ProxyAllocator<std::pair<Key const, Value>, Allocators::DynamicArena > >

// Equivalent to "m[k]", when it returns a arena-allocated std::map (since we can't
// use the default constructor in that case)
template<typename M>
typename M::mapped_type& PooledMapGet(M& m, const typename M::key_type& k, Allocators::DynamicArena& arena)
{
	return m.insert(std::make_pair(k,
		typename M::mapped_type(typename M::mapped_type::key_compare(), typename M::mapped_type::allocator_type(arena))
	)).first->second;
}

// Equivalent to "m[k]", when it returns a std::pair of arena-allocated std::vectors
template<typename M>
typename M::mapped_type& PooledPairGet(M& m, const typename M::key_type& k, Allocators::DynamicArena& arena)
{
	return m.insert(std::make_pair(k, std::make_pair(
			typename M::mapped_type::first_type(typename M::mapped_type::first_type::allocator_type(arena)),
			typename M::mapped_type::second_type(typename M::mapped_type::second_type::allocator_type(arena))
	))).first->second;
}

// Each multidraw batch has a list of index counts, and a list of pointers-to-first-indexes
typedef std::pair<std::vector<GLint, ProxyAllocator<GLint, Allocators::DynamicArena > >, std::vector<void*, ProxyAllocator<void*, Allocators::DynamicArena > > > BatchElements;

// Group batches by index buffer
typedef POOLED_BATCH_MAP(CVertexBuffer*, BatchElements) IndexBufferBatches;

// Group batches by vertex buffer
typedef POOLED_BATCH_MAP(CVertexBuffer*, IndexBufferBatches) VertexBufferBatches;

// Group batches by texture
typedef POOLED_BATCH_MAP(CTerrainTextureEntry*, VertexBufferBatches) TextureBatches;

void CPatchRData::RenderBases(const std::vector<CPatchRData*>& patches, const CShaderDefines& context, 
			      ShadowMap* shadow, bool isDummyShader, const CShaderProgramPtr& dummy)
{
	Allocators::DynamicArena arena(1 * MiB);

	TextureBatches batches (TextureBatches::key_compare(), (TextureBatches::allocator_type(arena)));

 	PROFILE_START("compute batches");

 	// Collect all the patches' base splats into their appropriate batches
 	for (size_t i = 0; i < patches.size(); ++i)
 	{
 		CPatchRData* patch = patches[i];
 		for (size_t j = 0; j < patch->m_Splats.size(); ++j)
 		{
 			SSplat& splat = patch->m_Splats[j];

 			BatchElements& batch = PooledPairGet(
				PooledMapGet(
 					PooledMapGet(batches, splat.m_Texture, arena),
 					patch->m_VBBase->m_Owner, arena
				),
				patch->m_VBBaseIndices->m_Owner, arena
			);

 			batch.first.push_back(splat.m_IndexCount);

 			u8* indexBase = patch->m_VBBaseIndices->m_Owner->GetBindAddress();
 			batch.second.push_back(indexBase + sizeof(u16)*(patch->m_VBBaseIndices->m_Index + splat.m_IndexStart));
		}
 	}

 	PROFILE_END("compute batches");

 	// Render each batch
 	for (TextureBatches::iterator itt = batches.begin(); itt != batches.end(); ++itt)
	{
		int numPasses = 1;
		
		CShaderTechniquePtr techBase;
		
		if (!isDummyShader)
		{
			if (itt->first->GetMaterial().GetShaderEffect().length() == 0)
			{
				LOGERROR(L"Terrain renderer failed to load shader effect.\n");
				continue;
			}
						
			techBase = g_Renderer.GetShaderManager().LoadEffect(itt->first->GetMaterial().GetShaderEffect(),
						context, itt->first->GetMaterial().GetShaderDefines(0));
			
			numPasses = techBase->GetNumPasses();
		}
		
		for (int pass = 0; pass < numPasses; ++pass)
		{
			if (!isDummyShader)
			{
				techBase->BeginPass(pass);
				TerrainRenderer::PrepareShader(techBase->GetShader(), shadow);
			}
			
			const CShaderProgramPtr& shader = isDummyShader ? dummy : techBase->GetShader(pass);
			
			if (itt->first->GetMaterial().GetSamplers().size() != 0)
			{
				const CMaterial::SamplersVector& samplers = itt->first->GetMaterial().GetSamplers();
				size_t samplersNum = samplers.size();
				
				for (size_t s = 0; s < samplersNum; ++s)
				{
					const CMaterial::TextureSampler& samp = samplers[s];
					shader->BindTexture(samp.Name, samp.Sampler);
				}
				
				itt->first->GetMaterial().GetStaticUniforms().BindUniforms(shader);

#if !CONFIG2_GLES
				if (isDummyShader)
				{
					glMatrixMode(GL_TEXTURE);
					glLoadMatrixf(itt->first->GetTextureMatrix());
					glMatrixMode(GL_MODELVIEW);
				}
				else
#endif
				{
					float c = itt->first->GetTextureMatrix()[0];
					float ms = itt->first->GetTextureMatrix()[8];
					shader->Uniform(str_textureTransform, c, ms, -ms, 0.f);
				}
			}
			else
			{
				shader->BindTexture(str_baseTex, g_Renderer.GetTextureManager().GetErrorTexture());
			}

			for (VertexBufferBatches::iterator itv = itt->second.begin(); itv != itt->second.end(); ++itv)
			{
				GLsizei stride = sizeof(SBaseVertex);
				SBaseVertex *base = (SBaseVertex *)itv->first->Bind();
				shader->VertexPointer(3, GL_FLOAT, stride, &base->m_Position[0]);
				shader->ColorPointer(4, GL_UNSIGNED_BYTE, stride, &base->m_DiffuseColor);
				shader->NormalPointer(GL_FLOAT, stride, &base->m_Normal[0]);
				shader->TexCoordPointer(GL_TEXTURE0, 3, GL_FLOAT, stride, &base->m_Position[0]);

				shader->AssertPointersBound();

				for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
				{
					it->first->Bind();

					BatchElements& batch = it->second;

					if (!g_Renderer.m_SkipSubmit)
					{
						// Don't use glMultiDrawElements here since it doesn't have a significant
						// performance impact and it suffers from various driver bugs (e.g. it breaks
						// in Mesa 7.10 swrast with index VBOs)
						for (size_t i = 0; i < batch.first.size(); ++i)
							glDrawElements(GL_TRIANGLES, batch.first[i], GL_UNSIGNED_SHORT, batch.second[i]);
					}

					g_Renderer.m_Stats.m_DrawCalls++;
					g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 3;
				}
			}
			
			if (!isDummyShader)
				techBase->EndPass();
		}
	}

#if !CONFIG2_GLES
	if (isDummyShader)
	{
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
#endif

	CVertexBuffer::Unbind();
}

/**
 * Helper structure for RenderBlends.
 */
struct SBlendBatch
{
	SBlendBatch(Allocators::DynamicArena& arena) :
		m_Batches(VertexBufferBatches::key_compare(), VertexBufferBatches::allocator_type(arena))
	{
	}

	CTerrainTextureEntry* m_Texture;
 	VertexBufferBatches m_Batches;
};

/**
 * Helper structure for RenderBlends.
 */
struct SBlendStackItem
{
	SBlendStackItem(CVertexBuffer::VBChunk* v, CVertexBuffer::VBChunk* i,
			const std::vector<CPatchRData::SSplat>& s, Allocators::DynamicArena& arena) :
		vertices(v), indices(i), splats(s.begin(), s.end(), SplatStack::allocator_type(arena))
	{
	}

	typedef std::vector<CPatchRData::SSplat, ProxyAllocator<CPatchRData::SSplat, Allocators::DynamicArena > > SplatStack;
	CVertexBuffer::VBChunk* vertices;
	CVertexBuffer::VBChunk* indices;
	SplatStack splats;
};

void CPatchRData::RenderBlends(const std::vector<CPatchRData*>& patches, const CShaderDefines& context, 
			      ShadowMap* shadow, bool isDummyShader, const CShaderProgramPtr& dummy)
{
	Allocators::DynamicArena arena(1 * MiB);

	typedef std::vector<SBlendBatch, ProxyAllocator<SBlendBatch, Allocators::DynamicArena > > BatchesStack;
	BatchesStack batches((BatchesStack::allocator_type(arena)));
	
	CShaderDefines contextBlend = context;
	contextBlend.Add(str_BLEND, str_1);

 	PROFILE_START("compute batches");

 	// Reserve an arbitrary size that's probably big enough in most cases,
 	// to avoid heavy reallocations
 	batches.reserve(256);

	typedef std::vector<SBlendStackItem, ProxyAllocator<SBlendStackItem, Allocators::DynamicArena > > BlendStacks;
	BlendStacks blendStacks((BlendStacks::allocator_type(arena)));
	blendStacks.reserve(patches.size());

	// Extract all the blend splats from each patch
 	for (size_t i = 0; i < patches.size(); ++i)
 	{
 		CPatchRData* patch = patches[i];
 		if (!patch->m_BlendSplats.empty())
 		{

 			blendStacks.push_back(SBlendStackItem(patch->m_VBBlends, patch->m_VBBlendIndices, patch->m_BlendSplats, arena));
 			// Reverse the splats so the first to be rendered is at the back of the list
 			std::reverse(blendStacks.back().splats.begin(), blendStacks.back().splats.end());
 		}
 	}

 	// Rearrange the collection of splats to be grouped by texture, preserving
 	// order of splats within each patch:
 	// (This is exactly the same algorithm used in CPatchRData::BuildBlends,
 	// but applied to patch-sized splats rather than to tile-sized splats;
 	// see that function for comments on the algorithm.)
	while (true)
	{
		if (!batches.empty())
		{
			CTerrainTextureEntry* tex = batches.back().m_Texture;

			for (size_t k = 0; k < blendStacks.size(); ++k)
			{
				SBlendStackItem::SplatStack& splats = blendStacks[k].splats;
				if (!splats.empty() && splats.back().m_Texture == tex)
				{
					CVertexBuffer::VBChunk* vertices = blendStacks[k].vertices;
					CVertexBuffer::VBChunk* indices = blendStacks[k].indices;

					BatchElements& batch = PooledPairGet(PooledMapGet(batches.back().m_Batches, vertices->m_Owner, arena), indices->m_Owner, arena);
					batch.first.push_back(splats.back().m_IndexCount);

		 			u8* indexBase = indices->m_Owner->GetBindAddress();
		 			batch.second.push_back(indexBase + sizeof(u16)*(indices->m_Index + splats.back().m_IndexStart));

					splats.pop_back();
				}
			}
		}

		CTerrainTextureEntry* bestTex = NULL;
		size_t bestStackSize = 0;

		for (size_t k = 0; k < blendStacks.size(); ++k)
		{
			SBlendStackItem::SplatStack& splats = blendStacks[k].splats;
			if (splats.size() > bestStackSize)
			{
				bestStackSize = splats.size();
				bestTex = splats.back().m_Texture;
			}
		}

		if (bestStackSize == 0)
			break;

		SBlendBatch layer(arena);
		layer.m_Texture = bestTex;
		batches.push_back(layer);
	}

 	PROFILE_END("compute batches");

 	CVertexBuffer* lastVB = NULL;

 	for (BatchesStack::iterator itt = batches.begin(); itt != batches.end(); ++itt)
	{		
		if (itt->m_Texture->GetMaterial().GetSamplers().size() == 0)
			continue;
		
		int numPasses = 1;
		CShaderTechniquePtr techBase;
		
		if (!isDummyShader)
		{
			techBase = g_Renderer.GetShaderManager().LoadEffect(itt->m_Texture->GetMaterial().GetShaderEffect(), contextBlend, itt->m_Texture->GetMaterial().GetShaderDefines(0));
			
			numPasses = techBase->GetNumPasses();
		}
		
		CShaderProgramPtr previousShader;
		for (int pass = 0; pass < numPasses; ++pass)
		{
			if (!isDummyShader)
			{
				techBase->BeginPass(pass);
				TerrainRenderer::PrepareShader(techBase->GetShader(), shadow);
				
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
				
			const CShaderProgramPtr& shader = isDummyShader ? dummy : techBase->GetShader(pass);
				
			if (itt->m_Texture)
			{
				const CMaterial::SamplersVector& samplers = itt->m_Texture->GetMaterial().GetSamplers();
				size_t samplersNum = samplers.size();
				
				for (size_t s = 0; s < samplersNum; ++s)
				{
					const CMaterial::TextureSampler& samp = samplers[s];
					shader->BindTexture(samp.Name, samp.Sampler);
				}

				shader->BindTexture(str_blendTex, itt->m_Texture->m_TerrainAlpha->second.m_hCompositeAlphaMap);

				itt->m_Texture->GetMaterial().GetStaticUniforms().BindUniforms(shader);
				
#if !CONFIG2_GLES
				if (isDummyShader)
				{
					pglClientActiveTextureARB(GL_TEXTURE0);
					glMatrixMode(GL_TEXTURE);
					glLoadMatrixf(itt->m_Texture->GetTextureMatrix());
					glMatrixMode(GL_MODELVIEW);
				}
				else
#endif
				{
					float c = itt->m_Texture->GetTextureMatrix()[0];
					float ms = itt->m_Texture->GetTextureMatrix()[8];
					shader->Uniform(str_textureTransform, c, ms, -ms, 0.f);
				}
			}
			else
			{
				shader->BindTexture(str_baseTex, g_Renderer.GetTextureManager().GetErrorTexture());
			}

			for (VertexBufferBatches::iterator itv = itt->m_Batches.begin(); itv != itt->m_Batches.end(); ++itv)
			{
				// Rebind the VB only if it changed since the last batch
				if (itv->first != lastVB || shader != previousShader)
				{
					lastVB = itv->first;
					previousShader = shader;
					GLsizei stride = sizeof(SBlendVertex);
					SBlendVertex *base = (SBlendVertex *)itv->first->Bind();

					shader->VertexPointer(3, GL_FLOAT, stride, &base->m_Position[0]);
					shader->ColorPointer(4, GL_UNSIGNED_BYTE, stride, &base->m_DiffuseColor);
					shader->NormalPointer(GL_FLOAT, stride, &base->m_Normal[0]);
					shader->TexCoordPointer(GL_TEXTURE0, 3, GL_FLOAT, stride, &base->m_Position[0]);
					shader->TexCoordPointer(GL_TEXTURE1, 2, GL_FLOAT, stride, &base->m_AlphaUVs[0]);
				}

				shader->AssertPointersBound();

				for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
				{
					it->first->Bind();

					BatchElements& batch = it->second;

					if (!g_Renderer.m_SkipSubmit)
					{
						for (size_t i = 0; i < batch.first.size(); ++i)
							glDrawElements(GL_TRIANGLES, batch.first[i], GL_UNSIGNED_SHORT, batch.second[i]);
					}

					g_Renderer.m_Stats.m_DrawCalls++;
					g_Renderer.m_Stats.m_BlendSplats++;
					g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 3;
				}
			}
			
			if (!isDummyShader)
			{
				glDisable(GL_BLEND);
				techBase->EndPass();
			}
		}
	}

#if !CONFIG2_GLES
	if (isDummyShader)
	{
		pglClientActiveTextureARB(GL_TEXTURE0);
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}
#endif

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderStreams(const std::vector<CPatchRData*>& patches, const CShaderProgramPtr& shader, int streamflags)
{
	// Each batch has a list of index counts, and a list of pointers-to-first-indexes
	typedef std::pair<std::vector<GLint>, std::vector<void*> > BatchElements;

	// Group batches by index buffer
	typedef std::map<CVertexBuffer*, BatchElements> IndexBufferBatches;

	// Group batches by vertex buffer
	typedef std::map<CVertexBuffer*, IndexBufferBatches> VertexBufferBatches;

 	VertexBufferBatches batches;

 	PROFILE_START("compute batches");

 	// Collect all the patches into their appropriate batches
 	for (size_t i = 0; i < patches.size(); ++i)
 	{
 		CPatchRData* patch = patches[i];
		BatchElements& batch = batches[patch->m_VBBase->m_Owner][patch->m_VBBaseIndices->m_Owner];

		batch.first.push_back(patch->m_VBBaseIndices->m_Count);

		u8* indexBase = patch->m_VBBaseIndices->m_Owner->GetBindAddress();
 		batch.second.push_back(indexBase + sizeof(u16)*(patch->m_VBBaseIndices->m_Index));
 	}

 	PROFILE_END("compute batches");

	ENSURE(!(streamflags & ~(STREAM_POS|STREAM_COLOR|STREAM_POSTOUV0|STREAM_POSTOUV1)));

 	// Render each batch
 	for (VertexBufferBatches::iterator itv = batches.begin(); itv != batches.end(); ++itv)
	{
		GLsizei stride = sizeof(SBaseVertex);
		SBaseVertex *base = (SBaseVertex *)itv->first->Bind();

		shader->VertexPointer(3, GL_FLOAT, stride, &base->m_Position);
		if (streamflags & STREAM_POSTOUV0)
			shader->TexCoordPointer(GL_TEXTURE0, 3, GL_FLOAT, stride, &base->m_Position);
		if (streamflags & STREAM_POSTOUV1)
			shader->TexCoordPointer(GL_TEXTURE1, 3, GL_FLOAT, stride, &base->m_Position);
		if (streamflags & STREAM_COLOR)
			shader->ColorPointer(4, GL_UNSIGNED_BYTE, stride, &base->m_DiffuseColor);

		shader->AssertPointersBound();

		for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
		{
			it->first->Bind();

			BatchElements& batch = it->second;

			if (!g_Renderer.m_SkipSubmit)
			{
				for (size_t i = 0; i < batch.first.size(); ++i)
					glDrawElements(GL_TRIANGLES, batch.first[i], GL_UNSIGNED_SHORT, batch.second[i]);
			}

			g_Renderer.m_Stats.m_DrawCalls++;
			g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 3;
		}
	}

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderOutline()
{
	CTerrain* terrain = m_Patch->m_Parent;
	ssize_t gx = m_Patch->m_X * PATCH_SIZE;
	ssize_t gz = m_Patch->m_Z * PATCH_SIZE;

	CVector3D pos;
	std::vector<CVector3D> line;

	ssize_t i, j;

	for (i = 0, j = 0; i <= PATCH_SIZE; ++i)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}
	for (i = PATCH_SIZE, j = 1; j <= PATCH_SIZE; ++j)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}
	for (i = PATCH_SIZE-1, j = PATCH_SIZE; i >= 0; --i)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}
	for (i = 0, j = PATCH_SIZE-1; j >= 0; --j)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}

#if CONFIG2_GLES
#warning TODO: implement CPatchRData::RenderOutlines for GLES
#else
	glVertexPointer(3, GL_FLOAT, sizeof(CVector3D), &line[0]);
	glDrawArrays(GL_LINE_STRIP, 0, line.size());
#endif
}

void CPatchRData::RenderSides(CShaderProgramPtr& shader)
{
	ENSURE(m_UpdateFlags==0);

	if (!m_VBSides)
		return;

	SSideVertex *base = (SSideVertex *)m_VBSides->m_Owner->Bind();

	// setup data pointers
	GLsizei stride = sizeof(SSideVertex);
	shader->VertexPointer(3, GL_FLOAT, stride, &base->m_Position);

	shader->AssertPointersBound();

	if (!g_Renderer.m_SkipSubmit)
		glDrawArrays(GL_TRIANGLE_STRIP, m_VBSides->m_Index, (GLsizei)m_VBSides->m_Count);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris += m_VBSides->m_Count - 2;

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderPriorities(CTextRenderer& textRenderer)
{
	CTerrain* terrain = m_Patch->m_Parent;
	CCamera* camera = g_Game->GetView()->GetCamera();

	for (ssize_t j = 0; j < PATCH_SIZE; ++j)
	{
		for (ssize_t i = 0; i < PATCH_SIZE; ++i)
		{
			ssize_t gx = m_Patch->m_X * PATCH_SIZE + i;
			ssize_t gz = m_Patch->m_Z * PATCH_SIZE + j;

			CVector3D pos;
			terrain->CalcPosition(gx, gz, pos);

			// Move a bit towards the center of the tile
			pos.X += TERRAIN_TILE_SIZE/4.f;
			pos.Z += TERRAIN_TILE_SIZE/4.f;

			float x, y;
			camera->GetScreenCoordinates(pos, x, y);

			textRenderer.PrintfAt(x, y, L"%d", m_Patch->m_MiniPatches[j][i].Priority);
		}
	}
}

//
// Water build and rendering
//

// Build vertex buffer for water vertices over our patch
void CPatchRData::BuildWater()
{
	PROFILE3("build water");

	// number of vertices in each direction in each patch
	ENSURE((PATCH_SIZE % water_cell_size) == 0);

	if (m_VBWater)
	{
		g_VBMan.Release(m_VBWater);
		m_VBWater = 0;
	}
	if (m_VBWaterIndices)
	{
		g_VBMan.Release(m_VBWaterIndices);
		m_VBWaterIndices = 0;
	}
	if (m_VBWaterShore)
	{
		g_VBMan.Release(m_VBWaterShore);
		m_VBWaterShore = 0;
	}
	if (m_VBWaterIndicesShore)
	{
		g_VBMan.Release(m_VBWaterIndicesShore);
		m_VBWaterIndicesShore = 0;
	}
	m_WaterBounds.SetEmpty();

	// We need to use this to access the water manager or we may not have the
	// actual values but some compiled-in defaults
	CmpPtr<ICmpWaterManager> cmpWaterManager(*m_Simulation, SYSTEM_ENTITY);
	if (!cmpWaterManager)
		return;
	
	// Build data for water
	std::vector<SWaterVertex> water_vertex_data;
	std::vector<GLushort> water_indices;
	u16 water_index_map[PATCH_SIZE+1][PATCH_SIZE+1];
	memset(water_index_map, 0xFF, sizeof(water_index_map));

	// Build data for shore
	std::vector<SWaterVertex> water_vertex_data_shore;
	std::vector<GLushort> water_indices_shore;
	u16 water_shore_index_map[PATCH_SIZE+1][PATCH_SIZE+1];
	memset(water_shore_index_map, 0xFF, sizeof(water_shore_index_map));
	
	WaterManager* WaterMgr = g_Renderer.GetWaterManager();

	CPatch* patch = m_Patch;
	CTerrain* terrain = patch->m_Parent;

	ssize_t mapSize = (size_t)terrain->GetVerticesPerSide();
	
	//Top-left coordinates of our patch.
	ssize_t x1 = m_Patch->m_X*PATCH_SIZE;
	ssize_t z1 = m_Patch->m_Z*PATCH_SIZE;

	// to whoever implements different water heights, this is a TODO: water height)
	float waterHeight = cmpWaterManager->GetExactWaterLevel(0.0f,0.0f);

	// The 4 points making a water tile.
	int moves[4][2] = { {0,0}, {water_cell_size,0}, {0,water_cell_size}, {water_cell_size,water_cell_size} };
	// Where to look for when checking for water for shore tiles.
	int check[10][2] = { {0,0},{water_cell_size,0},{water_cell_size*2,0},{0,water_cell_size},{0,water_cell_size*2},{water_cell_size,water_cell_size},{water_cell_size*2,water_cell_size*2}, {-water_cell_size,0}, {0,-water_cell_size}, {-water_cell_size,-water_cell_size} };
	
	// build vertices, uv, and shader varying
	for (ssize_t z = 0; z < PATCH_SIZE; z += water_cell_size)
	{
		for (ssize_t x = 0; x < PATCH_SIZE; x += water_cell_size)
		{
			
			// Check that this tile is close to water
			bool nearWat = false;
			for (size_t test = 0; test < 10; ++test)
				if (terrain->GetVertexGroundLevel(x+x1+check[test][0], z+z1+check[test][1]) < waterHeight)
					nearWat = true;
			if (!nearWat)
				continue;
			
			// This is actually lying and I should call CcmpTerrain
			/*if (!terrain->IsOnMap(x+x1, z+z1)
			 && !terrain->IsOnMap(x+x1, z+z1 + water_cell_size)
			 && !terrain->IsOnMap(x+x1 + water_cell_size, z+z1)
			 && !terrain->IsOnMap(x+x1 + water_cell_size, z+z1 + water_cell_size))
			 continue;*/
						
			for (int i = 0; i < 4; ++i)
			{
				if (water_index_map[z+moves[i][1]][x+moves[i][0]] != 0xFFFF)
					continue;
				
				ssize_t zz = z+z1+moves[i][1];
				ssize_t xx = x+x1+moves[i][0];
				
				SWaterVertex vertex;
				terrain->CalcPosition(xx,zz, vertex.m_Position);
				float depth = waterHeight - vertex.m_Position.Y;
				
				vertex.m_Position.Y = waterHeight;
				
				m_WaterBounds += vertex.m_Position;
				
				vertex.m_WaterData = CVector2D(WaterMgr->m_WindStrength[xx + zz*mapSize], depth);
				
				water_index_map[z+moves[i][1]][x+moves[i][0]] = water_vertex_data.size();
				water_vertex_data.push_back(vertex);
			}
			water_indices.push_back(water_index_map[z + moves[2][1]][x + moves[2][0]]);
			water_indices.push_back(water_index_map[z + moves[0][1]][x + moves[0][0]]);
			water_indices.push_back(water_index_map[z + moves[1][1]][x + moves[1][0]]);
			water_indices.push_back(water_index_map[z + moves[1][1]][x + moves[1][0]]);
			water_indices.push_back(water_index_map[z + moves[3][1]][x + moves[3][0]]);
			water_indices.push_back(water_index_map[z + moves[2][1]][x + moves[2][0]]);
			
			// Check id this tile is partly over land.
			// If so add a square over the terrain. This is necessary to render waves that go on shore.
			if (terrain->GetVertexGroundLevel(x+x1, z+z1) < waterHeight
				&& terrain->GetVertexGroundLevel(x+x1 + water_cell_size, z+z1) < waterHeight
				&& terrain->GetVertexGroundLevel(x+x1, z+z1+water_cell_size) < waterHeight
				&& terrain->GetVertexGroundLevel(x+x1 + water_cell_size, z+z1+water_cell_size) < waterHeight)
				continue;
			
			for (int i = 0; i < 4; ++i)
			{
				if (water_shore_index_map[z+moves[i][1]][x+moves[i][0]] != 0xFFFF)
					continue;
				ssize_t zz = z+z1+moves[i][1];
				ssize_t xx = x+x1+moves[i][0];
				
				SWaterVertex vertex;
				terrain->CalcPosition(xx,zz, vertex.m_Position);
				
				vertex.m_Position.Y += 0.02f;
				m_WaterBounds += vertex.m_Position;
				
				vertex.m_WaterData = CVector2D(0.0f, -5.0f);
				
				water_shore_index_map[z+moves[i][1]][x+moves[i][0]] = water_vertex_data_shore.size();
				water_vertex_data_shore.push_back(vertex);
			}
			if (terrain->GetTriangulationDir(x+x1,z+z1))
			{
				water_indices_shore.push_back(water_shore_index_map[z + moves[2][1]][x + moves[2][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[0][1]][x + moves[0][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[1][1]][x + moves[1][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[1][1]][x + moves[1][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[3][1]][x + moves[3][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[2][1]][x + moves[2][0]]);
			}
			else
			{
				water_indices_shore.push_back(water_shore_index_map[z + moves[3][1]][x + moves[3][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[2][1]][x + moves[2][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[0][1]][x + moves[0][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[3][1]][x + moves[3][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[0][1]][x + moves[0][0]]);
				water_indices_shore.push_back(water_shore_index_map[z + moves[1][1]][x + moves[1][0]]);
			}
		}
	}

	// no vertex buffers if no data generated
	if (water_indices.size() != 0)
	{
		m_VBWater = g_VBMan.Allocate(sizeof(SWaterVertex), water_vertex_data.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
		m_VBWater->m_Owner->UpdateChunkVertices(m_VBWater, &water_vertex_data[0]);
		
		m_VBWaterIndices = g_VBMan.Allocate(sizeof(GLushort), water_indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
		m_VBWaterIndices->m_Owner->UpdateChunkVertices(m_VBWaterIndices, &water_indices[0]);
	}

	if (water_indices_shore.size() != 0)
	{
		m_VBWaterShore = g_VBMan.Allocate(sizeof(SWaterVertex), water_vertex_data_shore.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
		m_VBWaterShore->m_Owner->UpdateChunkVertices(m_VBWaterShore, &water_vertex_data_shore[0]);
		
		// Construct indices buffer
		m_VBWaterIndicesShore = g_VBMan.Allocate(sizeof(GLushort), water_indices_shore.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
		m_VBWaterIndicesShore->m_Owner->UpdateChunkVertices(m_VBWaterIndicesShore, &water_indices_shore[0]);
	}}

void CPatchRData::RenderWater(CShaderProgramPtr& shader, bool onlyShore, bool fixedPipeline)
{
	ASSERT(m_UpdateFlags==0);

	if (g_Renderer.m_SkipSubmit || (!m_VBWater && !m_VBWaterShore))
		return;

	if (m_VBWater != 0x0 && !onlyShore)
	{
		SWaterVertex *base=(SWaterVertex *)m_VBWater->m_Owner->Bind();
		
		// setup data pointers
		GLsizei stride = sizeof(SWaterVertex);
		shader->VertexPointer(3, GL_FLOAT, stride, &base[m_VBWater->m_Index].m_Position);
		if (!fixedPipeline)
			shader->VertexAttribPointer(str_a_waterInfo, 2, GL_FLOAT, false, stride, &base[m_VBWater->m_Index].m_WaterData);
		
		shader->AssertPointersBound();
		
		u8* indexBase = m_VBWaterIndices->m_Owner->Bind();
		glDrawElements(GL_TRIANGLES, (GLsizei) m_VBWaterIndices->m_Count,
					   GL_UNSIGNED_SHORT, indexBase + sizeof(u16)*(m_VBWaterIndices->m_Index));
		
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_WaterTris += m_VBWaterIndices->m_Count / 3;
	}
	if (m_VBWaterShore != 0x0 && g_Renderer.GetWaterManager()->m_WaterFancyEffects && !g_Renderer.GetWaterManager()->m_WaterUgly)
	{
		SWaterVertex *base=(SWaterVertex *)m_VBWaterShore->m_Owner->Bind();
		
		GLsizei stride = sizeof(SWaterVertex);
		shader->VertexPointer(3, GL_FLOAT, stride, &base[m_VBWaterShore->m_Index].m_Position);
		if (!fixedPipeline)
			shader->VertexAttribPointer(str_a_waterInfo, 2, GL_FLOAT, false, stride, &base[m_VBWaterShore->m_Index].m_WaterData);
		
		shader->AssertPointersBound();
		
		u8* indexBase = m_VBWaterIndicesShore->m_Owner->Bind();
		glDrawElements(GL_TRIANGLES, (GLsizei) m_VBWaterIndicesShore->m_Count,
					   GL_UNSIGNED_SHORT, indexBase + sizeof(u16)*(m_VBWaterIndicesShore->m_Index));
		
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_WaterTris += m_VBWaterIndicesShore->m_Count / 3;
	}

	CVertexBuffer::Unbind();
}
