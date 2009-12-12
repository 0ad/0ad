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

/*
 * Describes ground via heightmap and array of CPatch.
 */

#include "precompiled.h"

#include "lib/res/graphics/ogl_tex.h"

#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"

#include "simulation/Entity.h"

#include "TerrainProperties.h"
#include "TextureEntry.h"
#include "TextureManager.h"

#include <string.h>
#include "Terrain.h"
#include "Patch.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"

///////////////////////////////////////////////////////////////////////////////
// CTerrain constructor
CTerrain::CTerrain()
: m_Heightmap(0), m_Patches(0), m_MapSize(0), m_MapSizePatches(0),
m_BaseColour(255, 255, 255, 255)
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
// Initialise: initialise this terrain to the given size
// using given heightmap to setup elevation data
bool CTerrain::Initialize(ssize_t patchesPerSide,const u16* data)
{
	// clean up any previous terrain
	ReleaseData();

	// store terrain size
	m_MapSize=patchesPerSide*PATCH_SIZE+1;
	m_MapSizePatches=patchesPerSide;
	// allocate data for new terrain
	m_Heightmap=new u16[m_MapSize*m_MapSize];
	m_Patches=new CPatch[m_MapSizePatches*m_MapSizePatches];

	// given a heightmap?
	if (data) {
		// yes; keep a copy of it
		cpu_memcpy(m_Heightmap,data,m_MapSize*m_MapSize*sizeof(u16));
	} else {
		// build a flat terrain
		memset(m_Heightmap,0,m_MapSize*m_MapSize*sizeof(u16));
	}

	// setup patch parents, indices etc
	InitialisePatches();

	return true;
}

///////////////////////////////////////////////////////////////////////////////

float CTerrain::GetExactGroundLevel(const CVector2D& v) const
{
	return GetExactGroundLevel(v.x, v.y);
}

bool CTerrain::IsOnMap(const CVector2D& v) const
{
	return IsOnMap(v.x, v.y);
}

bool CTerrain::IsPassable(const CVector2D &loc/*tile space*/, HEntity entity) const
{
	CMiniPatch *pTile = GetTile(loc.x, loc.y);
	if(!pTile)
	{
		LOGWARNING(L"IsPassable: invalid coordinates %.1f %.1f\n", loc.x, loc.y);
		return false;
	}
	if(!pTile->Tex1)
		return false;		// Invalid terrain type in the scenario file
	CTextureEntry *pTexEntry = g_TexMan.FindTexture(pTile->Tex1);
	CTerrainPropertiesPtr pProperties = pTexEntry->GetProperties();
	if(!pProperties)
	{
		VfsPath texturePath = pTexEntry->GetTexturePath();
		LOGWARNING(L"IsPassable: no properties loaded for %ls\n", texturePath.string().c_str());
		return false;
	}
	return pProperties->IsPassable(entity);
}

///////////////////////////////////////////////////////////////////////////////
// CalcPosition: calculate the world space position of the vertex at (i,j)
void CTerrain::CalcPosition(ssize_t i, ssize_t j, CVector3D& pos) const
{
	u16 height;
	if ((size_t)i < (size_t)m_MapSize && (size_t)j < (size_t)m_MapSize) // will reject negative coordinates
		height = m_Heightmap[j*m_MapSize + i];
	else
		height = 0;
	pos.X = float(i*CELL_SIZE);
	pos.Y = float(height*HEIGHT_SCALE);
	pos.Z = float(j*CELL_SIZE);
}


///////////////////////////////////////////////////////////////////////////////
// CalcNormal: calculate the world space normal of the vertex at (i,j)
void CTerrain::CalcNormal(ssize_t i, ssize_t j, CVector3D& normal) const
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
	float nlen=normal.Length();
	if (nlen>0.00001f) normal*=1.0f/nlen;
}


///////////////////////////////////////////////////////////////////////////////
// GetPatch: return the patch at (i,j) in patch space, or null if the patch is
// out of bounds
CPatch* CTerrain::GetPatch(ssize_t i, ssize_t j) const
{
	// range check (invalid indices are passed in by the culling and
	// patch blend code because they iterate from 0..#patches and examine
	// neighbors without checking if they're already on the edge)
	if( (size_t)i >= (size_t)m_MapSizePatches || (size_t)j >= (size_t)m_MapSizePatches )
		return 0;

	return &m_Patches[(j*m_MapSizePatches)+i];
}


///////////////////////////////////////////////////////////////////////////////
// GetTile: return the tile at (i,j) in tile space, or null if the tile is out
// of bounds
CMiniPatch* CTerrain::GetTile(ssize_t i, ssize_t j) const
{
	// see comment above
	if( (size_t)i >= (size_t)(m_MapSize-1) || (size_t)j >= (size_t)(m_MapSize-1) )
		return 0;

	CPatch* patch=GetPatch(i/PATCH_SIZE, j/PATCH_SIZE);	// can't fail (due to above check)
	return &patch->m_MiniPatches[j%PATCH_SIZE][i%PATCH_SIZE];
}

