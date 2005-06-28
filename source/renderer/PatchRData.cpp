#include "precompiled.h"


#include <set>
#include <algorithm>
#include "Pyrogenesis.h"
#include "res/ogl_tex.h"
#include "Renderer.h"
#include "PatchRData.h"
#include "AlphaMapCalculator.h"
#include "ps/CLogger.h"

///////////////////////////////////////////////////////////////////
// shared list of all submitted patches this frame
std::vector<CPatch*> CPatchRData::m_Patches;


const int BlendOffsets[8][2] = {
	{  0, -1 },
	{ -1, -1 },
	{ -1,  0 },
	{ -1,  1 },
	{  0,  1 },
	{  1,  1 },
	{  1,  0 },
	{  1, -1 }
};








static SColor4ub ConvertColor(const RGBColor& src)
{
	SColor4ub result;
	result.R=(u8)clamp(int(src.X*255),0,255);
	result.G=(u8)clamp(int(src.Y*255),0,255);
	result.B=(u8)clamp(int(src.Z*255),0,255);
	result.A=0xff;
	return result;
}

///////////////////////////////////////////////////////////////////
// CPatchRData constructor
CPatchRData::CPatchRData(CPatch* patch) : m_Patch(patch), m_Vertices(0), m_VBBase(0), m_VBBlends(0) 
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


static Handle GetTerrainTileTexture(CTerrain* terrain,int gx,int gz)
{
	CMiniPatch* mp=terrain->GetTile(gx,gz);
	return mp ? mp->Tex1 : 0;
}

