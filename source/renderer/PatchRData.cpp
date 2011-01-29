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
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpRangeManager.h"

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
CPatchRData::CPatchRData(CPatch* patch) : m_Patch(patch), m_VBBase(0), m_VBBlends(0), m_Vertices(0)
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
			return a.m_TileMask & (1 << 8);
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
	m_BlendIndices.clear();
	m_BlendSplats.clear();
	m_BlendVertices.clear();
	m_BlendVertexIndices.clear();

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
		splat.m_IndexStart = m_BlendIndices.size();
		splat.m_Texture = blendLayers[k].m_Texture;

		for (size_t t = 0; t < blendLayers[k].m_Tiles.size(); ++t)
		{
			SBlendLayer::Tile& tile = blendLayers[k].m_Tiles[t];

			ssize_t index = AddBlend(tile.i, tile.j, tile.shape);

			if (index != -1)
			{
				// (These indices will get incremented by the VB base offset later)
				m_BlendIndices.push_back(index + 0);
				m_BlendIndices.push_back(index + 1);
				m_BlendIndices.push_back(index + 2);
				m_BlendIndices.push_back(index + 3);
				splat.m_IndexCount += 4;
			}
		}
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

		m_VBBlends = g_VBMan.Allocate(sizeof(SBlendVertex), m_BlendVertices.size(), true);
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends, &m_BlendVertices[0]);

		debug_assert(m_VBBlends->m_Index < 65536);
		unsigned short base = (unsigned short)m_VBBlends->m_Index;

		// Update the indices to include the base offset
		for (size_t k = 0; k < m_BlendIndices.size(); ++k)
			m_BlendIndices[k] += base;
	}
}

ssize_t CPatchRData::AddBlend(u16 i, u16 j, u8 shape)
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
		return -1;

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
	const size_t vindex = m_BlendVertices.size();

	const SBaseVertex& vtx0 = m_Vertices[(j * vsize) + i];
	CalculateUV(dst.m_UVs, gx, gz);
	dst.m_AlphaUVs[0] = vtx[0].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[0].m_AlphaUVs[1];
	dst.m_LOSColor = vtx0.m_LOSColor;
	dst.m_Position = vtx0.m_Position;
	m_BlendVertices.push_back(dst);
	m_BlendVertexIndices.push_back((j * vsize) + i);

	const SBaseVertex& vtx1 = m_Vertices[(j * vsize) + i + 1];
	CalculateUV(dst.m_UVs, gx + 1, gz);
	dst.m_AlphaUVs[0] = vtx[1].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[1].m_AlphaUVs[1];
	dst.m_LOSColor = vtx1.m_LOSColor;
	dst.m_Position = vtx1.m_Position;
	m_BlendVertices.push_back(dst);
	m_BlendVertexIndices.push_back((j * vsize) + i + 1);

	const SBaseVertex& vtx2 = m_Vertices[((j + 1) * vsize) + i + 1];
	CalculateUV(dst.m_UVs, gx + 1, gz + 1);
	dst.m_AlphaUVs[0] = vtx[2].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[2].m_AlphaUVs[1];
	dst.m_LOSColor = vtx2.m_LOSColor;
	dst.m_Position = vtx2.m_Position;
	m_BlendVertices.push_back(dst);
	m_BlendVertexIndices.push_back(((j + 1) * vsize) + i + 1);

	const SBaseVertex& vtx3 = m_Vertices[((j + 1) * vsize) + i];
	CalculateUV(dst.m_UVs, gx, gz + 1);
	dst.m_AlphaUVs[0] = vtx[3].m_AlphaUVs[0];
	dst.m_AlphaUVs[1] = vtx[3].m_AlphaUVs[1];
	dst.m_LOSColor = vtx3.m_LOSColor;
	dst.m_Position = vtx3.m_Position;
	m_BlendVertices.push_back(dst);
	m_BlendVertexIndices.push_back(((j + 1) * vsize) + i);

	return vindex;
}

