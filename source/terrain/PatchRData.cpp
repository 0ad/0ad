#pragma warning(disable:4786)

#include <assert.h>
#include <set>
#include <algorithm>
#include "res/tex.h"
#include "Renderer.h"
#include "PatchRData.h"
#include "AlphaMapCalculator.h"

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


CPatchRData::CPatchRData(CPatch* patch) : m_Patch(patch), m_Vertices(0), m_VBBase(0), m_VBBlends(0) 
{
	assert(patch);
	Build();
}

CPatchRData::~CPatchRData() 
{
	delete[] m_Vertices;
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
//				u32 count=neighbourTextures.size();
// janwas fixing warnings: not used?
				// sort textures from lowest to highest priority
				std::sort(neighbourTextures.begin(),neighbourTextures.end());

				// for each of the neighbouring textures ..
				for (uint k=0;k<neighbourTextures.size();++k) {

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
						splat.m_Indices[0]=vindex;
						splat.m_Indices[1]=vindex+1;
						splat.m_Indices[2]=vindex+2;
						splat.m_Indices[3]=vindex+3;
						splats.push_back(splat);

						// add this texture to set of unique splat textures
						splatTextures.insert(splat.m_Texture);
					}
				}
			}
		}
	}

	// now build outgoing splats
	m_BlendSplats.resize(splatTextures.size());
	int splatCount=0;

	std::set<Handle>::iterator iter=splatTextures.begin();
	for (;iter!=splatTextures.end();++iter) {
		Handle tex=*iter;

		SSplat& splat=m_BlendSplats[splatCount];
		splat.m_IndexStart=(u32)m_BlendIndices.size();
		splat.m_Texture=tex;

		for (uint k=0;k<splats.size();k++) {
			if (splats[k].m_Texture==tex) {
				m_BlendIndices.push_back(splats[k].m_Indices[0]);
				m_BlendIndices.push_back(splats[k].m_Indices[1]);
				m_BlendIndices.push_back(splats[k].m_Indices[2]);
				m_BlendIndices.push_back(splats[k].m_Indices[3]);
				splat.m_IndexCount+=4;
			}
		}
		splatCount++;
	}

	if (g_Renderer.m_Caps.m_VBO) {
		if (m_VBBlends) {
			// destroy old buffer
			glDeleteBuffersARB(1,(GLuint*) &m_VBBlends);
		} else {
			// generate buffer index
			glGenBuffersARB(1,(GLuint*) &m_VBBlends);
		}

		// create new buffer
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBlends);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB,m_BlendVertices.size()*sizeof(SBlendVertex),&m_BlendVertices[0],GL_STATIC_DRAW_ARB);
	}
}

void CPatchRData::BuildIndices()
{
	// number of vertices in each direction in each patch
	int vsize=PATCH_SIZE+1;

	// release existing indices and bins
	m_Indices.clear();
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
	
	for (uint i=0;i<m_Splats.size();i++) {
		Handle h=textures[i];

		SSplat& splat=m_Splats[i];
		splat.m_Texture=h;
		splat.m_IndexStart=(u32)m_Indices.size();
		
		for (int j=0;j<PATCH_SIZE;j++) {
			for (int i=0;i<PATCH_SIZE;i++) {
				if (texgrid[j][i]==h){
					m_Indices.push_back(((j+0)*vsize+(i+0)));
					m_Indices.push_back(((j+0)*vsize+(i+1)));
					m_Indices.push_back(((j+1)*vsize+(i+1)));
					m_Indices.push_back(((j+1)*vsize+(i+0)));
				}
			}
		}
		splat.m_IndexCount=(u32)(m_Indices.size()-splat.m_IndexStart);
	}
}

inline int clamp(int x,int min,int max)
{
	if (x<min) return min;
	else if (x>max) return max;
	else return x;
}

static SColor4ub ConvertColor(const RGBColor& src)
{
	SColor4ub result;
	result.R=clamp(int(src.X*255),0,255);
	result.G=clamp(int(src.Y*255),0,255);
	result.B=clamp(int(src.Z*255),0,255);
	result.A=0xff;
	return result;
}

static void BuildHeightmapNormals(int size,u16 *heightmap,CVector3D* normals)
{
    int x, y;
    int sm=size-1;

    for(y = 0;y < size; y++)
		for(x = 0; x < size; x++) {

	      // Access current normalmap grid point
	      CVector3D* N = &normals[y*size+x];

	      // Compute normal by using the height differential
		  u16 h1=(x==sm) ? heightmap[y*size+x] : heightmap[y*size+x+1];
		  u16 h2=(y==sm) ? heightmap[y*size+x] : heightmap[(y+1)*size+x];
		  u16 h3=(x==0) ? heightmap[y*size+x] : heightmap[y*size+x-1];
		  u16 h4=(y==0) ? heightmap[y*size+x] : heightmap[(y-1)*size+x+1];
          N->X = (h3-h1)*HEIGHT_SCALE;
          N->Y = CELL_SIZE;
          N->Z = (h4-h2)*HEIGHT_SCALE;

	      // Normalize it
		  float len=N->GetLength();
		  if (len>0) {
			  (*N)*=1.0f/len;
		  } else {
			  *N=CVector3D(0,0,0);
		  }
	}
}