bool QueryAdjacency(int x,int y,Handle h,Handle* texgrid)
{
	for (int j=y-1;j<=y+1;j++) {
		for (int i=x-1;i<=x+1;i++) {
			if (i<0 || i>PATCH_SIZE+1 || j<0 || j>PATCH_SIZE+1) {
				continue;
			}

			if (texgrid[j*(PATCH_SIZE+2)+i]==h) {
				return true;
			}
		}
	}
	return false;
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

	// get index of this patch
	int px=m_Patch->m_X;
	int pz=m_Patch->m_Z;

	CTerrain* terrain=m_Patch->m_Parent;

	// temporary list of splats
	std::vector<STmpSplat> splats;
	// set of textures used for splats
	std::set<Handle> splatTextures;

	// for each tile in patch ..
	for (int j=0;j<PATCH_SIZE;j++) {
		for (int i=0;i<PATCH_SIZE;i++) {
			u32 gx,gz;
			CMiniPatch* mp=&m_Patch->m_MiniPatches[j][i];
			mp->GetTileIndex(gx,gz);

			// build list of textures of higher priority than current tile that are used by neighbouring tiles
			std::vector<STex> neighbourTextures;
			for (int m=-1;m<=1;m++) {					
				for (int k=-1;k<=1;k++) {
					CMiniPatch* nmp=terrain->GetTile(gx+k,gz+m);
					if (nmp) {
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
				uint count=(uint)neighbourTextures.size();
				for (uint k=0;k<count;++k) {

					// now build the grid of blends dependent on whether the tile adjacent to the current tile 
					// uses the current neighbour texture
					BlendShape8 shape;
					for (int m=0;m<8;m++) {
						int ox=gx+BlendOffsets[m][1];
						int oz=gz+BlendOffsets[m][0];
						
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

						int vsize=PATCH_SIZE+1;

						SBlendVertex dst;
						int vindex=(int)m_BlendVertices.size();

						const SBaseVertex& vtx0=m_Vertices[(j*vsize)+i];
						dst.m_UVs[0]=i*0.125f;
						dst.m_UVs[1]=j*0.125f;
						dst.m_AlphaUVs[0]=vtx[0].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[0].m_AlphaUVs[1];
						dst.m_Color=vtx0.m_Color;
						dst.m_Position=vtx0.m_Position;
						m_BlendVertices.push_back(dst);

						const SBaseVertex& vtx1=m_Vertices[(j*vsize)+i+1];
						dst.m_UVs[0]=(i+1)*0.125f;
						dst.m_UVs[1]=j*0.125f;
						dst.m_AlphaUVs[0]=vtx[1].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[1].m_AlphaUVs[1];
						dst.m_Color=vtx1.m_Color;
						dst.m_Position=vtx1.m_Position;
						m_BlendVertices.push_back(dst);

						const SBaseVertex& vtx2=m_Vertices[((j+1)*vsize)+i+1];
						dst.m_UVs[0]=(i+1)*0.125f;
						dst.m_UVs[1]=(j+1)*0.125f;
						dst.m_AlphaUVs[0]=vtx[2].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[2].m_AlphaUVs[1];
						dst.m_Color=vtx2.m_Color;
						dst.m_Position=vtx2.m_Position;
						m_BlendVertices.push_back(dst);

						const SBaseVertex& vtx3=m_Vertices[((j+1)*vsize)+i];
						dst.m_UVs[0]=i*0.125f;
						dst.m_UVs[1]=(j+1)*0.125f;
						dst.m_AlphaUVs[0]=vtx[3].m_AlphaUVs[0];
						dst.m_AlphaUVs[1]=vtx[3].m_AlphaUVs[1];
						dst.m_Color=vtx3.m_Color;
						dst.m_Position=vtx3.m_Position;
						m_BlendVertices.push_back(dst);

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
	if (m_BlendVertices.size())  {
		m_VBBlends=g_VBMan.Allocate(sizeof(SBlendVertex),m_BlendVertices.size(),false);
		m_VBBlends->m_Owner->UpdateChunkVertices(m_VBBlends,&m_BlendVertices[0]);
	
		// now build outgoing splats
		m_BlendSplats.resize(splatTextures.size());
		int splatCount=0;
	
		debug_assert(m_VBBlends->m_Index < 65536);
		unsigned short base = (unsigned short)m_VBBlends->m_Index;
		std::set<Handle>::iterator iter=splatTextures.begin();
		for (;iter!=splatTextures.end();++iter) {
			Handle tex=*iter;
	
			SSplat& splat=m_BlendSplats[splatCount];
			splat.m_IndexStart=(u32)m_BlendIndices.size();
			splat.m_Texture=tex;
	
			for (uint k=0;k<(uint)splats.size();k++) {
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
	int vsize=PATCH_SIZE+1;

	// release existing indices and bins
	m_Indices.clear();
	m_ShadowMapIndices.clear();
	m_Splats.clear();

	// build grid of textures on this patch and boundaries of adjacent patches
	std::vector<Handle> textures;
	Handle texgrid[PATCH_SIZE][PATCH_SIZE];
	for (int j=0;j<PATCH_SIZE;j++) {
		for (int i=0;i<PATCH_SIZE;i++) {
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
	u32 base=(u32)m_VBBase->m_Index;
	for (uint i=0;i<(uint)m_Splats.size();i++) {
		Handle h=textures[i];

		SSplat& splat=m_Splats[i];
		splat.m_Texture=h;
		splat.m_IndexStart=(u32)m_Indices.size();
		
		for (int j=0;j<PATCH_SIZE;j++) {
			for (int i=0;i<PATCH_SIZE;i++) {
				if (texgrid[j][i]==h){
					m_Indices.push_back(((j+0)*vsize+(i+0))+base);
					m_Indices.push_back(((j+0)*vsize+(i+1))+base);
					m_Indices.push_back(((j+1)*vsize+(i+1))+base);
					m_Indices.push_back(((j+1)*vsize+(i+0))+base);
				}
			}
		}
		splat.m_IndexCount=(u32)m_Indices.size()-splat.m_IndexStart;
	}
	
	// build indices for the shadow map pass
	for (int j=0;j<PATCH_SIZE;j++) {
		for (int i=0;i<PATCH_SIZE;i++) {
			m_ShadowMapIndices.push_back(((j+0)*vsize+(i+0))+base);
			m_ShadowMapIndices.push_back(((j+0)*vsize+(i+1))+base);
			m_ShadowMapIndices.push_back(((j+1)*vsize+(i+1))+base);
			m_ShadowMapIndices.push_back(((j+1)*vsize+(i+0))+base);
		}
	}
}


void CPatchRData::BuildVertices()
{
	CVector3D normal;
	RGBColor c;

	// number of vertices in each direction in each patch
	int vsize=PATCH_SIZE+1;
	
	if (!m_Vertices) {
		m_Vertices=new SBaseVertex[vsize*vsize];
	}
	SBaseVertex* vertices=m_Vertices;
	
	
	// get index of this patch
	u32 px=m_Patch->m_X;
	u32 pz=m_Patch->m_Z;
	
	CTerrain* terrain=m_Patch->m_Parent;
	u32 mapSize=terrain->GetVerticesPerSide();

	// build vertices
	for (int j=0;j<vsize;j++) {
		for (int i=0;i<vsize;i++) {
			int ix=px*PATCH_SIZE+i;
			int iz=pz*PATCH_SIZE+j;
			int v=(j*vsize)+i;

			terrain->CalcPosition(ix,iz,vertices[v].m_Position);			
			terrain->CalcNormal(ix,iz,normal);
			g_Renderer.m_SHCoeffsTerrain.Evaluate(normal,c);
			vertices[v].m_Color=ConvertColor(c);
			vertices[v].m_UVs[0]=i*0.125f;
			vertices[v].m_UVs[1]=j*0.125f;
		}
	}
	
	if (!m_VBBase) {
		m_VBBase=g_VBMan.Allocate(sizeof(SBaseVertex),vsize*vsize,false);
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
}

void CPatchRData::RenderBase()
{
	debug_assert(m_UpdateFlags==0);

	u8* base=m_VBBase->m_Owner->Bind();

	// setup data pointers
	u32 stride=sizeof(SBaseVertex);
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_Position));
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBaseVertex,m_Color));
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_UVs[0]));
	
	// render each splat
	for (uint i=0;i<(uint)m_Splats.size();i++) {
		SSplat& splat=m_Splats[i];
		g_Renderer.BindTexture(0,tex_id(splat.m_Texture));
		glDrawElements(GL_QUADS,splat.m_IndexCount,GL_UNSIGNED_SHORT,&m_Indices[splat.m_IndexStart]);
		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}
}

void CPatchRData::RenderStreams(u32 streamflags)
{
	debug_assert(m_UpdateFlags==0);

	u8* base=m_VBBase->m_Owner->Bind();

	// setup data pointers
	glVertexPointer(3,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_Position));
	if (streamflags & STREAM_UV0) {
		glTexCoordPointer(2,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_UVs));
	} else if (streamflags & STREAM_POSTOUV0) {
		glTexCoordPointer(3,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_Position));
	}
	
	// render all base splats at once
	glDrawElements(GL_QUADS,(GLsizei)m_Indices.size(),GL_UNSIGNED_SHORT,&m_Indices[0]);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris+=(u32)m_Indices.size()/2;
}


void CPatchRData::RenderBlends()
{
	debug_assert(m_UpdateFlags==0);

	if (m_BlendVertices.size()==0) return;

	u8* base=m_VBBlends->m_Owner->Bind();

	// setup data pointers
	u32 stride=sizeof(SBlendVertex);
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_Position));
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBlendVertex,m_Color));
	
	glClientActiveTextureARB(GL_TEXTURE0);
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_UVs[0]));

	glClientActiveTextureARB(GL_TEXTURE1);
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_AlphaUVs[0]));

	for (uint i=0;i<(uint)m_BlendSplats.size();i++) {
		SSplat& splat=m_BlendSplats[i];
		g_Renderer.BindTexture(0,tex_id(splat.m_Texture));
		glDrawElements(GL_QUADS,splat.m_IndexCount,GL_UNSIGNED_SHORT,&m_BlendIndices[splat.m_IndexStart]);

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_BlendSplats++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}
}

