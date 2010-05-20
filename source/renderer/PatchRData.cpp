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

#include "precompiled.h"


#include <set>
#include <algorithm>
#include "ps/Pyrogenesis.h"
#include "lib/res/graphics/ogl_tex.h"
#include "graphics/LightEnv.h"
#include "Renderer.h"
#include "renderer/PatchRData.h"
#include "AlphaMapCalculator.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "maths/MathUtil.h"
#include "simulation/LOSManager.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "simulation2/Simulation2.h"

const ssize_t BlendOffsets[8][2] = {
	{  0, -1 },
	{ -1, -1 },
	{ -1,  0 },
	{ -1,  1 },
	{  0,  1 },
	{  1,  1 },
	{  1,  0 },
	{  1, -1 }
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


static Handle GetTerrainTileTexture(CTerrain* terrain,ssize_t gx,ssize_t gz)
{
	CMiniPatch* mp=terrain->GetTile(gx,gz);
	return mp ? mp->Tex1 : 0;
}

const float uvFactor = 0.125f / sqrt(2.f);
static void CalculateUV(float uv[2], ssize_t x, ssize_t z)
{
	// The UV axes are offset 45 degrees from XZ
	uv[0] = ( x-z)*uvFactor;
	uv[1] = (-x-z)*uvFactor;
}

struct STmpSplat {
	Handle m_Texture;
	u16 m_Indices[4];
};

void CPatchRData::BuildBlends()
{
	m_BlendIndices.clear();
	m_BlendSplats.clear();
	m_BlendVertices.clear();
	m_BlendVertexIndices.clear();

	CTerrain* terrain=m_Patch->m_Parent;

	// temporary list of splats
	std::vector<STmpSplat> splats;
	// set of textures used for splats
	std::set<Handle> splatTextures;

	// for each tile in patch ..
	for (ssize_t j=0;j<PATCH_SIZE;j++) {
		for (ssize_t i=0;i<PATCH_SIZE;i++) {
			ssize_t gx,gz;
			CMiniPatch* mp=&m_Patch->m_MiniPatches[j][i];
			mp->GetTileIndex(gx,gz);

			// build list of textures of higher priority than current tile that are used by neighbouring tiles
			std::vector<STex> neighbourTextures;
			for (int m=-1;m<=1;m++) {
				for (int k=-1;k<=1;k++) {
					CMiniPatch* nmp=terrain->GetTile(gx+k,gz+m);
					if (nmp && nmp->Tex1 != mp->Tex1) {
						if (nmp->Tex1Priority>mp->Tex1Priority || (nmp->Tex1Priority==mp->Tex1Priority && nmp->Tex1>mp->Tex1)) {
							STex tex;
							tex.m_Handle=nmp->Tex1;
							tex.m_Priority=nmp->Tex1Priority;
							if (std::find(neighbourTextures.begin(),neighbourTextures.end(),tex)==neighbourTextures.end()) {
								neighbourTextures.push_back(tex);
							}
						}
					}
				}
			}
			if (neighbourTextures.size()>0) {
				// sort textures from lowest to highest priority
				std::sort(neighbourTextures.begin(),neighbourTextures.end());

				// for each of the neighbouring textures ..
				size_t count=neighbourTextures.size();
				for (size_t k=0;k<count;++k) {

					// now build the grid of blends dependent on whether the tile adjacent to the current tile
					// uses the current neighbour texture
					BlendShape8 shape;
					for (size_t m=0;m<8;m++) {
						ssize_t ox=gx+BlendOffsets[m][1];
						ssize_t oz=gz+BlendOffsets[m][0];

						// get texture on adjacent tile
						Handle atex=GetTerrainTileTexture(terrain,ox,oz);
						// fill 0/1 into shape array
						shape[m]=(atex==neighbourTextures[k].m_Handle) ? 0 : 1;
					}

					// calculate the required alphamap and the required rotation of the alphamap from blendshape
					unsigned int alphamapflags;
					int alphamap=CAlphaMapCalculator::Calculate(shape,alphamapflags);

					// now actually render the blend tile (if we need one)
					if (alphamap!=-1) {
						float u0=g_Renderer.m_AlphaMapCoords[alphamap].u0;
						float u1=g_Renderer.m_AlphaMapCoords[alphamap].u1;
						float v0=g_Renderer.m_AlphaMapCoords[alphamap].v0;
						float v1=g_Renderer.m_AlphaMapCoords[alphamap].v1;
						if (alphamapflags & BLENDMAP_FLIPU) {
							// flip u
							float t=u0;
							u0=u1;
							u1=t;
						}

						if (alphamapflags & BLENDMAP_FLIPV) {
							// flip v
							float t=v0;
							v0=v1;
							v1=t;
						}

						int base=0;
						if (alphamapflags & BLENDMAP_ROTATE90) {
							// rotate 1
							base=1;
						} else if (alphamapflags & BLENDMAP_ROTATE180) {
							// rotate 2
							base=2;
						} else if (alphamapflags & BLENDMAP_ROTATE270) {
							// rotate 3
							base=3;
						}

						SBlendVertex vtx[4];
						vtx[(base+0)%4].m_AlphaUVs[0]=u0;
						vtx[(base+0)%4].m_AlphaUVs[1]=v0;
						vtx[(base+1)%4].m_AlphaUVs[0]=u1;
						vtx[(base+1)%4].m_AlphaUVs[1]=v0;
						vtx[(base+2)%4].m_AlphaUVs[0]=u1;
						vtx[(base+2)%4].m_AlphaUVs[1]=v1;
						vtx[(base+3)%4].m_AlphaUVs[0]=u0;
						vtx[(base+3)%4].m_AlphaUVs[1]=v1;

						ssize_t vsize=PATCH_SIZE+1;

						SBlendVertex dst;
						const size_t vindex=m_BlendVertices.size();

						const SBaseVertex& vtx0=m_Vertices[(j*vsize)+i];
						CalculateUV(dst.m_UVs, gx, gz);
						dst.m_AlphaUVs[0]=vtx[0].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[0].m_AlphaUVs[1];
						dst.m_LOSColor=vtx0.m_LOSColor;
						dst.m_Position=vtx0.m_Position;
						m_BlendVertices.push_back(dst);
						m_BlendVertexIndices.push_back((j*vsize)+i);

						const SBaseVertex& vtx1=m_Vertices[(j*vsize)+i+1];
						CalculateUV(dst.m_UVs, gx+1, gz);
						dst.m_AlphaUVs[0]=vtx[1].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[1].m_AlphaUVs[1];
						dst.m_LOSColor=vtx1.m_LOSColor;
						dst.m_Position=vtx1.m_Position;
						m_BlendVertices.push_back(dst);
						m_BlendVertexIndices.push_back((j*vsize)+i+1);

						const SBaseVertex& vtx2=m_Vertices[((j+1)*vsize)+i+1];
						CalculateUV(dst.m_UVs, gx+1, gz+1);
						dst.m_AlphaUVs[0]=vtx[2].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[2].m_AlphaUVs[1];
						dst.m_LOSColor=vtx2.m_LOSColor;
						dst.m_Position=vtx2.m_Position;
						m_BlendVertices.push_back(dst);
						m_BlendVertexIndices.push_back(((j+1)*vsize)+i+1);

						const SBaseVertex& vtx3=m_Vertices[((j+1)*vsize)+i];
						CalculateUV(dst.m_UVs, gx, gz+1);
						dst.m_AlphaUVs[0]=vtx[3].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[3].m_AlphaUVs[1];
						dst.m_LOSColor=vtx3.m_LOSColor;
						dst.m_Position=vtx3.m_Position;
						m_BlendVertices.push_back(dst);
						m_BlendVertexIndices.push_back(((j+1)*vsize)+i);

						// build a splat for this quad
						STmpSplat splat;
						splat.m_Texture=neighbourTextures[k].m_Handle;
						splat.m_Indices[0]=(u16)(vindex);
						splat.m_Indices[1]=(u16)(vindex+1);
						splat.m_Indices[2]=(u16)(vindex+2);
						splat.m_Indices[3]=(u16)(vindex+3);
						splats.push_back(splat);

						// add this texture to set of unique splat textures
						splatTextures.insert(splat.m_Texture);
					}
				}
			}
		}
	}

	// build vertex data
	if (m_VBBlends) {
		// release existing vertex buffer chunk
		g_VBMan.Release(m_VBBlends);
		m_VBBlends=0;
	}
	if (m_BlendVertices.size()) {
		m_VBBlends=g_VBMan.Allocate(sizeof(SBlendVertex),m_BlendVertices.size(),true);
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends,&m_BlendVertices[0]);

		// now build outgoing splats
		m_BlendSplats.resize(splatTextures.size());
		size_t splatCount=0;

		debug_assert(m_VBBlends->m_Index < 65536);
		unsigned short base = (unsigned short)m_VBBlends->m_Index;
		std::set<Handle>::iterator iter=splatTextures.begin();
		for (;iter!=splatTextures.end();++iter) {
			Handle tex=*iter;

			SSplat& splat=m_BlendSplats[splatCount];
			splat.m_IndexStart=m_BlendIndices.size();
			splat.m_Texture=tex;

			for (size_t k=0;k<splats.size();k++) {
				if (splats[k].m_Texture==tex) {
					m_BlendIndices.push_back(splats[k].m_Indices[0]+base);
					m_BlendIndices.push_back(splats[k].m_Indices[1]+base);
					m_BlendIndices.push_back(splats[k].m_Indices[2]+base);
					m_BlendIndices.push_back(splats[k].m_Indices[3]+base);
					splat.m_IndexCount+=4;
				}
			}
			splatCount++;
		}
	}
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
	std::vector<Handle> textures;
	Handle texgrid[PATCH_SIZE][PATCH_SIZE];
	for (ssize_t j=0;j<PATCH_SIZE;j++) {
		for (ssize_t i=0;i<PATCH_SIZE;i++) {
			Handle h=m_Patch->m_MiniPatches[j][i].Tex1;
			texgrid[j][i]=h;
			if (std::find(textures.begin(),textures.end(),h)==textures.end()) {
				textures.push_back(h);
			}
		}
	}

	// now build base splats from interior textures
	m_Splats.resize(textures.size());
	// build indices for base splats
	size_t base=m_VBBase->m_Index;
	for (size_t i=0;i<m_Splats.size();i++) {
		Handle h=textures[i];

		SSplat& splat=m_Splats[i];
		splat.m_Texture=h;
		splat.m_IndexStart=m_Indices.size();

		for (ssize_t j=0;j<PATCH_SIZE;j++) {
			for (ssize_t i=0;i<PATCH_SIZE;i++) {
				if (texgrid[j][i]==h){
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
	ssize_t mapSize=terrain->GetVerticesPerSide();
	ssize_t vsize=PATCH_SIZE+1;
	SColor4ub baseColour = terrain->GetBaseColour();

	if (g_Game && false) // XXX: need to implement this for new sim system
	{
		CLOSManager* losMgr = g_Game->GetWorld()->GetLOSManager();

		// this is very similar to BuildVertices(), but just for color
		for (ssize_t j=0;j<vsize;j++) {
			for (ssize_t i=0;i<vsize;i++) {
				ssize_t ix=px*PATCH_SIZE+i;
				ssize_t iz=pz*PATCH_SIZE+j;
				ssize_t v=(j*vsize)+i;

				const ssize_t DX[] = {1,1,0,0};
				const ssize_t DZ[] = {0,1,1,0};
				SColor4ub losMod = baseColour;

				for(size_t k=0; k<4; k++)
				{
					ssize_t tx = ix - DX[k];
					ssize_t tz = iz - DZ[k];

					if(tx >= 0 && tz >= 0 && tx <= mapSize-2 && tz <= mapSize-2)
					{
						ELOSStatus s = losMgr->GetStatus(tx, tz, g_Game->GetLocalPlayer());
						if(s==LOS_EXPLORED && losMod.R > 178)
							losMod = SColor4ub(178, 178, 178, 255);
						else if(s==LOS_UNEXPLORED && losMod.R > 0)
							losMod = SColor4ub(0, 0, 0, 255);
					}
				}

				m_Vertices[v].m_LOSColor = losMod;
			}
		}
	}
	else
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
		ogl_tex_bind(splat.m_Texture);

		if (!g_Renderer.m_SkipSubmit) {
			glDrawElements(GL_QUADS, (GLsizei)splat.m_IndexCount,
				GL_UNSIGNED_SHORT, &m_Indices[splat.m_IndexStart]);
		}

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}
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
}


void CPatchRData::RenderBlends()
{
	debug_assert(m_UpdateFlags==0);

	if (m_BlendVertices.size()==0) return;

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
		ogl_tex_bind(splat.m_Texture);

		if (!g_Renderer.m_SkipSubmit) {
			glDrawElements(GL_QUADS, (GLsizei)splat.m_IndexCount,
				GL_UNSIGNED_SHORT, &m_BlendIndices[splat.m_IndexStart]);
		}

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_BlendSplats++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}
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