float CTerrain::GetVertexGroundLevel(ssize_t i, ssize_t j) const
{
	i = clamp(i, (ssize_t)0, m_MapSize-1);
	j = clamp(j, (ssize_t)0, m_MapSize-1);
	return HEIGHT_SCALE * m_Heightmap[j*m_MapSize + i];
}

float CTerrain::GetSlope(float x, float z) const
{
	// Clamp to size-2 so we can use the tiles (xi,zi)-(xi+1,zi+1)
	const ssize_t xi = clamp((ssize_t)floor(x/CELL_SIZE), (ssize_t)0, m_MapSize-2);
	const ssize_t zi = clamp((ssize_t)floor(z/CELL_SIZE), (ssize_t)0, m_MapSize-2);

	float h00 = m_Heightmap[zi*m_MapSize + xi];
	float h01 = m_Heightmap[(zi+1)*m_MapSize + xi];
	float h10 = m_Heightmap[zi*m_MapSize + (xi+1)];
	float h11 = m_Heightmap[(zi+1)*m_MapSize + (xi+1)];
	
	// Difference of highest point from lowest point
	return std::max(std::max(h00, h01), std::max(h10, h11)) -
	       std::min(std::min(h00, h01), std::min(h10, h11));
}

CVector2D CTerrain::GetSlopeAngleFace( CEntity* entity ) const
{
	CVector2D ret;

	const float D = 0.1f;		// Amount to look forward to calculate the slope
	float x = entity->m_position.X;
	float z = entity->m_position.Z;

	// Get forward slope and use it as the x angle
	CVector2D d = entity->m_ahead.Normalize() * D;
	float dy = GetExactGroundLevel(x+d.x, z+d.y) - GetExactGroundLevel(x-d.x, z-d.y);
	ret.x = atan2(dy, 2*D);

	// Get sideways slope and use it as the y angle
	CVector2D d2(-d.y, d.x);
	float dy2 = GetExactGroundLevel(x+d2.x, z+d2.y) - GetExactGroundLevel(x-d2.x, z-d2.y);
	ret.y = atan2(dy2, 2*D);

	return ret;
}

float CTerrain::GetExactGroundLevel(float x, float z) const
{
	// Clamp to size-2 so we can use the tiles (xi,zi)-(xi+1,zi+1)
	const ssize_t xi = clamp((ssize_t)floor(x/CELL_SIZE), (ssize_t)0, m_MapSize-2);
	const ssize_t zi = clamp((ssize_t)floor(z/CELL_SIZE), (ssize_t)0, m_MapSize-2);

	const float xf = clamp(x/CELL_SIZE-xi, 0.0f, 1.0f);
	const float zf = clamp(z/CELL_SIZE-zi, 0.0f, 1.0f);

	float h00 = m_Heightmap[zi*m_MapSize + xi];
	float h01 = m_Heightmap[(zi+1)*m_MapSize + xi];
	float h10 = m_Heightmap[zi*m_MapSize + (xi+1)];
	float h11 = m_Heightmap[(zi+1)*m_MapSize + (xi+1)];
	// Linearly interpolate
	return (HEIGHT_SCALE * (
		(1 - zf) * ((1 - xf) * h00 + xf * h10)
		   + zf  * ((1 - xf) * h01 + xf * h11)));
}

