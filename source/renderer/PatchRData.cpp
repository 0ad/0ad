/* Copyright (C) 2022 Wildfire Games.
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

#include "renderer/PatchRData.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/LOSTexture.h"
#include "graphics/Patch.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TextRenderer.h"
#include "graphics/TextureManager.h"
#include "lib/allocators/DynamicArena.h"
#include "lib/allocators/STLAllocators.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/GameSetup/Config.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "renderer/AlphaMapCalculator.h"
#include "renderer/DebugRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/TerrainRenderer.h"
#include "renderer/WaterManager.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/Simulation2.h"

#include <algorithm>
#include <numeric>
#include <set>

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

CPatchRData::CPatchRData(CPatch* patch, CSimulation2* simulation) :
	m_Patch(patch), m_Simulation(simulation)
{
	ENSURE(patch);
	Build();
}

CPatchRData::~CPatchRData() = default;

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

	std::vector<STileBlend> blends;
	blends.reserve(9);

	// For each tile in patch ..
	for (ssize_t j = 0; j < PATCH_SIZE; ++j)
	{
		for (ssize_t i = 0; i < PATCH_SIZE; ++i)
		{
			ssize_t gx = m_Patch->m_X * PATCH_SIZE + i;
			ssize_t gz = m_Patch->m_Z * PATCH_SIZE + j;

			blends.clear();

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
	m_VBBlends.Reset();
	m_VBBlendIndices.Reset();

	if (blendVertices.size())
	{
		// Construct vertex buffer

		m_VBBlends = g_VBMan.AllocateChunk(
			sizeof(SBlendVertex), blendVertices.size(),
			Renderer::Backend::IBuffer::Type::VERTEX, false,
			nullptr, CVertexBufferManager::Group::TERRAIN);
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends.Get(), &blendVertices[0]);

		// Update the indices to include the base offset of the vertex data
		for (size_t k = 0; k < blendIndices.size(); ++k)
			blendIndices[k] += static_cast<u16>(m_VBBlends->m_Index);

		m_VBBlendIndices = g_VBMan.AllocateChunk(
			sizeof(u16), blendIndices.size(),
			Renderer::Backend::IBuffer::Type::INDEX, false,
			nullptr, CVertexBufferManager::Group::TERRAIN);
		m_VBBlendIndices->m_Owner->UpdateChunkVertices(m_VBBlendIndices.Get(), &blendIndices[0]);
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

	CVector3D normal;

	u16 index = static_cast<u16>(blendVertices.size());

	terrain->CalcPosition(gx, gz, dst.m_Position);
	terrain->CalcNormal(gx, gz, normal);
	dst.m_Normal = normal;
	dst.m_AlphaUVs[0] = vtx[0].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[0].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	terrain->CalcPosition(gx + 1, gz, dst.m_Position);
	terrain->CalcNormal(gx + 1, gz, normal);
	dst.m_Normal = normal;
	dst.m_AlphaUVs[0] = vtx[1].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[1].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	terrain->CalcPosition(gx + 1, gz + 1, dst.m_Position);
	terrain->CalcNormal(gx + 1, gz + 1, normal);
	dst.m_Normal = normal;
	dst.m_AlphaUVs[0] = vtx[2].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[2].m_AlphaUVs[1];
	blendVertices.push_back(dst);

	terrain->CalcPosition(gx, gz + 1, dst.m_Position);
	terrain->CalcNormal(gx, gz + 1, normal);
	dst.m_Normal = normal;
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

	// PATCH_SIZE must be 2^8-2 or less to not overflow u16 indices buffer. Thankfully this is always true.
	ENSURE(vsize*vsize < 65536);

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

	for (size_t k = 0; k < m_Splats.size(); ++k)
	{
		CTerrainTextureEntry* tex = textures[k];

		SSplat& splat=m_Splats[k];
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
	m_VBBaseIndices.Reset();

	ENSURE(indices.size());

	// Construct vertex buffer
	m_VBBaseIndices = g_VBMan.AllocateChunk(
		sizeof(u16), indices.size(),
		Renderer::Backend::IBuffer::Type::INDEX, false, nullptr, CVertexBufferManager::Group::TERRAIN);
	m_VBBaseIndices->m_Owner->UpdateChunkVertices(m_VBBaseIndices.Get(), &indices[0]);
}


void CPatchRData::BuildVertices()
{
	PROFILE3("build vertices");

	// create both vertices and lighting colors

	// number of vertices in each direction in each patch
	ssize_t vsize = PATCH_SIZE + 1;

	std::vector<SBaseVertex> vertices;
	vertices.resize(vsize * vsize);

	// get index of this patch
	ssize_t px = m_Patch->m_X;
	ssize_t pz = m_Patch->m_Z;

	CTerrain* terrain = m_Patch->m_Parent;

	// build vertices
	for (ssize_t j = 0; j < vsize; ++j)
	{
		for (ssize_t i = 0; i < vsize; ++i)
		{
			ssize_t ix = px * PATCH_SIZE + i;
			ssize_t iz = pz * PATCH_SIZE + j;
			ssize_t v = j * vsize + i;

			// calculate vertex data
			terrain->CalcPosition(ix, iz, vertices[v].m_Position);

			CVector3D normal;
			terrain->CalcNormal(ix, iz, normal);
			vertices[v].m_Normal = normal;
		}
	}

	// upload to vertex buffer
	if (!m_VBBase)
	{
		m_VBBase = g_VBMan.AllocateChunk(
			sizeof(SBaseVertex), vsize * vsize,
			Renderer::Backend::IBuffer::Type::VERTEX, false,
			nullptr, CVertexBufferManager::Group::TERRAIN);
	}

	m_VBBase->m_Owner->UpdateChunkVertices(m_VBBase.Get(), &vertices[0]);
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

		if (k == 0)
		{
			vertices.emplace_back(v1);
			vertices.emplace_back(v0);
		}
		if (k > 0)
		{
			const size_t lastIndex = vertices.size() - 1;
			vertices.emplace_back(v1);
			vertices.emplace_back(vertices[lastIndex]);
			vertices.emplace_back(v0);
			vertices.emplace_back(v1);
			if (k + 1 < vsize)
			{
				vertices.emplace_back(v1);
				vertices.emplace_back(v0);
			}
		}
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
	{
		m_VBSides = g_VBMan.AllocateChunk(
			sizeof(SSideVertex), sideVertices.size(),
			Renderer::Backend::IBuffer::Type::VERTEX, false,
			nullptr, CVertexBufferManager::Group::DEFAULT);
	}
	m_VBSides->m_Owner->UpdateChunkVertices(m_VBSides.Get(), &sideVertices[0]);
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

// To minimise the cost of memory allocations, everything used for computing
// batches uses a arena allocator. (All allocations are short-lived so we can
// just throw away the whole arena at the end of each frame.)

using Arena = Allocators::DynamicArena<1 * MiB>;

// std::map types with appropriate arena allocators and default comparison operator
template<class Key, class Value>
using PooledBatchMap = std::map<Key, Value, std::less<Key>, ProxyAllocator<std::pair<Key const, Value>, Arena>>;

// Equivalent to "m[k]", when it returns a arena-allocated std::map (since we can't
// use the default constructor in that case)
template<typename M>
typename M::mapped_type& PooledMapGet(M& m, const typename M::key_type& k, Arena& arena)
{
	return m.insert(std::make_pair(k,
		typename M::mapped_type(typename M::mapped_type::key_compare(), typename M::mapped_type::allocator_type(arena))
	)).first->second;
}

// Equivalent to "m[k]", when it returns a std::pair of arena-allocated std::vectors
template<typename M>
typename M::mapped_type& PooledPairGet(M& m, const typename M::key_type& k, Arena& arena)
{
	return m.insert(std::make_pair(k, std::make_pair(
			typename M::mapped_type::first_type(typename M::mapped_type::first_type::allocator_type(arena)),
			typename M::mapped_type::second_type(typename M::mapped_type::second_type::allocator_type(arena))
	))).first->second;
}

// Each multidraw batch has a list of index counts, and a list of pointers-to-first-indexes
using BatchElements = std::pair<std::vector<u32, ProxyAllocator<u32, Arena>>, std::vector<u32, ProxyAllocator<u32, Arena>>>;

// Group batches by index buffer
using IndexBufferBatches = PooledBatchMap<CVertexBuffer*, BatchElements>;

// Group batches by vertex buffer
using VertexBufferBatches = PooledBatchMap<CVertexBuffer*, IndexBufferBatches>;

// Group batches by texture
using TextureBatches = PooledBatchMap<CTerrainTextureEntry*, VertexBufferBatches>;

// Group batches by shaders.
using ShaderTechniqueBatches = PooledBatchMap<std::pair<CStrIntern, CShaderDefines>, TextureBatches>;

void CPatchRData::RenderBases(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const std::vector<CPatchRData*>& patches, const CShaderDefines& context, ShadowMap* shadow)
{
	PROFILE3("render terrain bases");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain bases");

	Arena arena;

	ShaderTechniqueBatches batches(ShaderTechniqueBatches::key_compare(), (ShaderTechniqueBatches::allocator_type(arena)));

	PROFILE_START("compute batches");

	// Collect all the patches' base splats into their appropriate batches
	for (size_t i = 0; i < patches.size(); ++i)
	{
		CPatchRData* patch = patches[i];
		for (size_t j = 0; j < patch->m_Splats.size(); ++j)
		{
			SSplat& splat = patch->m_Splats[j];
			const CMaterial& material = splat.m_Texture->GetMaterial();
			if (material.GetShaderEffect().empty())
			{
				LOGERROR("Terrain renderer failed to load shader effect.\n");
				continue;
			}

			BatchElements& batch = PooledPairGet(
				PooledMapGet(
					PooledMapGet(
						PooledMapGet(batches, std::make_pair(material.GetShaderEffect(), material.GetShaderDefines()), arena),
						splat.m_Texture, arena
					),
					patch->m_VBBase->m_Owner, arena
				),
				patch->m_VBBaseIndices->m_Owner, arena
			);

			batch.first.push_back(splat.m_IndexCount);

			batch.second.push_back(patch->m_VBBaseIndices->m_Index + splat.m_IndexStart);
		}
	}

	PROFILE_END("compute batches");

	// Render each batch
	for (ShaderTechniqueBatches::iterator itTech = batches.begin(); itTech != batches.end(); ++itTech)
	{
		CShaderDefines defines = context;
		defines.SetMany(itTech->first.second);
		CShaderTechniquePtr techBase = g_Renderer.GetShaderManager().LoadEffect(
			itTech->first.first, defines);

		const int numPasses = techBase->GetNumPasses();
		for (int pass = 0; pass < numPasses; ++pass)
		{
			deviceCommandContext->SetGraphicsPipelineState(
				techBase->GetGraphicsPipelineStateDesc(pass));
			deviceCommandContext->BeginPass();
			Renderer::Backend::IShaderProgram* shader = techBase->GetShader(pass);
			TerrainRenderer::PrepareShader(deviceCommandContext, shader, shadow);

			const int32_t baseTexBindingSlot =
				shader->GetBindingSlot(str_baseTex);
			const int32_t textureTransformBindingSlot =
				shader->GetBindingSlot(str_textureTransform);

			TextureBatches& textureBatches = itTech->second;
			for (TextureBatches::iterator itt = textureBatches.begin(); itt != textureBatches.end(); ++itt)
			{
				if (!itt->first->GetMaterial().GetSamplers().empty())
				{
					const CMaterial::SamplersVector& samplers =
						itt->first->GetMaterial().GetSamplers();
					for(const CMaterial::TextureSampler& samp : samplers)
						samp.Sampler->UploadBackendTextureIfNeeded(deviceCommandContext);
					for(const CMaterial::TextureSampler& samp : samplers)
					{
						deviceCommandContext->SetTexture(
							shader->GetBindingSlot(samp.Name),
							samp.Sampler->GetBackendTexture());
					}

					itt->first->GetMaterial().GetStaticUniforms().BindUniforms(
						deviceCommandContext, shader);

					float c = itt->first->GetTextureMatrix()[0];
					float ms = itt->first->GetTextureMatrix()[8];
					deviceCommandContext->SetUniform(
						textureTransformBindingSlot, c, ms);
				}
				else
				{
					deviceCommandContext->SetTexture(
						baseTexBindingSlot,
						g_Renderer.GetTextureManager().GetErrorTexture()->GetBackendTexture());
				}

				for (VertexBufferBatches::iterator itv = itt->second.begin(); itv != itt->second.end(); ++itv)
				{
					itv->first->UploadIfNeeded(deviceCommandContext);

					const uint32_t stride = sizeof(SBaseVertex);

					deviceCommandContext->SetVertexAttributeFormat(
						Renderer::Backend::VertexAttributeStream::POSITION,
						Renderer::Backend::Format::R32G32B32_SFLOAT,
						offsetof(SBaseVertex, m_Position), stride,
						Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
					deviceCommandContext->SetVertexAttributeFormat(
						Renderer::Backend::VertexAttributeStream::NORMAL,
						Renderer::Backend::Format::R32G32B32_SFLOAT,
						offsetof(SBaseVertex, m_Normal), stride,
						Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
					deviceCommandContext->SetVertexAttributeFormat(
						Renderer::Backend::VertexAttributeStream::UV0,
						Renderer::Backend::Format::R32G32B32_SFLOAT,
						offsetof(SBaseVertex, m_Position), stride,
						Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

					deviceCommandContext->SetVertexBuffer(0, itv->first->GetBuffer());

					for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
					{
						it->first->UploadIfNeeded(deviceCommandContext);
						deviceCommandContext->SetIndexBuffer(it->first->GetBuffer());

						BatchElements& batch = it->second;

						for (size_t i = 0; i < batch.first.size(); ++i)
							deviceCommandContext->DrawIndexed(batch.second[i], batch.first[i], 0);

						g_Renderer.m_Stats.m_DrawCalls++;
						g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 3;
					}
				}
			}
			deviceCommandContext->EndPass();
		}
	}
}

/**
 * Helper structure for RenderBlends.
 */
