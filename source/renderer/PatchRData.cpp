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

#include <set>
#include <algorithm>

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "lib/res/graphics/unifont.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/World.h"
#include "ps/GameSetup/Config.h"
#include "renderer/AlphaMapCalculator.h"
#include "renderer/PatchRData.h"
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
CPatchRData::CPatchRData(CPatch* patch) :
	m_Patch(patch), m_VBBase(0), m_VBBaseIndices(0), m_VBSides(0), m_VBBlends(0), m_Vertices(0)
{
	debug_assert(patch);
	Build();
}

///////////////////////////////////////////////////////////////////
// CPatchRData destructor
CPatchRData::~CPatchRData()
{
	// delete copy of vertex data
	delete[] m_Vertices;
	// release vertex buffer chunks
	if (m_VBBase) g_VBMan.Release(m_VBBase);
	if (m_VBBaseIndices) g_VBMan.Release(m_VBBaseIndices);
	if (m_VBSides) g_VBMan.Release(m_VBSides);
	if (m_VBBlends) g_VBMan.Release(m_VBBlends);
}

const float uvFactor = 0.125f / sqrt(2.f);
static void CalculateUV(float uv[2], ssize_t x, ssize_t z)
{
	// The UV axes are offset 45 degrees from XZ
	uv[0] = ( x-z)*uvFactor;
	uv[1] = (-x-z)*uvFactor;
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
	m_BlendSplats.clear();
	m_BlendVertices.clear();

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
					SBlendLayer::Tile t = { blendStacks[k].i, blendStacks[k].j, blendStacks[k].blends.back().m_TileMask };
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
		splat.m_IndexStart = m_BlendVertices.size();
		splat.m_Texture = blendLayers[k].m_Texture;

		for (size_t t = 0; t < blendLayers[k].m_Tiles.size(); ++t)
		{
			SBlendLayer::Tile& tile = blendLayers[k].m_Tiles[t];
			AddBlend(tile.i, tile.j, tile.shape);
		}

		splat.m_IndexCount = m_BlendVertices.size() - splat.m_IndexStart;
	}

	// Release existing vertex buffer chunk
	if (m_VBBlends)
	{
		g_VBMan.Release(m_VBBlends);
		m_VBBlends = 0;
	}

	if (m_BlendVertices.size())
	{
		// Construct vertex buffer

		m_VBBlends = g_VBMan.Allocate(sizeof(SBlendVertex), m_BlendVertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends, &m_BlendVertices[0]);

		debug_assert(m_VBBlends->m_Index < 65536);
		unsigned short base = (unsigned short)m_VBBlends->m_Index;

		// Update the indices to include the base offset
		for (size_t k = 0; k < m_BlendSplats.size(); ++k)
			m_BlendSplats[k].m_IndexStart += base;
	}
}

void CPatchRData::AddBlend(u16 i, u16 j, u8 shape)
{
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

	float u0 = g_Renderer.m_AlphaMapCoords[alphamap].u0;
	float u1 = g_Renderer.m_AlphaMapCoords[alphamap].u1;
	float v0 = g_Renderer.m_AlphaMapCoords[alphamap].v0;
	float v1 = g_Renderer.m_AlphaMapCoords[alphamap].v1;

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

	ssize_t vsize = PATCH_SIZE + 1;

	SBlendVertex dst;

	const SBaseVertex& vtx0 = m_Vertices[(j * vsize) + i];
	CalculateUV(dst.m_UVs, gx, gz);
	dst.m_AlphaUVs[0] = vtx[0].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[0].m_AlphaUVs[1];
	dst.m_Position = vtx0.m_Position;
	m_BlendVertices.push_back(dst);

	const SBaseVertex& vtx1 = m_Vertices[(j * vsize) + i + 1];
	CalculateUV(dst.m_UVs, gx + 1, gz);
	dst.m_AlphaUVs[0] = vtx[1].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[1].m_AlphaUVs[1];
	dst.m_Position = vtx1.m_Position;
	m_BlendVertices.push_back(dst);

	const SBaseVertex& vtx2 = m_Vertices[((j + 1) * vsize) + i + 1];
	CalculateUV(dst.m_UVs, gx + 1, gz + 1);
	dst.m_AlphaUVs[0] = vtx[2].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[2].m_AlphaUVs[1];
	dst.m_Position = vtx2.m_Position;
	m_BlendVertices.push_back(dst);

	const SBaseVertex& vtx3 = m_Vertices[((j + 1) * vsize) + i];
	CalculateUV(dst.m_UVs, gx, gz + 1);
	dst.m_AlphaUVs[0] = vtx[3].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[3].m_AlphaUVs[1];
	dst.m_Position = vtx3.m_Position;
	m_BlendVertices.push_back(dst);
}