void CPatchRData::BuildVertices()
{
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
	for (int j=0; j<vsize; j++)
	{
		for (int i=0; i<vsize; i++)
		{
			int ix=px*16+i;
			int iz=pz*16+j;

			CVector3D pos,normal;
			terrain->CalcPosition(ix,iz,pos);			
			terrain->CalcNormal(ix,iz,normal);
			
			RGBColor c;
			g_Renderer.m_SHCoeffsTerrain.Evaluate(normal,c);

			int v=(j*vsize)+i;
			vertices[v].m_UVs[0]=i*0.125f;
			vertices[v].m_UVs[1]=j*0.125f;
			vertices[v].m_Color=ConvertColor(c);
			vertices[v].m_Position=pos;							
		}
	}
	
	if (g_Renderer.m_Caps.m_VBO) {
		if (!m_VBBase) {
			glGenBuffersARB(1,(GLuint*) &m_VBBase);
			glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBase);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB,vsize*vsize*sizeof(SBaseVertex),0,GL_STATIC_DRAW_ARB);
		} 
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBase);
		glBufferSubDataARB(GL_ARRAY_BUFFER_ARB,0,vsize*vsize*sizeof(SBaseVertex),m_Vertices);
	} 
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
	assert(m_UpdateFlags==0);

	u8* base;
	if (g_Renderer.m_Caps.m_VBO) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBase);
		base=0;
	} else {
		base=(u8*) &m_Vertices[0];
	}

	// setup data pointers
	u32 stride=sizeof(SBaseVertex);
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_Position));
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBaseVertex,m_Color));
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBaseVertex,m_UVs[0]));
	
	// render each splat
	for (uint i=0;i<m_Splats.size();i++) {
		SSplat& splat=m_Splats[i];
		tex_bind(splat.m_Texture);
		glDrawElements(GL_QUADS,splat.m_IndexCount,GL_UNSIGNED_SHORT,&m_Indices[splat.m_IndexStart]);
		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}
}

void CPatchRData::RenderStreams(u32 streamflags)
{
	assert(m_UpdateFlags==0);

	u8* base;
	if (g_Renderer.m_Caps.m_VBO) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBase);
		base=0;
	} else {
		base=(u8*) &m_Vertices[0];
	}

	// setup data pointers
	glVertexPointer(3,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_Position));
	if (streamflags & STREAM_UV0) glTexCoordPointer(2,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_UVs));
	
	// render all base splats at once
	glDrawElements(GL_QUADS,(GLsizei)m_Indices.size(),GL_UNSIGNED_SHORT,&m_Indices[0]);

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris+=(u32)m_Indices.size()/2;
}


void CPatchRData::RenderBlends()
{
	assert(m_UpdateFlags==0);

	if (m_BlendVertices.size()==0) return;

	u8* base;
	if (g_Renderer.m_Caps.m_VBO) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBlends);
		base=0;
	} else {
		base=(u8*) &m_BlendVertices[0];
	}


	// setup data pointers
	u32 stride=sizeof(SBlendVertex);
	glVertexPointer(3,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_Position));
	glColorPointer(4,GL_UNSIGNED_BYTE,stride,base+offsetof(SBlendVertex,m_Color));
	
	glClientActiveTexture(GL_TEXTURE0_ARB);
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_UVs[0]));

	glClientActiveTexture(GL_TEXTURE1_ARB);
	glTexCoordPointer(2,GL_FLOAT,stride,base+offsetof(SBlendVertex,m_AlphaUVs[0]));

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);

	for (uint i=0;i<m_BlendSplats.size();i++) {
		SSplat& splat=m_BlendSplats[i];
		tex_bind(splat.m_Texture);
		glDrawElements(GL_QUADS,splat.m_IndexCount,GL_UNSIGNED_SHORT,&m_BlendIndices[splat.m_IndexStart]);

		// bump stats
		g_Renderer.m_Stats.m_DrawCalls++;
		g_Renderer.m_Stats.m_BlendSplats++;
		g_Renderer.m_Stats.m_TerrainTris+=splat.m_IndexCount/2;
	}
}

void CPatchRData::RenderOutline()
{
	const u16 EdgeIndices[PATCH_SIZE*4] = {
		  1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,
		 33,  50,  67,  84, 101, 118, 135, 152, 169, 186, 203, 220, 237, 254, 271, 288, 
		287, 286, 285, 284, 283, 282, 281, 280, 279, 278, 277, 276, 275, 274, 273, 272, 
		255, 238, 221, 204, 187, 170, 153, 136, 119, 102,  85,  68,  51,  34,  17,   0
	};

	u8* base;
	if (g_Renderer.m_Caps.m_VBO) {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,m_VBBase);
		base=0;
	} else {
		base=(u8*) &m_Vertices[0];
	}

	// setup data pointers
	glVertexPointer(3,GL_FLOAT,sizeof(SBaseVertex),base+offsetof(SBaseVertex,m_Position));
	// render outline as line loop
	u32 numIndices=sizeof(EdgeIndices)/sizeof(u16);
	glDrawElements(GL_LINE_LOOP,numIndices,GL_UNSIGNED_SHORT,EdgeIndices);

	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris+=numIndices/2;
}