void CPatchRData::RenderOutlines()
{
	for (uint i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->RenderOutline();
	}
}

void CPatchRData::RenderStreamsAll(u32 streamflags)
{
	for (uint i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->RenderStreams(streamflags);
	}
}

void CPatchRData::RenderOutline()
{
	uint i;
	uint vsize=PATCH_SIZE+1;
	u8* base=m_VBBase->m_Owner->Bind();

	glBegin(GL_LINES);
	for (i=0;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[i].m_Position.X);
		glVertex3fv(&m_Vertices[i+1].m_Position.X);
	}
	glEnd();
	glBegin(GL_LINES);
	for (i=0;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[PATCH_SIZE+(i*(PATCH_SIZE+1))].m_Position.X);
		glVertex3fv(&m_Vertices[PATCH_SIZE+((i+1)*(PATCH_SIZE+1))].m_Position.X);
	}
	glEnd();
	glBegin(GL_LINES);
	for (i=1;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[(vsize*vsize)-i].m_Position.X);
		glVertex3fv(&m_Vertices[(vsize*vsize)-(i+1)].m_Position.X);
	}
	glEnd();
	glBegin(GL_LINES);
	for (i=1;i<PATCH_SIZE;i++) {
		glVertex3fv(&m_Vertices[(vsize*(vsize-1))-(i*vsize)].m_Position.X);
		glVertex3fv(&m_Vertices[(vsize*(vsize-1))-((i+1)*vsize)].m_Position.X);
	}
	glEnd();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// SubmitBaseBatches: submit base batches for this patch to the vertex buffer