void CPatchRData::BuildIndices()
{
	// must have allocated some vertices before trying to build corresponding indices
	debug_assert(m_VBBase);

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
	debug_assert(base + vsize*vsize < 65536); // mustn't overflow u16 indexes
	for (size_t i=0;i<m_Splats.size();i++) {
		CTerrainTextureEntry* tex=textures[i];

		SSplat& splat=m_Splats[i];
		splat.m_Texture=tex;
		splat.m_IndexStart=indices.size();

		for (ssize_t j=0;j<PATCH_SIZE;j++) {
			for (ssize_t i=0;i<PATCH_SIZE;i++) {
				if (texgrid[j][i]==tex){
					indices.push_back(u16(((j+0)*vsize+(i+0))+base));
					indices.push_back(u16(((j+0)*vsize+(i+1))+base));
					indices.push_back(u16(((j+1)*vsize+(i+1))+base));
					indices.push_back(u16(((j+1)*vsize+(i+0))+base));
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

	debug_assert(indices.size());

	// Construct vertex buffer
	m_VBBaseIndices = g_VBMan.Allocate(sizeof(u16), indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
	m_VBBaseIndices->m_Owner->UpdateChunkVertices(m_VBBaseIndices, &indices[0]);
}


void CPatchRData::BuildVertices()
{
	// create both vertices and lighting colors

	// number of vertices in each direction in each patch
	ssize_t vsize=PATCH_SIZE+1;

	if (!m_Vertices) {
		m_Vertices=new SBaseVertex[vsize*vsize];
	}
	SBaseVertex* vertices=m_Vertices;


	// get index of this patch
	ssize_t px=m_Patch->m_X;
	ssize_t pz=m_Patch->m_Z;

	CTerrain* terrain=m_Patch->m_Parent;
	const CLightEnv& lightEnv = g_Renderer.GetLightEnv();

	// build vertices
	for (ssize_t j=0;j<vsize;j++) {
		for (ssize_t i=0;i<vsize;i++) {
			ssize_t ix=px*PATCH_SIZE+i;
			ssize_t iz=pz*PATCH_SIZE+j;
			ssize_t v=(j*vsize)+i;

			// calculate vertex data
			terrain->CalcPosition(ix,iz,vertices[v].m_Position);
			CalculateUV(vertices[v].m_UVs, ix, iz);

			// Calculate diffuse lighting for this vertex
			// Ambient is added by the lighting pass (since ambient is the same
			// for all vertices, it need not be stored in the vertex structure)
			CVector3D normal;
			terrain->CalcNormal(ix,iz,normal);

			RGBColor diffuse;
			lightEnv.EvaluateDirect(normal, diffuse);
			vertices[v].m_DiffuseColor = ConvertRGBColorTo4ub(diffuse);
		}
	}

	// upload to vertex buffer
	if (!m_VBBase)
		m_VBBase = g_VBMan.Allocate(sizeof(SBaseVertex), vsize * vsize, GL_STATIC_DRAW, GL_ARRAY_BUFFER);

	m_VBBase->m_Owner->UpdateChunkVertices(m_VBBase,m_Vertices);
}

void CPatchRData::BuildSide(std::vector<SSideVertex>& vertices, CPatchSideFlags side)
{
	ssize_t vsize = PATCH_SIZE + 1;
	CTerrain* terrain = m_Patch->m_Parent;
	CmpPtr<ICmpWaterManager> cmpWaterManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);

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
		if (!cmpWaterManager.null())
			waterHeight = cmpWaterManager->GetExactWaterLevel(pos.X, pos.Z);
		pos.Y = std::max(pos.Y, waterHeight);

		SSideVertex v0, v1;
		v0.m_Position = pos;
		v1.m_Position = pos;
		v1.m_Position.Y = 0;

		// If this is the start of this tristrip, but we've already got a partial
		// tristrip, and a couple of degenerate triangles to join the strips properly
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
}

void CPatchRData::Update()
{
	if (m_UpdateFlags!=0) {
		// TODO,RC 11/04/04 - need to only rebuild necessary bits of renderdata rather
		// than everything; it's complicated slightly because the blends are dependent
		// on both vertex and index data
		BuildVertices();
		BuildSides();
		BuildIndices();
		BuildBlends();

		m_UpdateFlags=0;
	}
}

void CPatchRData::RenderBases(const std::vector<CPatchRData*>& patches)
{
	// Each multidraw batch has a list of index counts, and a list of pointers-to-first-indexes
	typedef std::pair<std::vector<GLint>, std::vector<void*> > BatchElements;

	// Group batches by index buffer
	typedef std::map<CVertexBuffer*, BatchElements> IndexBufferBatches;

	// Group batches by vertex buffer
	typedef std::map<CVertexBuffer*, IndexBufferBatches> VertexBufferBatches;

	// Group batches by texture
	typedef std::map<CTerrainTextureEntry*, VertexBufferBatches> TextureBatches;

 	TextureBatches batches;

 	PROFILE_START("compute batches");

 	// Collect all the patches' base splats into their appropriate batches
 	for (size_t i = 0; i < patches.size(); ++i)
 	{
 		CPatchRData* patch = patches[i];
 		for (size_t j = 0; j < patch->m_Splats.size(); ++j)
 		{
 			SSplat& splat = patch->m_Splats[j];

 			BatchElements& batch = batches[splat.m_Texture][patch->m_VBBase->m_Owner][patch->m_VBBaseIndices->m_Owner];

 			batch.first.push_back(splat.m_IndexCount);

 			u8* indexBase = patch->m_VBBaseIndices->m_Owner->GetBindAddress();
 			batch.second.push_back(indexBase + sizeof(u16)*(patch->m_VBBaseIndices->m_Index + splat.m_IndexStart));
		}
 	}

 	PROFILE_END("compute batches");

 	// Render each batch
 	for (TextureBatches::iterator itt = batches.begin(); itt != batches.end(); ++itt)
	{
		if (itt->first)
			itt->first->GetTexture()->Bind();
		else
			g_Renderer.GetTextureManager().GetErrorTexture()->Bind();

		for (VertexBufferBatches::iterator itv = itt->second.begin(); itv != itt->second.end(); ++itv)
		{
			GLsizei stride = sizeof(SBaseVertex);
			SBaseVertex *base = (SBaseVertex *)itv->first->Bind();
			glVertexPointer(3, GL_FLOAT, stride, &base->m_Position[0]);
			glColorPointer(4, GL_UNSIGNED_BYTE, stride, &base->m_DiffuseColor);
			glTexCoordPointer(2, GL_FLOAT, stride, &base->m_UVs[0]);

			for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
			{
				it->first->Bind();

				BatchElements& batch = it->second;

				if (!g_Renderer.m_SkipSubmit)
				{
					pglMultiDrawElementsEXT(GL_QUADS, &batch.first[0], GL_UNSIGNED_SHORT,
							(GLvoid**)&batch.second[0], batch.first.size());
				}

				g_Renderer.m_Stats.m_DrawCalls++;
				g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 2;
			}
		}
	}

	CVertexBuffer::Unbind();
}

/**
 * Helper structure for RenderBlends.
 */
struct SBlendBatch
{
	CTerrainTextureEntry* m_Texture;

	// Each multidraw batch has a list of start vertex offsets, and a list of vertex counts
	typedef std::pair<std::vector<GLint>, std::vector<GLsizei> > BatchElements;

	// Group batches by vertex buffer
	typedef std::map<CVertexBuffer*, BatchElements> VertexBufferBatches;

 	VertexBufferBatches m_Batches;
};

void CPatchRData::RenderBlends(const std::vector<CPatchRData*>& patches)
{
 	std::vector<SBlendBatch> batches;

 	PROFILE_START("compute batches");

 	// Reserve an arbitrary size that's probably big enough in most cases,
 	// to avoid heavy reallocations
 	batches.reserve(256);

	std::vector<std::pair<CVertexBuffer*, std::vector<SSplat> > > blendStacks;
	blendStacks.reserve(patches.size());

	// Extract all the blend splats from each patch
 	for (size_t i = 0; i < patches.size(); ++i)
 	{
 		CPatchRData* patch = patches[i];
 		if (!patch->m_BlendSplats.empty())
 		{
 			blendStacks.push_back(std::make_pair(patch->m_VBBlends->m_Owner, patch->m_BlendSplats));
 			// Reverse the splats so the first to be rendered is at the back of the list
 			std::reverse(blendStacks.back().second.begin(), blendStacks.back().second.end());
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
				if (!blendStacks[k].second.empty() && blendStacks[k].second.back().m_Texture == tex)
				{
					SBlendBatch::BatchElements& batch = batches.back().m_Batches[blendStacks[k].first];
					batch.first.push_back(blendStacks[k].second.back().m_IndexStart);
					batch.second.push_back(blendStacks[k].second.back().m_IndexCount);
					blendStacks[k].second.pop_back();
				}
			}
		}

		CTerrainTextureEntry* bestTex = NULL;
		size_t bestStackSize = 0;

		for (size_t k = 0; k < blendStacks.size(); ++k)
		{
			if (blendStacks[k].second.size() > bestStackSize)
			{
				bestStackSize = blendStacks[k].second.size();
				bestTex = blendStacks[k].second.back().m_Texture;
			}
		}

		if (bestStackSize == 0)
			break;

		SBlendBatch layer;
		layer.m_Texture = bestTex;
		batches.push_back(layer);
	}

 	PROFILE_END("compute batches");

 	CVertexBuffer* lastVB = NULL;

 	for (std::vector<SBlendBatch>::iterator itt = batches.begin(); itt != batches.end(); ++itt)
	{
		if (itt->m_Texture)
			itt->m_Texture->GetTexture()->Bind();
		else
			g_Renderer.GetTextureManager().GetErrorTexture()->Bind();

		for (SBlendBatch::VertexBufferBatches::iterator itv = itt->m_Batches.begin(); itv != itt->m_Batches.end(); ++itv)
		{
			// Rebind the VB only if it changed since the last batch
			if (itv->first != lastVB)
			{
				lastVB = itv->first;
				GLsizei stride = sizeof(SBlendVertex);
				SBlendVertex *base = (SBlendVertex *)itv->first->Bind();

				glVertexPointer(3, GL_FLOAT, stride, &base->m_Position[0]);

				pglClientActiveTextureARB(GL_TEXTURE0);
				glTexCoordPointer(2, GL_FLOAT, stride, &base->m_UVs[0]);

				pglClientActiveTextureARB(GL_TEXTURE1);
				glTexCoordPointer(2, GL_FLOAT, stride, &base->m_AlphaUVs[0]);
			}

			SBlendBatch::BatchElements& batch = itv->second;

			// Since every blend vertex likely has distinct UV even if they
			// share positions, there's no value in using indexed arrays, so
			// we just use DrawArrays instead of DrawElements
			if (!g_Renderer.m_SkipSubmit)
				pglMultiDrawArraysEXT(GL_QUADS, &batch.first[0], &batch.second[0], batch.first.size());

			g_Renderer.m_Stats.m_DrawCalls++;
			g_Renderer.m_Stats.m_BlendSplats++;
			g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.second.begin(), batch.second.end(), 0) / 2;
		}
	}

	pglClientActiveTextureARB(GL_TEXTURE0);

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderStreams(const std::vector<CPatchRData*>& patches, int streamflags)
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

 	// Render each batch
 	for (VertexBufferBatches::iterator itv = batches.begin(); itv != batches.end(); ++itv)
	{
		GLsizei stride = sizeof(SBaseVertex);
		SBaseVertex *base = (SBaseVertex *)itv->first->Bind();

		glVertexPointer(3, GL_FLOAT, stride, &base->m_Position);
		if (streamflags & STREAM_UV0)
		{
			pglClientActiveTextureARB(GL_TEXTURE0);
			glTexCoordPointer(2, GL_FLOAT, stride, &base->m_UVs);
		}
		if (streamflags & STREAM_POSTOUV0)
		{
			pglClientActiveTextureARB(GL_TEXTURE0);
			glTexCoordPointer(3, GL_FLOAT, stride, &base->m_Position);
		}
		if (streamflags & STREAM_POSTOUV1)
		{
			pglClientActiveTextureARB(GL_TEXTURE1);
			glTexCoordPointer(3, GL_FLOAT, stride, &base->m_Position);
		}
		if (streamflags & STREAM_POSTOUV2)
		{
			pglClientActiveTextureARB(GL_TEXTURE2);
			glTexCoordPointer(3, GL_FLOAT, stride, &base->m_Position);
		}
		if (streamflags & STREAM_POSTOUV3)
		{
			pglClientActiveTextureARB(GL_TEXTURE3);
			glTexCoordPointer(3, GL_FLOAT, stride, &base->m_Position);
		}
		if (streamflags & STREAM_COLOR)
		{
			glColorPointer(4, GL_UNSIGNED_BYTE, stride, &base->m_DiffuseColor);
		}

		for (IndexBufferBatches::iterator it = itv->second.begin(); it != itv->second.end(); ++it)
		{
			it->first->Bind();

			BatchElements& batch = it->second;

			if (!g_Renderer.m_SkipSubmit)
			{
				pglMultiDrawElementsEXT(GL_QUADS, &batch.first[0], GL_UNSIGNED_SHORT,
						(GLvoid**)&batch.second[0], batch.first.size());
			}

			g_Renderer.m_Stats.m_DrawCalls++;
			g_Renderer.m_Stats.m_TerrainTris += std::accumulate(batch.first.begin(), batch.first.end(), 0) / 2;
		}
	}

	pglClientActiveTextureARB(GL_TEXTURE0);

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderOutline()
{
	size_t vsize=PATCH_SIZE+1;

	glBegin(GL_LINES);
	for (ssize_t i=0;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[i].m_Position.X);
		glVertex3fv(&m_Vertices[i+1].m_Position.X);
	}
	glEnd();
	glBegin(GL_LINES);
	for (ssize_t i=0;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[PATCH_SIZE+(i*(PATCH_SIZE+1))].m_Position.X);
		glVertex3fv(&m_Vertices[PATCH_SIZE+((i+1)*(PATCH_SIZE+1))].m_Position.X);
	}
	glEnd();
	glBegin(GL_LINES);
	for (ssize_t i=1;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[(vsize*vsize)-i].m_Position.X);
		glVertex3fv(&m_Vertices[(vsize*vsize)-(i+1)].m_Position.X);
	}
	glEnd();
	glBegin(GL_LINES);
	for (ssize_t i=1;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[(vsize*(vsize-1))-(i*vsize)].m_Position.X);
		glVertex3fv(&m_Vertices[(vsize*(vsize-1))-((i+1)*vsize)].m_Position.X);
	}
	glEnd();
}

void CPatchRData::RenderSides()
{
	debug_assert(m_UpdateFlags==0);

	if (!m_VBSides)
		return;

	SSideVertex *base = (SSideVertex *)m_VBSides->m_Owner->Bind();

	// setup data pointers
	GLsizei stride = sizeof(SSideVertex);
	glVertexPointer(3, GL_FLOAT, stride, &base->m_Position);

	if (!g_Renderer.m_SkipSubmit)
		glDrawArrays(GL_TRIANGLE_STRIP, m_VBSides->m_Index, (GLsizei)m_VBSides->m_Count);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris += m_VBSides->m_Count - 2;

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderPriorities()
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
			pos.X += CELL_SIZE/4.f;
			pos.Z += CELL_SIZE/4.f;

			float x, y;
			camera->GetScreenCoordinates(pos, x, y);

			glPushMatrix();
			glTranslatef(x, g_yres - y, 0.f);

			// Draw the text upside-down, because it's aligned with
			// the GUI (which uses the top-left as (0,0))
			glScalef(1.0f, -1.0f, 1.0f);

			glwprintf(L"%d", m_Patch->m_MiniPatches[j][i].Priority);
			glPopMatrix();
		}
	}
}
