///////////////////////////////////////////////////////////////////////////////
//
// Name:		Terrain.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "res/ogl_tex.h"
#include "res/mem.h"

#include <string.h>
#include "Terrain.h"


///////////////////////////////////////////////////////////////////////////////
// CTerrain constructor
CTerrain::CTerrain() : m_Heightmap(0), m_Patches(0), m_MapSize(0), m_MapSizePatches(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// CTerrain constructor
CTerrain::~CTerrain()
{
	ReleaseData();
}


///////////////////////////////////////////////////////////////////////////////
// ReleaseData: delete any data allocated by this terrain
void CTerrain::ReleaseData()
{
	delete[] m_Heightmap;
	delete[] m_Patches;
}


///////////////////////////////////////////////////////////////////////////////
// Initialise: initialise this terrain to the given size (in patches per side);
// using given heightmap to setup elevation data
bool CTerrain::Initialize(u32 size,const u16* data)
{
	// clean up any previous terrain
	ReleaseData();

	// store terrain size
	m_MapSize=(size*PATCH_SIZE)+1;
	m_MapSizePatches=size;

	// allocate data for new terrain
	m_Heightmap=new u16[m_MapSize*m_MapSize];
	m_Patches=new CPatch[m_MapSizePatches*m_MapSizePatches];

	// given a heightmap?
	if (data) {
		// yes; keep a copy of it
		memcpy(m_Heightmap,data,m_MapSize*m_MapSize*sizeof(u16));
	} else {
		// build a flat terrain
		memset(m_Heightmap,0,m_MapSize*m_MapSize*sizeof(u16));
	}

	// setup patch parents, indices etc
	InitialisePatches();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CalcPosition: calculate the world space position of the vertex at (i,j)
void CTerrain::CalcPosition(u32 i,u32 j,CVector3D& pos)
{
	u16 height=m_Heightmap[j*m_MapSize + i];
	pos.X = float(i)*CELL_SIZE;
	pos.Y = float(height)*HEIGHT_SCALE;
	pos.Z = float(j)*CELL_SIZE;
}


///////////////////////////////////////////////////////////////////////////////
// CalcNormal: calculate the world space normal of the vertex at (i,j)
void CTerrain::CalcNormal(u32 i,u32 j,CVector3D& normal)
{
	CVector3D left, right, up, down;
	
	left.Clear();
	right.Clear();
	up.Clear();
	down.Clear();
	
	// get position of vertex where normal is being evaluated
	CVector3D basepos;
	CalcPosition(i,j,basepos);

	CVector3D tmp;
	if (i>0) {
		CalcPosition(i-1,j,tmp);
		left=tmp-basepos;
	}

	if (i<m_MapSize-1) {
		CalcPosition(i+1,j,tmp);
		right=tmp-basepos;
	}

	if (j>0) {
		CalcPosition(i,j-1,tmp);
		up=tmp-basepos;
	}

	if (j<m_MapSize-1) {
		CalcPosition(i,j+1,tmp);
		down=tmp-basepos;
	}

	CVector3D n0 = up.Cross(left);
	CVector3D n1 = left.Cross(down);
	CVector3D n2 = down.Cross(right);
	CVector3D n3 = right.Cross(up);	

	normal = n0 + n1 + n2 + n3;
	float nlen=normal.GetLength();
	if (nlen>0.00001f) normal*=1.0f/nlen;
}


///////////////////////////////////////////////////////////////////////////////
// GetPatch: return the patch at (x,z) in patch space, or null if the patch is 
// out of bounds
CPatch* CTerrain::GetPatch(int32_t x,int32_t z)
{
	if (x<0 || x>=int32_t(m_MapSizePatches)) return 0;	
	if (z<0 || z>=int32_t(m_MapSizePatches)) return 0;
	return &m_Patches[(z*m_MapSizePatches)+x]; 
}


///////////////////////////////////////////////////////////////////////////////
// GetPatch: return the tile at (x,z) in tile space, or null if the tile is out 
// of bounds
CMiniPatch* CTerrain::GetTile(int32_t x,int32_t z)
{
	if (x<0 || x>=int32_t(m_MapSize)-1) return 0;	
	if (z<0 || z>=int32_t(m_MapSize)-1) return 0;

	CPatch* patch=GetPatch(x/PATCH_SIZE,z/PATCH_SIZE);
	return &patch->m_MiniPatches[z%PATCH_SIZE][x%PATCH_SIZE];
}

float CTerrain::getExactGroundLevel( float x, float y ) const
{
	x /= (float)CELL_SIZE;
	y /= (float)CELL_SIZE;

	int xi = (int)floor( x );
	int yi = (int)floor( y );
	float xf = x - (float)xi;
	float yf = y - (float)yi;

	if( xi < 0 )
	{
		xi = 0; xf = 0.0f;
	}
	if( xi >= (int)m_MapSize )
	{
		xi = m_MapSize - 1; xf = 1.0f;
	}
	if( yi < 0 )
	{
		yi = 0; yf = 0.0f;
	}
	if( yi >= (int)m_MapSize )
	{
		yi = m_MapSize - 1; yf = 1.0f;
	}
	/*
	assert( isOnMap( x, y ) );

	if( !isOnMap( x, y ) )
		return 0.0f;
	*/

	float h00 = m_Heightmap[yi*m_MapSize + xi];
	float h01 = m_Heightmap[yi*m_MapSize + xi + m_MapSize];
	float h10 = m_Heightmap[yi*m_MapSize + xi + 1];
	float h11 = m_Heightmap[yi*m_MapSize + xi + m_MapSize + 1];
	return( HEIGHT_SCALE * ( ( 1 - yf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + yf * ( ( 1 - xf ) * h01 + xf * h11 ) ) );
}

///////////////////////////////////////////////////////////////////////////////
// Resize: resize this terrain to the given size (in patches per side)
void CTerrain::Resize(u32 size)
{
	if (size==m_MapSizePatches) {
		// inexplicable request to resize terrain to the same size .. ignore it
		return;
	}

	if (!m_Heightmap) {
		// not yet created a terrain; build a default terrain of the given size now
		Initialize(size,0);
		return;
	}

	// allocate data for new terrain
	u32 newMapSize=(size*PATCH_SIZE)+1;
	u16* newHeightmap=new u16[newMapSize*newMapSize];
	CPatch* newPatches=new CPatch[size*size];

	if (size>m_MapSizePatches) {
		// new map is bigger than old one - zero the heightmap so we don't get uninitialised
		// height data along the expanded edges
		memset(newHeightmap,0,newMapSize*newMapSize);
	}

	// now copy over rows of data
	u32 j;
  	u16* src=m_Heightmap;
	u16* dst=newHeightmap;
	u32 copysize=newMapSize>m_MapSize ? m_MapSize : newMapSize;
	for (j=0;j<copysize;j++) {
		memcpy(dst,src,copysize*sizeof(u16));
		dst+=copysize;
		src+=m_MapSize;
		if (newMapSize>m_MapSize) {
			// entend the last height to the end of the row
			for (u32 i=0;i<newMapSize-m_MapSize;i++) {
				*dst++=*(src-1);
			}
		}		
	}


	if (newMapSize>m_MapSize) {
		// copy over heights of the last row to any remaining rows
		src=newHeightmap+((m_MapSize-1)*newMapSize);
		dst=src+newMapSize;
		for (u32 i=0;i<newMapSize-m_MapSize;i++) {
			memcpy(dst,src,newMapSize*sizeof(u16));
			dst+=newMapSize;
		}
	}

	// now build new patches
	for (j=0;j<size;j++) {
		for (u32 i=0;i<size;i++) {
			// copy over texture data from existing tiles, if possible
			if (i<m_MapSizePatches && j<m_MapSizePatches) {
				memcpy(newPatches[j*size+i].m_MiniPatches,m_Patches[j*m_MapSizePatches+i].m_MiniPatches,sizeof(CMiniPatch)*PATCH_SIZE*PATCH_SIZE);
			} 
		}

		if (j<m_MapSizePatches && size>m_MapSizePatches) {
			// copy over the last tile from each column
			for (u32 n=0;n<size-m_MapSizePatches;n++) {
				for (int m=0;m<PATCH_SIZE;m++) {
					CMiniPatch& src=m_Patches[j*m_MapSizePatches+m_MapSizePatches-1].m_MiniPatches[m][15];
					for (int k=0;k<PATCH_SIZE;k++) {
						CMiniPatch& dst=newPatches[j*size+m_MapSizePatches+n].m_MiniPatches[m][k];
						dst.Tex1=src.Tex1;
						dst.Tex1Priority=src.Tex1Priority;
					}
				}
			}
		}
	}
	
	if (size>m_MapSizePatches) {
		// copy over the last tile from each column
		CPatch* srcpatch=&newPatches[(m_MapSizePatches-1)*size];
		CPatch* dstpatch=srcpatch+size;
		for (u32 p=0;p<size-m_MapSizePatches;p++) {
			for (u32 n=0;n<size;n++) {
				for (int m=0;m<PATCH_SIZE;m++) {
					for (int k=0;k<PATCH_SIZE;k++) {
						CMiniPatch& src=srcpatch->m_MiniPatches[15][k];
						CMiniPatch& dst=dstpatch->m_MiniPatches[m][k];
						dst.Tex1=src.Tex1;
						dst.Tex1Priority=src.Tex1Priority;
					}
				}
				srcpatch++;
				dstpatch++;
			}
		}
	}


	// release all the original data
	ReleaseData();

	// store new data
	m_Heightmap=newHeightmap;
	m_Patches=newPatches;
	m_MapSize=newMapSize;
	m_MapSizePatches=size;

	// initialise all the new patches
	InitialisePatches();
}

///////////////////////////////////////////////////////////////////////////////
// InitialisePatches: initialise patch data
void CTerrain::InitialisePatches()
{
	for (u32 j=0;j<m_MapSizePatches;j++) {
		for (u32 i=0;i<m_MapSizePatches;i++) {
			CPatch* patch=GetPatch(i,j);
			patch->Initialize(this,i,j);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// SetHeightMap: set up a new heightmap from 16-bit source data; 
// assumes heightmap matches current terrain size
void CTerrain::SetHeightMap(u16* heightmap)
{
	// keep a copy of the given heightmap
	memcpy(m_Heightmap,heightmap,m_MapSize*m_MapSize*sizeof(u16));

	// recalculate patch bounds, invalidate vertices
	for (u32 j=0;j<m_MapSizePatches;j++) {
		for (u32 i=0;i<m_MapSizePatches;i++) {
			CPatch* patch=GetPatch(i,j);
			patch->InvalidateBounds();
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// FlattenArea: flatten out an area of terrain (specified in world space 
// coords); return the average height of the flattened area
float CTerrain::FlattenArea(float x0,float x1,float z0,float z1)
{
	u32 tx0=u32(clamp(int(float(x0/CELL_SIZE)),0,int(m_MapSize)));	
	u32 tx1=u32(clamp(int(float(x1/CELL_SIZE)+1.0f),0,int(m_MapSize)));	
	u32 tz0=u32(clamp(int(float(z0/CELL_SIZE)),0,int(m_MapSize)));	
	u32 tz1=u32(clamp(int(float(z1/CELL_SIZE)+1.0f),0,int(m_MapSize)));	
	
	u32 count=0;
	u32 y=0;
	for (u32 x=tx0;x<=tx1;x++) {
		for (u32 z=tz0;z<=tz1;z++) {
			y+=m_Heightmap[z*m_MapSize + x];
			count++;
		}
	}
	y/=count;

	for (u32 x=tx0;x<=tx1;x++) {
		for (u32 z=tz0;z<=tz1;z++) {
			m_Heightmap[z*m_MapSize + x]=y;
			CPatch* patch=GetPatch(x/PATCH_SIZE,z/PATCH_SIZE);
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
		}
	}

	return y*HEIGHT_SCALE;
}