void CPatchRData::BuildIndices()
{
	// must have allocated some vertices before trying to build corresponding indices
	debug_assert(m_VBBase);

	// number of vertices in each direction in each patch
	ssize_t vsize=PATCH_SIZE+1;

	// release existing indices and bins
	m_Indices.clear();
	m_ShadowMapIndices.clear();
	m_Splats.clear();

	// build grid of textures on this patch and boundaries of adjacent patches
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
	for (size_t i=0;i<m_Splats.size();i++) {
		CTerrainTextureEntry* tex=textures[i];

		SSplat& splat=m_Splats[i];
		splat.m_Texture=tex;
		splat.m_IndexStart=m_Indices.size();

		for (ssize_t j=0;j<PATCH_SIZE;j++) {
			for (ssize_t i=0;i<PATCH_SIZE;i++) {
				if (texgrid[j][i]==tex){
					m_Indices.push_back(u16(((j+0)*vsize+(i+0))+base));
					m_Indices.push_back(u16(((j+0)*vsize+(i+1))+base));
					m_Indices.push_back(u16(((j+1)*vsize+(i+1))+base));
					m_Indices.push_back(u16(((j+1)*vsize+(i+0))+base));
				}
			}
		}
		splat.m_IndexCount=m_Indices.size()-splat.m_IndexStart;
	}

	// build indices for the shadow map pass
	for (ssize_t j=0;j<PATCH_SIZE;j++) {
		for (ssize_t i=0;i<PATCH_SIZE;i++) {
			m_ShadowMapIndices.push_back(u16(((j+0)*vsize+(i+0))+base));
			m_ShadowMapIndices.push_back(u16(((j+0)*vsize+(i+1))+base));
			m_ShadowMapIndices.push_back(u16(((j+1)*vsize+(i+1))+base));
			m_ShadowMapIndices.push_back(u16(((j+1)*vsize+(i+0))+base));
		}
	}
}


void CPatchRData::BuildVertices()
{
	// create both vertices and lighting colors

	CVector3D normal;

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
			vertices[v].m_LOSColor = SColor4ub(0, 0, 0, 0);	// will be set to the proper value in Update()
			CalculateUV(vertices[v].m_UVs, ix, iz);

			// Calculate diffuse lighting for this vertex
			// Ambient is added by the lighting pass (since ambient is the same
			// for all vertices, it need not be stored in the vertex structure)
			terrain->CalcNormal(ix,iz,normal);

			RGBColor diffuse;
			lightEnv.EvaluateDirect(normal, diffuse);
			vertices[v].m_DiffuseColor = ConvertRGBColorTo4ub(diffuse);
		}
	}

	// upload to vertex buffer
	if (!m_VBBase) {
		m_VBBase=g_VBMan.Allocate(sizeof(SBaseVertex),vsize*vsize,true);
	}
	m_VBBase->m_Owner->UpdateChunkVertices(m_VBBase,m_Vertices);
}

void CPatchRData::Build()
{
	BuildVertices();
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
		BuildIndices();
		BuildBlends();

		m_UpdateFlags=0;
	}

	// Update vertex colors, which are affected by LOS

	ssize_t px=m_Patch->m_X;
	ssize_t pz=m_Patch->m_Z;

	CTerrain* terrain=m_Patch->m_Parent;
	ssize_t vsize=PATCH_SIZE+1;
	SColor4ub baseColour = terrain->GetBaseColour();

	CmpPtr<ICmpRangeManager> cmpRangeManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpRangeManager.null())
	{
		for (ssize_t j = 0; j < vsize; ++j)
		{
			for (ssize_t i = 0; i < vsize; ++i)
			{
				ssize_t v = (j*vsize)+i;
				m_Vertices[v].m_LOSColor = baseColour;
			}
		}
	}
	else
	{
		ICmpRangeManager::CLosQuerier los (cmpRangeManager->GetLosQuerier(g_Game->GetPlayerID()));

		// this is very similar to BuildVertices(), but just for color
		for (ssize_t j = 0; j < vsize; j++)
		{
			for (ssize_t i = 0; i < vsize; i++)
			{
				ssize_t ix = px * PATCH_SIZE + i;
				ssize_t iz = pz * PATCH_SIZE + j;
				ssize_t v = (j * vsize) + i;

				SColor4ub losMod;
				if (los.IsVisible(ix, iz))
					losMod = baseColour;
				else if (los.IsExplored(ix, iz))
					losMod = SColor4ub(178, 178, 178, 255);
				else
					losMod = SColor4ub(0, 0, 0, 255);

				m_Vertices[v].m_LOSColor = losMod;
			}
		}
	}

	// upload base vertices into their vertex buffer
	m_VBBase->m_Owner->UpdateChunkVertices(m_VBBase,m_Vertices);

	// update blend colors by copying them from vertex colors
	for(size_t i=0; i<m_BlendVertices.size(); i++)
	{
		m_BlendVertices[i].m_LOSColor = m_Vertices[m_BlendVertexIndices[i]].m_LOSColor;
	}

	// upload blend vertices into their vertex buffer too
	if(m_BlendVertices.size())
	{
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends,&m_BlendVertices[0]);
	}
}