void CPatchRData::SubmitBaseBatches()
{
	debug_assert(m_VBBase);

	for (uint i=0;i<m_Splats.size();i++) {
		const SSplat& splat=m_Splats[i];
		m_VBBase->m_Owner->AppendBatch(m_VBBase,splat.m_Texture,splat.m_IndexCount,&m_Indices[splat.m_IndexStart]);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////
// SubmitBlendBatches: submit next set of blend batches for this patch to the vertex buffer;
// return true if all blends on this patch have been submitted, else false
bool CPatchRData::SubmitBlendBatches()
{
	if (m_NextBlendSplat<m_BlendSplats.size()) {
		for (uint i=m_NextBlendSplat;i<m_BlendSplats.size();i++) {
			const SSplat& splat=m_BlendSplats[i];
			m_VBBlends->m_Owner->AppendBatch(m_VBBlends,splat.m_Texture,splat.m_IndexCount,&m_BlendIndices[splat.m_IndexStart]);
		}
		return true;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderBaseSplats: render all base passes of all patches; assumes vertex, texture and color 
// client states are enabled
void CPatchRData::RenderBaseSplats()
{
	uint i;

	// set up texture environment for base pass
	MICROLOG(L"base splat textures");
	glActiveTextureARB(GL_TEXTURE0);
	glClientActiveTextureARB(GL_TEXTURE0);
	oglCheck();
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	oglCheck();

	// Set alpha to 1.0
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_CONSTANT);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	float one[4] = { 1.f, 1.f, 1.f, 1.f };
	glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, one);
	oglCheck();

#if 1
	// submit base batches for each patch to the vertex buffer
	for (i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->SubmitBaseBatches();
	}
	oglCheck();

	// render base passes for each patch
	const std::list<CVertexBuffer*>& buffers=g_VBMan.GetBufferList();
	std::list<CVertexBuffer*>::const_iterator iter;
	for (iter=buffers.begin();iter!=buffers.end();++iter) {
		CVertexBuffer* buffer=*iter;
		
		// any batches in this VB?
		const std::vector<CVertexBuffer::Batch*>& batches=buffer->GetBatches();
		if (batches.size()>0) {
			u8* base=buffer->Bind();

			// setup data pointers
			u32 stride=sizeof(SBaseVertex);

			glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_Position));
			glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBaseVertex,m_Color));
			glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_UVs[0]));

			// render each batch
			for (i=0;i<batches.size();++i) {
				const CVertexBuffer::Batch* batch=batches[i];
				if (batch->m_IndexData.size()>0) {
					g_Renderer.BindTexture(0,tex_id(batch->m_Texture));
					for (uint j=0;j<batch->m_IndexData.size();j++) {
						glDrawElements(GL_QUADS,(GLsizei)batch->m_IndexData[j].first,GL_UNSIGNED_SHORT,batch->m_IndexData[j].second);
						g_Renderer.m_Stats.m_DrawCalls++;
						g_Renderer.m_Stats.m_TerrainTris+=(u32)batch->m_IndexData[j].first/2;
					}
				}
			}
		}
	}
	// everything rendered; empty out batch lists
	MICROLOG(L"clear");
	g_VBMan.ClearBatchIndices();
#else 
	for (i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->RenderBase();
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// RenderBlendSplats: render all blend passes of all patches; assumes vertex, texture and color 
// client states are enabled
void CPatchRData::RenderBlendSplats()
{
	uint i;

	// switch on second uv set
	glClientActiveTextureARB(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0);

	// switch on the composite alpha map texture
	g_Renderer.BindTexture(1,g_Renderer.m_CompositeAlphaMap);

	// setup additional texenv required by blend pass
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_ONE_MINUS_SRC_ALPHA);

	// switch on blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	// no need to write to the depth buffer a second time
	glDepthMask(0);

#if 1
	// submit blend batches for each patch to the vertex buffer
	for (i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->SetupBlendBatches();
	}

	bool finished=true;
	do
	{
		for (i=0;i<m_Patches.size();++i) {
			CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
			if (!patchdata->SubmitBlendBatches()) finished=false;
		}
	} while (!finished);

	// render blend passes for each patch
	const std::list<CVertexBuffer*>& buffers=g_VBMan.GetBufferList();
	std::list<CVertexBuffer*>::const_iterator iter;
	for (iter=buffers.begin();iter!=buffers.end();++iter) {
		CVertexBuffer* buffer=*iter;
		
		// any batches in this VB?
		const std::vector<CVertexBuffer::Batch*>& batches=buffer->GetBatches();
		if (batches.size()>0) {
			u8* base=buffer->Bind();

			// setup data pointers
			u32 stride=sizeof(SBlendVertex);
			glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_Position));
			glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBlendVertex,m_Color));
			glClientActiveTextureARB(GL_TEXTURE0);
			glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_UVs[0]));
		
			glClientActiveTextureARB(GL_TEXTURE1);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_AlphaUVs[0]));

			// render each batch
			for (i=0;i<batches.size();++i) {
				const CVertexBuffer::Batch* batch=batches[i];
				if (batch->m_IndexData.size()>0) {
					g_Renderer.BindTexture(0,tex_id(batch->m_Texture));
					for (uint j=0;j<batch->m_IndexData.size();j++) {
						glDrawElements(GL_QUADS,(GLsizei)batch->m_IndexData[j].first,GL_UNSIGNED_SHORT,batch->m_IndexData[j].second);
						g_Renderer.m_Stats.m_DrawCalls++;
						g_Renderer.m_Stats.m_TerrainTris+=(u32)batch->m_IndexData[j].first/2;
					}
				}
			}
		}
	}
	// everything rendered; empty out batch lists
	g_VBMan.ClearBatchIndices();