///////////////////////////////////////////////////////////////////////////////
// Resize: resize this terrain to the given size (in patches per side)
void CTerrain::Resize(ssize_t size)
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
	ssize_t newMapSize=size*PATCH_SIZE+1;
	u16* newHeightmap=new u16[newMapSize*newMapSize];
	CPatch* newPatches=new CPatch[size*size];

	if (size>m_MapSizePatches) {
		// new map is bigger than old one - zero the heightmap so we don't get uninitialised
		// height data along the expanded edges
		memset(newHeightmap,0,newMapSize*newMapSize*sizeof(u16));
	}

	// now copy over rows of data
	u16* src=m_Heightmap;
	u16* dst=newHeightmap;
	ssize_t copysize=std::min(newMapSize, m_MapSize);
	for (ssize_t j=0;j<copysize;j++) {
		cpu_memcpy(dst,src,copysize*sizeof(u16));
		dst+=copysize;
		src+=m_MapSize;
		if (newMapSize>m_MapSize) {
			// extend the last height to the end of the row
			for (size_t i=0;i<newMapSize-(size_t)m_MapSize;i++) {
				*dst++=*(src-1);
			}
		}
	}


	if (newMapSize>m_MapSize) {
		// copy over heights of the last row to any remaining rows
		src=newHeightmap+((m_MapSize-1)*newMapSize);
		dst=src+newMapSize;
		for (ssize_t i=0;i<newMapSize-m_MapSize;i++) {
			cpu_memcpy(dst,src,newMapSize*sizeof(u16));
			dst+=newMapSize;
		}
	}

	// now build new patches
	for (ssize_t j=0;j<size;j++) {
		for (ssize_t i=0;i<size;i++) {
			// copy over texture data from existing tiles, if possible
			if (i<m_MapSizePatches && j<m_MapSizePatches) {
				cpu_memcpy(newPatches[j*size+i].m_MiniPatches,m_Patches[j*m_MapSizePatches+i].m_MiniPatches,sizeof(CMiniPatch)*PATCH_SIZE*PATCH_SIZE);
			}
		}

		if (j<m_MapSizePatches && size>m_MapSizePatches) {
			// copy over the last tile from each column
			for (ssize_t n=0;n<size-m_MapSizePatches;n++) {
				for (ssize_t m=0;m<PATCH_SIZE;m++) {
					CMiniPatch& src=m_Patches[j*m_MapSizePatches+m_MapSizePatches-1].m_MiniPatches[m][15];
					for (ssize_t k=0;k<PATCH_SIZE;k++) {
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
		for (ssize_t p=0;p<(ssize_t)size-m_MapSizePatches;p++) {
			for (ssize_t n=0;n<(ssize_t)size;n++) {
				for (ssize_t m=0;m<PATCH_SIZE;m++) {
					for (ssize_t k=0;k<PATCH_SIZE;k++) {
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
	m_MapSize=(ssize_t)newMapSize;
	m_MapSizePatches=(ssize_t)size;

	// initialise all the new patches
	InitialisePatches();
}

///////////////////////////////////////////////////////////////////////////////
// InitialisePatches: initialise patch data
void CTerrain::InitialisePatches()
{
	for (ssize_t j=0;j<m_MapSizePatches;j++) {
		for (ssize_t i=0;i<m_MapSizePatches;i++) {
			CPatch* patch=GetPatch(i,j);	// can't fail
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
	cpu_memcpy(m_Heightmap,heightmap,m_MapSize*m_MapSize*sizeof(u16));

	// recalculate patch bounds, invalidate vertices
	for (ssize_t j=0;j<m_MapSizePatches;j++) {
		for (ssize_t i=0;i<m_MapSizePatches;i++) {
			CPatch* patch=GetPatch(i,j);	// can't fail
			patch->InvalidateBounds();
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// FlattenArea: flatten out an area of terrain (specified in world space
// coords); return the average height of the flattened area
float CTerrain::FlattenArea(float x0, float x1, float z0, float z1)
{
	const ssize_t tx0 = clamp(ssize_t(x0/CELL_SIZE),   (ssize_t)0, m_MapSize-1);
	const ssize_t tx1 = clamp(ssize_t(x1/CELL_SIZE)+1, (ssize_t)0, m_MapSize-1);
	const ssize_t tz0 = clamp(ssize_t(z0/CELL_SIZE),   (ssize_t)0, m_MapSize-1);
	const ssize_t tz1 = clamp(ssize_t(z1/CELL_SIZE)+1, (ssize_t)0, m_MapSize-1);

	size_t count=0;
	double sum=0.0f;
	for (ssize_t z=tz0;z<=tz1;z++) {
		for (ssize_t x=tx0;x<=tx1;x++) {
			sum+=m_Heightmap[z*m_MapSize + x];
			count++;
		}
	}
	const u16 avgY = u16(sum/count);

	for (ssize_t z=tz0;z<=tz1;z++) {
		for (ssize_t x=tx0;x<=tx1;x++) {
			m_Heightmap[z*m_MapSize + x]=avgY;
			CPatch* patch=GetPatch(x/PATCH_SIZE,z/PATCH_SIZE);	// can't fail (x,z were clamped)
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
		}
	}

	return avgY*HEIGHT_SCALE;
}

///////////////////////////////////////////////////////////////////////////////

void CTerrain::MakeDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1, int dirtyFlags)
{
	// flag vertex data as dirty for affected patches, and rebuild bounds of these patches
	ssize_t pi0 = clamp((i0/PATCH_SIZE)-1, (ssize_t)0, m_MapSizePatches);
	ssize_t pi1 = clamp((i1/PATCH_SIZE)+1, (ssize_t)0, m_MapSizePatches);
	ssize_t pj0 = clamp((j0/PATCH_SIZE)-1, (ssize_t)0, m_MapSizePatches);
	ssize_t pj1 = clamp((j1/PATCH_SIZE)+1, (ssize_t)0, m_MapSizePatches);
	for (ssize_t j = pj0; j < pj1; j++) {
		for (ssize_t i = pi0; i < pi1; i++) {
			CPatch* patch = GetPatch(i,j);	// can't fail (i,j were clamped)
			if (dirtyFlags & RENDERDATA_UPDATE_VERTICES)
				patch->CalcBounds();
			patch->SetDirty(dirtyFlags);
		}
	}
}

void CTerrain::MakeDirty(int dirtyFlags)
{
	for (ssize_t j = 0; j < m_MapSizePatches; j++) {
		for (ssize_t i = 0; i < m_MapSizePatches; i++) {
			CPatch* patch = GetPatch(i,j);	// can't fail
			if (dirtyFlags & RENDERDATA_UPDATE_VERTICES)
				patch->CalcBounds();
			patch->SetDirty(dirtyFlags);
		}
	}
}