struct SBlendBatch
{
	SBlendBatch(Arena& arena) :
		m_Batches(VertexBufferBatches::key_compare(), VertexBufferBatches::allocator_type(arena))
	{
	}

	CTerrainTextureEntry* m_Texture;
	CShaderTechniquePtr m_ShaderTech;
	VertexBufferBatches m_Batches;
};

/**
 * Helper structure for RenderBlends.
 */
struct SBlendStackItem
{
	SBlendStackItem(CVertexBuffer::VBChunk* v, CVertexBuffer::VBChunk* i,
			const std::vector<CPatchRData::SSplat>& s, Arena& arena) :
		vertices(v), indices(i), splats(s.begin(), s.end(), SplatStack::allocator_type(arena))
	{
	}

	using SplatStack = std::vector<CPatchRData::SSplat, ProxyAllocator<CPatchRData::SSplat, Arena>>;
	CVertexBuffer::VBChunk* vertices;
	CVertexBuffer::VBChunk* indices;
	SplatStack splats;
};

void CPatchRData::RenderBlends(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const std::vector<CPatchRData*>& patches, const CShaderDefines& context, ShadowMap* shadow)
{
	PROFILE3("render terrain blends");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain blends");

	Arena arena;

	using BatchesStack = std::vector<SBlendBatch, ProxyAllocator<SBlendBatch, Arena>>;
	BatchesStack batches((BatchesStack::allocator_type(arena)));

	CShaderDefines contextBlend = context;
	contextBlend.Add(str_BLEND, str_1);

 	PROFILE_START("compute batches");

 	// Reserve an arbitrary size that's probably big enough in most cases,
 	// to avoid heavy reallocations
 	batches.reserve(256);

	using BlendStacks = std::vector<SBlendStackItem, ProxyAllocator<SBlendStackItem, Arena>>;
	BlendStacks blendStacks((BlendStacks::allocator_type(arena)));
	blendStacks.reserve(patches.size());

	// Extract all the blend splats from each patch
 	for (size_t i = 0; i < patches.size(); ++i)
 	{
 		CPatchRData* patch = patches[i];
 		if (!patch->m_BlendSplats.empty())
 		{

 			blendStacks.push_back(SBlendStackItem(patch->m_VBBlends.Get(), patch->m_VBBlendIndices.Get(), patch->m_BlendSplats, arena));
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

		 			batch.second.push_back(indices->m_Index + splats.back().m_IndexStart);

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
		if (!bestTex->GetMaterial().GetSamplers().empty())
		{
			CShaderDefines defines = contextBlend;
			defines.SetMany(bestTex->GetMaterial().GetShaderDefines());
			layer.m_ShaderTech = g_Renderer.GetShaderManager().LoadEffect(
				bestTex->GetMaterial().GetShaderEffect(), defines);
		}
		batches.push_back(layer);
	}

	PROFILE_END("compute batches");

	CVertexBuffer* lastVB = nullptr;
	Renderer::Backend::IShaderProgram* previousShader = nullptr;
	for (BatchesStack::iterator itTechBegin = batches.begin(), itTechEnd = batches.begin(); itTechBegin != batches.end(); itTechBegin = itTechEnd)
	{
		while (itTechEnd != batches.end() && itTechEnd->m_ShaderTech == itTechBegin->m_ShaderTech)
			++itTechEnd;

		const CShaderTechniquePtr& techBase = itTechBegin->m_ShaderTech;
		const int numPasses = techBase->GetNumPasses();
		for (int pass = 0; pass < numPasses; ++pass)
		{
			Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
				techBase->GetGraphicsPipelineStateDesc(pass);
			pipelineStateDesc.blendState.enabled = true;
			pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
				Renderer::Backend::BlendFactor::SRC_ALPHA;
			pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
				Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
			pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
				Renderer::Backend::BlendOp::ADD;
			deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
			deviceCommandContext->BeginPass();

			Renderer::Backend::IShaderProgram* shader = techBase->GetShader(pass);
			TerrainRenderer::PrepareShader(deviceCommandContext, shader, shadow);

			Renderer::Backend::ITexture* lastBlendTex = nullptr;

			const int32_t baseTexBindingSlot =
				shader->GetBindingSlot(str_baseTex);
			const int32_t blendTexBindingSlot =
				shader->GetBindingSlot(str_blendTex);
			const int32_t textureTransformBindingSlot =
				shader->GetBindingSlot(str_textureTransform);

			for (BatchesStack::iterator itt = itTechBegin; itt != itTechEnd; ++itt)
			{
				if (itt->m_Texture->GetMaterial().GetSamplers().empty())
					continue;

				if (itt->m_Texture)
				{
					const CMaterial::SamplersVector& samplers = itt->m_Texture->GetMaterial().GetSamplers();
					for (const CMaterial::TextureSampler& samp : samplers)
						samp.Sampler->UploadBackendTextureIfNeeded(deviceCommandContext);
					for (const CMaterial::TextureSampler& samp : samplers)
					{
						deviceCommandContext->SetTexture(
							shader->GetBindingSlot(samp.Name),
							samp.Sampler->GetBackendTexture());
					}

					Renderer::Backend::ITexture* currentBlendTex = itt->m_Texture->m_TerrainAlpha->second.m_CompositeAlphaMap.get();
					if (currentBlendTex != lastBlendTex)
					{
						deviceCommandContext->SetTexture(
							blendTexBindingSlot, currentBlendTex);
						lastBlendTex = currentBlendTex;
					}

					itt->m_Texture->GetMaterial().GetStaticUniforms().BindUniforms(deviceCommandContext, shader);

					float c = itt->m_Texture->GetTextureMatrix()[0];
					float ms = itt->m_Texture->GetTextureMatrix()[8];
					deviceCommandContext->SetUniform(
						textureTransformBindingSlot, c, ms);
				}
				else
				{
					deviceCommandContext->SetTexture(
						baseTexBindingSlot, g_Renderer.GetTextureManager().GetErrorTexture()->GetBackendTexture());
				}

				for (VertexBufferBatches::iterator itv = itt->m_Batches.begin(); itv != itt->m_Batches.end(); ++itv)
				{
					// Rebind the VB only if it changed since the last batch
					if (itv->first != lastVB || shader != previousShader)
					{
						lastVB = itv->first;
						previousShader = shader;

						itv->first->UploadIfNeeded(deviceCommandContext);

						const uint32_t stride = sizeof(SBlendVertex);

						deviceCommandContext->SetVertexAttributeFormat(
							Renderer::Backend::VertexAttributeStream::POSITION,
							Renderer::Backend::Format::R32G32B32_SFLOAT,
							offsetof(SBlendVertex, m_Position), stride,
							Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
						deviceCommandContext->SetVertexAttributeFormat(
							Renderer::Backend::VertexAttributeStream::NORMAL,
							Renderer::Backend::Format::R32G32B32_SFLOAT,
							offsetof(SBlendVertex, m_Normal), stride,
							Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
						deviceCommandContext->SetVertexAttributeFormat(
							Renderer::Backend::VertexAttributeStream::UV0,
							Renderer::Backend::Format::R32G32B32_SFLOAT,
							offsetof(SBlendVertex, m_Position), stride,
							Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
						deviceCommandContext->SetVertexAttributeFormat(
							Renderer::Backend::VertexAttributeStream::UV1,
							Renderer::Backend::Format::R32G32_SFLOAT,
							offsetof(SBlendVertex, m_AlphaUVs), stride,
							Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

						deviceCommandContext->SetVertexBuffer(0, itv->first->GetBuffer());
					}

					for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
					{
						it->first->UploadIfNeeded(deviceCommandContext);
						deviceCommandContext->SetIndexBuffer(it->first->GetBuffer());

						BatchElements& batch = it->second;

						for (size_t i = 0; i < batch.first.size(); ++i)
							deviceCommandContext->DrawIndexed(batch.second[i], batch.first[i], 0);

						g_Renderer.m_Stats.m_DrawCalls++;
						g_Renderer.m_Stats.m_BlendSplats++;
						g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 3;
					}
				}
			}
			deviceCommandContext->EndPass();
		}
	}
}

void CPatchRData::RenderStreams(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const std::vector<CPatchRData*>& patches, const bool bindPositionAsTexCoord)
{
	PROFILE3("render terrain streams");

	// Each batch has a list of index counts, and a list of pointers-to-first-indexes
	using StreamBatchElements = std::pair<std::vector<u32>, std::vector<u32>>;

	// Group batches by index buffer
	using StreamIndexBufferBatches = std::map<CVertexBuffer*, StreamBatchElements>;

	// Group batches by vertex buffer
	using StreamVertexBufferBatches = std::map<CVertexBuffer*, StreamIndexBufferBatches>;

	StreamVertexBufferBatches batches;

 	PROFILE_START("compute batches");

 	// Collect all the patches into their appropriate batches
	for (const CPatchRData* patch : patches)
	{
		StreamBatchElements& batch = batches[patch->m_VBBase->m_Owner][patch->m_VBBaseIndices->m_Owner];

		batch.first.push_back(patch->m_VBBaseIndices->m_Count);

 		batch.second.push_back(patch->m_VBBaseIndices->m_Index);
 	}

 	PROFILE_END("compute batches");

	const uint32_t stride = sizeof(SBaseVertex);

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32B32_SFLOAT,
		offsetof(SBaseVertex, m_Position), stride,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	if (bindPositionAsTexCoord)
	{
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV0,
			Renderer::Backend::Format::R32G32B32_SFLOAT,
			offsetof(SBaseVertex, m_Position), stride,
			Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	}

 	// Render each batch
	for (const std::pair<CVertexBuffer* const, StreamIndexBufferBatches>& streamBatch : batches)
	{
		streamBatch.first->UploadIfNeeded(deviceCommandContext);

		deviceCommandContext->SetVertexBuffer(0, streamBatch.first->GetBuffer());

		for (const std::pair<CVertexBuffer* const, StreamBatchElements>& batchIndexBuffer : streamBatch.second)
		{
			batchIndexBuffer.first->UploadIfNeeded(deviceCommandContext);
			deviceCommandContext->SetIndexBuffer(batchIndexBuffer.first->GetBuffer());

			const StreamBatchElements& batch = batchIndexBuffer.second;

			for (size_t i = 0; i < batch.first.size(); ++i)
				deviceCommandContext->DrawIndexed(batch.second[i], batch.first[i], 0);

			g_Renderer.m_Stats.m_DrawCalls++;
			g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 3;
		}
	}
}

void CPatchRData::RenderOutline()
{
	CTerrain* terrain = m_Patch->m_Parent;
	ssize_t gx = m_Patch->m_X * PATCH_SIZE;
	ssize_t gz = m_Patch->m_Z * PATCH_SIZE;

	CVector3D pos;
	std::vector<CVector3D> line;
	for (ssize_t i = 0, j = 0; i <= PATCH_SIZE; ++i)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}
	for (ssize_t i = PATCH_SIZE, j = 1; j <= PATCH_SIZE; ++j)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}
	for (ssize_t i = PATCH_SIZE-1, j = PATCH_SIZE; i >= 0; --i)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}
	for (ssize_t i = 0, j = PATCH_SIZE-1; j >= 0; --j)
	{
		terrain->CalcPosition(gx + i, gz + j, pos);
		line.push_back(pos);
	}

	g_Renderer.GetDebugRenderer().DrawLine(line, CColor(0.0f, 0.0f, 1.0f, 1.0f), 0.1f);
}

void CPatchRData::RenderSides(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const std::vector<CPatchRData*>& patches)
{
	PROFILE3("render terrain sides");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain sides");

	if (patches.empty())
		return;

	const uint32_t stride = sizeof(SSideVertex);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32B32_SFLOAT,
		offsetof(SSideVertex, m_Position), stride,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

	CVertexBuffer* lastVB = nullptr;
	for (CPatchRData* patch : patches)
	{
		ENSURE(patch->m_UpdateFlags == 0);
		if (!patch->m_VBSides)
			continue;
		if (lastVB != patch->m_VBSides->m_Owner)
		{
			lastVB = patch->m_VBSides->m_Owner;
			patch->m_VBSides->m_Owner->UploadIfNeeded(deviceCommandContext);

			deviceCommandContext->SetVertexBuffer(0, patch->m_VBSides->m_Owner->GetBuffer());
		}

		deviceCommandContext->Draw(patch->m_VBSides->m_Index, patch->m_VBSides->m_Count);

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_TerrainTris += patch->m_VBSides->m_Count / 3;
	}
}

void CPatchRData::RenderPriorities(CTextRenderer& textRenderer)
{
	CTerrain* terrain = m_Patch->m_Parent;
	const CCamera& camera = *(g_Game->GetView()->GetCamera());

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
			camera.GetScreenCoordinates(pos, x, y);

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

	// Number of vertices in each direction in each patch
	ENSURE(PATCH_SIZE % water_cell_size == 0);

	m_VBWater.Reset();
	m_VBWaterIndices.Reset();
	m_VBWaterShore.Reset();
	m_VBWaterIndicesShore.Reset();

	m_WaterBounds.SetEmpty();

	// We need to use this to access the water manager or we may not have the
	// actual values but some compiled-in defaults
	CmpPtr<ICmpWaterManager> cmpWaterManager(*m_Simulation, SYSTEM_ENTITY);
	if (!cmpWaterManager)
		return;

	// Build data for water
	std::vector<SWaterVertex> water_vertex_data;
	std::vector<u16> water_indices;
	u16 water_index_map[PATCH_SIZE+1][PATCH_SIZE+1];
	memset(water_index_map, 0xFF, sizeof(water_index_map));

	// Build data for shore
	std::vector<SWaterVertex> water_vertex_data_shore;
	std::vector<u16> water_indices_shore;
	u16 water_shore_index_map[PATCH_SIZE+1][PATCH_SIZE+1];
	memset(water_shore_index_map, 0xFF, sizeof(water_shore_index_map));

	const WaterManager& waterManager = g_Renderer.GetSceneRenderer().GetWaterManager();

	CPatch* patch = m_Patch;
	CTerrain* terrain = patch->m_Parent;

	ssize_t mapSize = terrain->GetVerticesPerSide();

	// Top-left coordinates of our patch.
	ssize_t px = m_Patch->m_X * PATCH_SIZE;
	ssize_t pz = m_Patch->m_Z * PATCH_SIZE;

	// To whoever implements different water heights, this is a TODO: water height)
	float waterHeight = cmpWaterManager->GetExactWaterLevel(0.0f,0.0f);

	// The 4 points making a water tile.
	int moves[4][2] = {
		{0, 0},
		{water_cell_size, 0},
		{0, water_cell_size},
		{water_cell_size, water_cell_size}
	};
	// Where to look for when checking for water for shore tiles.
	int check[10][2] = {
		{0, 0},
		{water_cell_size, 0},
		{water_cell_size*2, 0},
		{0, water_cell_size},
		{0, water_cell_size*2},
		{water_cell_size, water_cell_size},
		{water_cell_size*2, water_cell_size*2},
		{-water_cell_size, 0},
		{0, -water_cell_size},
		{-water_cell_size, -water_cell_size}
	};

	// build vertices, uv, and shader varying
	for (ssize_t z = 0; z < PATCH_SIZE; z += water_cell_size)
	{
		for (ssize_t x = 0; x < PATCH_SIZE; x += water_cell_size)
		{
			// Check that this tile is close to water
			bool nearWater = false;
			for (size_t test = 0; test < 10; ++test)
				if (terrain->GetVertexGroundLevel(x + px + check[test][0], z + pz + check[test][1]) < waterHeight)
					nearWater = true;
			if (!nearWater)
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

				ssize_t xx = x + px + moves[i][0];
				ssize_t zz = z + pz + moves[i][1];

				SWaterVertex vertex;
				terrain->CalcPosition(xx,zz, vertex.m_Position);
				float depth = waterHeight - vertex.m_Position.Y;

				vertex.m_Position.Y = waterHeight;

				m_WaterBounds += vertex.m_Position;

				vertex.m_WaterData = CVector2D(waterManager.m_WindStrength[xx + zz*mapSize], depth);

				water_index_map[z+moves[i][1]][x+moves[i][0]] = static_cast<u16>(water_vertex_data.size());
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
			if (terrain->GetVertexGroundLevel(x+px, z+pz) < waterHeight &&
				terrain->GetVertexGroundLevel(x+px + water_cell_size, z+pz) < waterHeight &&
				terrain->GetVertexGroundLevel(x+px, z+pz+water_cell_size) < waterHeight &&
				terrain->GetVertexGroundLevel(x+px + water_cell_size, z+pz+water_cell_size) < waterHeight)
				continue;

			for (int i = 0; i < 4; ++i)
			{
				if (water_shore_index_map[z+moves[i][1]][x+moves[i][0]] != 0xFFFF)
					continue;
				ssize_t xx = x + px + moves[i][0];
				ssize_t zz = z + pz + moves[i][1];

				SWaterVertex vertex;
				terrain->CalcPosition(xx,zz, vertex.m_Position);

				vertex.m_Position.Y += 0.02f;
				m_WaterBounds += vertex.m_Position;

				vertex.m_WaterData = CVector2D(0.0f, -5.0f);

				water_shore_index_map[z+moves[i][1]][x+moves[i][0]] = static_cast<u16>(water_vertex_data_shore.size());
				water_vertex_data_shore.push_back(vertex);
			}
			if (terrain->GetTriangulationDir(x + px, z + pz))
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

	// No vertex buffers if no data generated
	if (!water_indices.empty())
	{
		m_VBWater = g_VBMan.AllocateChunk(
			sizeof(SWaterVertex), water_vertex_data.size(),
			Renderer::Backend::IBuffer::Type::VERTEX, false,
			nullptr, CVertexBufferManager::Group::WATER);
		m_VBWater->m_Owner->UpdateChunkVertices(m_VBWater.Get(), &water_vertex_data[0]);

		m_VBWaterIndices = g_VBMan.AllocateChunk(
			sizeof(u16), water_indices.size(),
			Renderer::Backend::IBuffer::Type::INDEX, false,
			nullptr, CVertexBufferManager::Group::WATER);
		m_VBWaterIndices->m_Owner->UpdateChunkVertices(m_VBWaterIndices.Get(), &water_indices[0]);
	}

	if (!water_indices_shore.empty())
	{
		m_VBWaterShore = g_VBMan.AllocateChunk(
			sizeof(SWaterVertex), water_vertex_data_shore.size(),
			Renderer::Backend::IBuffer::Type::VERTEX, false,
			nullptr, CVertexBufferManager::Group::WATER);
		m_VBWaterShore->m_Owner->UpdateChunkVertices(m_VBWaterShore.Get(), &water_vertex_data_shore[0]);

		// Construct indices buffer
		m_VBWaterIndicesShore = g_VBMan.AllocateChunk(
			sizeof(u16), water_indices_shore.size(),
			Renderer::Backend::IBuffer::Type::INDEX, false,
			nullptr, CVertexBufferManager::Group::WATER);
		m_VBWaterIndicesShore->m_Owner->UpdateChunkVertices(m_VBWaterIndicesShore.Get(), &water_indices_shore[0]);
	}
}

void CPatchRData::RenderWaterSurface(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const bool bindWaterData)
{
	ASSERT(m_UpdateFlags == 0);

	if (!m_VBWater)
		return;

	m_VBWater->m_Owner->UploadIfNeeded(deviceCommandContext);
	m_VBWaterIndices->m_Owner->UploadIfNeeded(deviceCommandContext);

	const uint32_t stride = sizeof(SWaterVertex);
	const uint32_t firstVertexOffset = m_VBWater->m_Index * stride;

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32B32_SFLOAT,
		firstVertexOffset + offsetof(SWaterVertex, m_Position), stride,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	if (bindWaterData)
	{
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV1,
			Renderer::Backend::Format::R32G32_SFLOAT,
			firstVertexOffset + offsetof(SWaterVertex, m_WaterData), stride,
			Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	}

	deviceCommandContext->SetVertexBuffer(0, m_VBWater->m_Owner->GetBuffer());
	deviceCommandContext->SetIndexBuffer(m_VBWaterIndices->m_Owner->GetBuffer());

	deviceCommandContext->DrawIndexed(m_VBWaterIndices->m_Index, m_VBWaterIndices->m_Count, 0);

	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_WaterTris += m_VBWaterIndices->m_Count / 3;
}

void CPatchRData::RenderWaterShore(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
{
	ASSERT(m_UpdateFlags == 0);

	if (!m_VBWaterShore)
		return;

	m_VBWaterShore->m_Owner->UploadIfNeeded(deviceCommandContext);
	m_VBWaterIndicesShore->m_Owner->UploadIfNeeded(deviceCommandContext);

	const uint32_t stride = sizeof(SWaterVertex);
	const uint32_t firstVertexOffset = m_VBWaterShore->m_Index * stride;

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32B32_SFLOAT,
		firstVertexOffset + offsetof(SWaterVertex, m_Position), stride,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::UV1,
		Renderer::Backend::Format::R32G32_SFLOAT,
		firstVertexOffset + offsetof(SWaterVertex, m_WaterData), stride,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

	deviceCommandContext->SetVertexBuffer(0, m_VBWaterShore->m_Owner->GetBuffer());
	deviceCommandContext->SetIndexBuffer(m_VBWaterIndicesShore->m_Owner->GetBuffer());

	deviceCommandContext->DrawIndexed(m_VBWaterIndicesShore->m_Index, m_VBWaterIndicesShore->m_Count, 0);

	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_WaterTris += m_VBWaterIndicesShore->m_Count / 3;
}