#else
	// render blend passes for each patch
	for (i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->RenderBlends();
	}
#endif

	// restore depth writes
	glDepthMask(1);

	// restore default state: switch off blending
	glDisable(GL_BLEND);

	// switch off second uv set
	glClientActiveTextureARB(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0);

	// switch off texture unit 1, make unit 0 active texture
	g_Renderer.BindTexture(1,0);
	glActiveTextureARB(GL_TEXTURE0);
	
	// tidy up client states
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Submit: submit a patch to render this frame
void CPatchRData::Submit(CPatch* patch)
{
	CPatchRData* data=(CPatchRData*) patch->GetRenderData();
	if (data==0) {
		// no renderdata for patch, create it now
		data=new CPatchRData(patch);
		patch->SetRenderData(data);
	} else {
		data->Update();
	}

	m_Patches.push_back(patch);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// ApplyShadowMap: apply given shadow map to all terrain patches; assume the texture matrix
// has been correctly setup on unit 1 to handle the projection
void CPatchRData::ApplyShadowMap(GLuint shadowmaphandle)
{
	uint i;

	g_Renderer.BindTexture(0,shadowmaphandle);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_TEXTURE);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);

	glColor3f(1,1,1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_DST_COLOR,GL_ZERO);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

#if 1
	// submit base batches for each patch to the vertex buffer
	for (i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();
		patchdata->m_VBBase->m_Owner->AppendBatch(patchdata->m_VBBase,0,patchdata->m_ShadowMapIndices.size(),
							&patchdata->m_ShadowMapIndices[0]);
	}

	// render base passes for each patch
	const std::list<CVertexBuffer*>& buffers=g_VBMan.GetBufferList();
	std::list<CVertexBuffer*>::const_iterator iter;
	for (iter=buffers.begin();iter!=buffers.end();++iter) {
		CVertexBuffer* buffer=*iter;
		
		// any batches in this VB?
		const std::vector<CVertexBuffer::Batch*>& batches=buffer->GetBatches();
		if (batches.size()>0) {
			u8* base=buffer->Bind();

			// setup data pointers
			u32 stride=sizeof(SBaseVertex);
			glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_Position));
			glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBaseVertex,m_Color));
			glTexCoordPointer(3,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_Position));

			// render batch (can only be one per buffer, since all batches are flagged as using a null texture)
			const CVertexBuffer::Batch* batch=batches[0];
			for (uint j=0;j<batch->m_IndexData.size();j++) {
				glDrawElements(GL_QUADS,(GLsizei)batch->m_IndexData[j].first,GL_UNSIGNED_SHORT,batch->m_IndexData[j].second);
				g_Renderer.m_Stats.m_DrawCalls++;
				g_Renderer.m_Stats.m_TerrainTris+=(u32)batch->m_IndexData[j].first/2;
			}
		}
	}
	// everything rendered; empty out batch lists
	g_VBMan.ClearBatchIndices();
#else 
	for (uint i=0;i<m_Patches.size();++i) {
		CPatchRData* patchdata=(CPatchRData*) m_Patches[i]->GetRenderData();;
		patchdata->RenderStreams(STREAM_POS|STREAM_POSTOUV0);
	}
#endif

	glDisable(GL_BLEND);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}