void CPatchRData::RenderBase(bool losColor)
{
	debug_assert(m_UpdateFlags==0);

	SBaseVertex *base=(SBaseVertex *)m_VBBase->m_Owner->Bind();

	// setup data pointers
	GLsizei stride=sizeof(SBaseVertex);
	glVertexPointer(3,GL_FLOAT,stride,&base->m_Position[0]);
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,losColor ? &base->m_LOSColor : &base->m_DiffuseColor);
	glTexCoordPointer(2,GL_FLOAT,stride,&base->m_UVs[0]);

	// render each splat
	for (size_t i=0;i<m_Splats.size();i++) {
		SSplat& splat=m_Splats[i];

		if (splat.m_Texture)
			splat.m_Texture->GetTexture()->Bind();
		else
			g_Renderer.GetTextureManager().GetErrorTexture()->Bind();

		if (!g_Renderer.m_SkipSubmit) {
			glDrawElements(GL_QUADS, (GLsizei)splat.m_IndexCount,
				GL_UNSIGNED_SHORT, &m_Indices[splat.m_IndexStart]);
		}

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}

	CVertexBuffer::Unbind();
}

void CPatchRData::RenderStreams(int streamflags, bool losColor)
{
	debug_assert(m_UpdateFlags==0);

	SBaseVertex* base=(SBaseVertex *)m_VBBase->m_Owner->Bind();

	// setup data pointers
	GLsizei stride=sizeof(SBaseVertex);
	glVertexPointer(3, GL_FLOAT, stride, &base->m_Position);
	if (streamflags & STREAM_UV0) {
		glTexCoordPointer(2, GL_FLOAT, stride, &base->m_UVs);
	} else if (streamflags & STREAM_POSTOUV0) {
		glTexCoordPointer(3, GL_FLOAT, stride, &base->m_Position);
	}
	if (streamflags & STREAM_COLOR)
	{
		glColorPointer(4,GL_UNSIGNED_BYTE,stride,losColor ? &base->m_LOSColor : &base->m_DiffuseColor);
	}

	// render all base splats at once
	if (!g_Renderer.m_SkipSubmit) {
		glDrawElements(GL_QUADS,(GLsizei)m_Indices.size(),GL_UNSIGNED_SHORT,&m_Indices[0]);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris+=m_Indices.size()/2;

	CVertexBuffer::Unbind();
}


void CPatchRData::RenderBlends()
{
	debug_assert(m_UpdateFlags==0);

	if (m_BlendVertices.empty())
		return;

	u8* base=m_VBBlends->m_Owner->Bind();

	// setup data pointers
	GLsizei stride=sizeof(SBlendVertex);
	// ((GCC warns about offsetof: SBlendVertex contains a CVector3D which has
	// a constructor, and so is not a POD type, and so offsetof is theoretically
	// invalid - see http://gcc.gnu.org/ml/gcc/2003-11/msg00281.html - but it
	// doesn't seem to be worth changing this code since it works anyway.))
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_Position));
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBlendVertex,m_LOSColor));

	pglClientActiveTextureARB(GL_TEXTURE0);
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_UVs[0]));

	pglClientActiveTextureARB(GL_TEXTURE1);
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_AlphaUVs[0]));

	for (size_t i=0;i<m_BlendSplats.size();i++) {
		SSplat& splat=m_BlendSplats[i];

		if (splat.m_Texture)
			splat.m_Texture->GetTexture()->Bind();
		else
			g_Renderer.GetTextureManager().GetErrorTexture()->Bind();

		if (!g_Renderer.m_SkipSubmit) {
			glDrawElements(GL_QUADS, (GLsizei)splat.m_IndexCount,
				GL_UNSIGNED_SHORT, &m_BlendIndices[splat.m_IndexStart]);
		}

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_BlendSplats++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}

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
