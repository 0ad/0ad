/* Copyright (C) 2015 Wildfire Games.
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
#include "lib/sysdep/cpu.h"

#include "renderer/Renderer.h"

#include "TerrainProperties.h"
#include "TerrainTextureEntry.h"
#include "TerrainTextureManager.h"

#include <string.h>
#include "Terrain.h"
#include "Patch.h"
#include "maths/FixedVector3D.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "simulation2/helpers/Pathfinding.h"

///////////////////////////////////////////////////////////////////////////////
// CTerrain constructor
CTerrain::CTerrain()
: m_Heightmap(0), m_Patches(0), m_MapSize(0), m_MapSizePatches(0),
m_BaseColor(255, 255, 255, 255)
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
	m_HeightMipmap.ReleaseData();

	delete[] m_Heightmap;
	delete[] m_Patches;
}


///////////////////////////////////////////////////////////////////////////////
// Initialise: initialise this terrain to the given size
// using given heightmap to setup elevation data
bool CTerrain::Initialize(ssize_t patchesPerSide, const u16* data)
{
	// clean up any previous terrain
	ReleaseData();

	// store terrain size
	m_MapSize = patchesPerSide*PATCH_SIZE+1;
	m_MapSizePatches = patchesPerSide;
	// allocate data for new terrain
	m_Heightmap = new u16[m_MapSize*m_MapSize];
	m_Patches = new CPatch[m_MapSizePatches*m_MapSizePatches];

	// given a heightmap?
	if (data)
	{
		// yes; keep a copy of it
		memcpy(m_Heightmap, data, m_MapSize*m_MapSize*sizeof(u16));
	}
	else
	{
		// build a flat terrain
		memset(m_Heightmap, 0, m_MapSize*m_MapSize*sizeof(u16));
	}

	// setup patch parents, indices etc
	InitialisePatches();

	// initialise mipmap
	m_HeightMipmap.Initialize(m_MapSize, m_Heightmap);

	return true;
}

///////////////////////////////////////////////////////////////////////////////

CStr8 CTerrain::GetMovementClass(ssize_t i, ssize_t j) const
{
	CMiniPatch* tile = GetTile(i, j);
	if (tile && tile->GetTextureEntry())
		return tile->GetTextureEntry()->GetProperties().GetMovementClass();

	return "default";
}

///////////////////////////////////////////////////////////////////////////////
// CalcPosition: calculate the world space position of the vertex at (i,j)
// If i,j is off the map, it acts as if the edges of the terrain are extended
// outwards to infinity
void CTerrain::CalcPosition(ssize_t i, ssize_t j, CVector3D& pos) const
{
	ssize_t hi = clamp(i, (ssize_t)0, m_MapSize-1);
	ssize_t hj = clamp(j, (ssize_t)0, m_MapSize-1);
	u16 height = m_Heightmap[hj*m_MapSize + hi];
	pos.X = float(i*TERRAIN_TILE_SIZE);
	pos.Y = float(height*HEIGHT_SCALE);
	pos.Z = float(j*TERRAIN_TILE_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
// CalcPositionFixed: calculate the world space position of the vertex at (i,j)
void CTerrain::CalcPositionFixed(ssize_t i, ssize_t j, CFixedVector3D& pos) const
{
	ssize_t hi = clamp(i, (ssize_t)0, m_MapSize-1);
	ssize_t hj = clamp(j, (ssize_t)0, m_MapSize-1);
	u16 height = m_Heightmap[hj*m_MapSize + hi];
	pos.X = fixed::FromInt(i) * (int)TERRAIN_TILE_SIZE;
	// fixed max value is 32767, but height is a u16, so divide by two to avoid overflow
	pos.Y = fixed::FromInt(height/ 2 ) / ((int)HEIGHT_UNITS_PER_METRE / 2);
	pos.Z = fixed::FromInt(j) * (int)TERRAIN_TILE_SIZE;
}


///////////////////////////////////////////////////////////////////////////////
// CalcNormal: calculate the world space normal of the vertex at (i,j)
void CTerrain::CalcNormal(ssize_t i, ssize_t j, CVector3D& normal) const
{
	CVector3D left, right, up, down;

	// Calculate normals of the four half-tile triangles surrounding this vertex:

	// get position of vertex where normal is being evaluated
	CVector3D basepos;
	CalcPosition(i, j, basepos);

	if (i > 0) {
		CalcPosition(i-1, j, left);
		left -= basepos;
		left.Normalize();
	}

	if (i < m_MapSize-1) {
		CalcPosition(i+1, j, right);
		right -= basepos;
		right.Normalize();
	}

	if (j > 0) {
		CalcPosition(i, j-1, up);
		up -= basepos;
		up.Normalize();
	}

	if (j < m_MapSize-1) {
		CalcPosition(i, j+1, down);
		down -= basepos;
		down.Normalize();
	}

	CVector3D n0 = up.Cross(left);
	CVector3D n1 = left.Cross(down);
	CVector3D n2 = down.Cross(right);
	CVector3D n3 = right.Cross(up);

	// Compute the mean of the normals
	normal = n0 + n1 + n2 + n3;
	float nlen=normal.Length();
	if (nlen>0.00001f) normal*=1.0f/nlen;
}

///////////////////////////////////////////////////////////////////////////////
// CalcNormalFixed: calculate the world space normal of the vertex at (i,j)
void CTerrain::CalcNormalFixed(ssize_t i, ssize_t j, CFixedVector3D& normal) const
{
	CFixedVector3D left, right, up, down;

	// Calculate normals of the four half-tile triangles surrounding this vertex:

	// get position of vertex where normal is being evaluated
	CFixedVector3D basepos;
	CalcPositionFixed(i, j, basepos);

	if (i > 0) {
		CalcPositionFixed(i-1, j, left);
		left -= basepos;
		left.Normalize();
	}

	if (i < m_MapSize-1) {
		CalcPositionFixed(i+1, j, right);
		right -= basepos;
		right.Normalize();
	}

	if (j > 0) {
		CalcPositionFixed(i, j-1, up);
		up -= basepos;
		up.Normalize();
	}

	if (j < m_MapSize-1) {
		CalcPositionFixed(i, j+1, down);
		down -= basepos;
		down.Normalize();
	}

	CFixedVector3D n0 = up.Cross(left);
	CFixedVector3D n1 = left.Cross(down);
	CFixedVector3D n2 = down.Cross(right);
	CFixedVector3D n3 = right.Cross(up);

	// Compute the mean of the normals
	normal = n0 + n1 + n2 + n3;
	normal.Normalize();
}

CVector3D CTerrain::CalcExactNormal(float x, float z) const
{
	// Clamp to size-2 so we can use the tiles (xi,zi)-(xi+1,zi+1)
	const ssize_t xi = clamp((ssize_t)floor(x/TERRAIN_TILE_SIZE), (ssize_t)0, m_MapSize-2);
	const ssize_t zi = clamp((ssize_t)floor(z/TERRAIN_TILE_SIZE), (ssize_t)0, m_MapSize-2);

	const float xf = clamp(x/TERRAIN_TILE_SIZE-xi, 0.0f, 1.0f);
	const float zf = clamp(z/TERRAIN_TILE_SIZE-zi, 0.0f, 1.0f);

	float h00 = m_Heightmap[zi*m_MapSize + xi];
	float h01 = m_Heightmap[(zi+1)*m_MapSize + xi];
	float h10 = m_Heightmap[zi*m_MapSize + (xi+1)];
	float h11 = m_Heightmap[(zi+1)*m_MapSize + (xi+1)];

	// Determine which terrain triangle this point is on,
	// then compute the normal of that triangle's plane

	if (GetTriangulationDir(xi, zi))
	{
		if (xf + zf <= 1.f)
		{
			// Lower-left triangle (don't use h11)
			return -CVector3D(TERRAIN_TILE_SIZE, (h10-h00)*HEIGHT_SCALE, 0).Cross(CVector3D(0, (h01-h00)*HEIGHT_SCALE, TERRAIN_TILE_SIZE)).Normalized();
		}
		else
		{
			// Upper-right triangle (don't use h00)
			return -CVector3D(TERRAIN_TILE_SIZE, (h11-h01)*HEIGHT_SCALE, 0).Cross(CVector3D(0, (h11-h10)*HEIGHT_SCALE, TERRAIN_TILE_SIZE)).Normalized();
		}
	}
	else
	{
		if (xf <= zf)
		{
			// Upper-left triangle (don't use h10)
			return -CVector3D(TERRAIN_TILE_SIZE, (h11-h01)*HEIGHT_SCALE, 0).Cross(CVector3D(0, (h01-h00)*HEIGHT_SCALE, TERRAIN_TILE_SIZE)).Normalized();
		}
		else
		{
			// Lower-right triangle (don't use h01)
			return -CVector3D(TERRAIN_TILE_SIZE, (h10-h00)*HEIGHT_SCALE, 0).Cross(CVector3D(0, (h11-h10)*HEIGHT_SCALE, TERRAIN_TILE_SIZE)).Normalized();
		}
	}
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

fixed CTerrain::GetVertexGroundLevelFixed(ssize_t i, ssize_t j) const
{
	i = clamp(i, (ssize_t)0, m_MapSize-1);
	j = clamp(j, (ssize_t)0, m_MapSize-1);
	// Convert to fixed metres (being careful to avoid intermediate overflows)
	return fixed::FromInt(m_Heightmap[j*m_MapSize + i] / 2) / (int)(HEIGHT_UNITS_PER_METRE / 2);
}

fixed CTerrain::GetSlopeFixed(ssize_t i, ssize_t j) const
{
	// Clamp to size-2 so we can use the tiles (i,j)-(i+1,j+1)
	i = clamp(i, (ssize_t)0, m_MapSize-2);
	j = clamp(j, (ssize_t)0, m_MapSize-2);

	u16 h00 = m_Heightmap[j*m_MapSize + i];
	u16 h01 = m_Heightmap[(j+1)*m_MapSize + i];
	u16 h10 = m_Heightmap[j*m_MapSize + (i+1)];
	u16 h11 = m_Heightmap[(j+1)*m_MapSize + (i+1)];
	
	// Difference of highest point from lowest point
	u16 delta = std::max(std::max(h00, h01), std::max(h10, h11)) -
	            std::min(std::min(h00, h01), std::min(h10, h11));

	// Compute fractional slope (being careful to avoid intermediate overflows)
	return fixed::FromInt(delta / TERRAIN_TILE_SIZE) / (int)HEIGHT_UNITS_PER_METRE;
}

fixed CTerrain::GetExactSlopeFixed(fixed x, fixed z) const
{
	// Clamp to size-2 so we can use the tiles (xi,zi)-(xi+1,zi+1)
	const ssize_t xi = clamp((ssize_t)(x / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), (ssize_t)0, m_MapSize-2);
	const ssize_t zi = clamp((ssize_t)(z / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), (ssize_t)0, m_MapSize-2);

	const fixed one = fixed::FromInt(1);

	const fixed xf = clamp((x / (int)TERRAIN_TILE_SIZE) - fixed::FromInt(xi), fixed::Zero(), one);
	const fixed zf = clamp((z / (int)TERRAIN_TILE_SIZE) - fixed::FromInt(zi), fixed::Zero(), one);

	u16 h00 = m_Heightmap[zi*m_MapSize + xi];
	u16 h01 = m_Heightmap[(zi+1)*m_MapSize + xi];
	u16 h10 = m_Heightmap[zi*m_MapSize + (xi+1)];
	u16 h11 = m_Heightmap[(zi+1)*m_MapSize + (xi+1)];

	u16 delta;
	if (GetTriangulationDir(xi, zi))
	{
		if (xf + zf <= one)
		{
			// Lower-left triangle (don't use h11)
			delta = std::max(std::max(h00, h01), h10) -
			        std::min(std::min(h00, h01), h10);
		}
		else
		{
			// Upper-right triangle (don't use h00)
			delta = std::max(std::max(h01, h10), h11) -
			        std::min(std::min(h01, h10), h11);
		}
	}
	else
	{
		if (xf <= zf)
		{
			// Upper-left triangle (don't use h10)
			delta = std::max(std::max(h00, h01), h11) -
			        std::min(std::min(h00, h01), h11);
		}
		else
		{
			// Lower-right triangle (don't use h01)
			delta = std::max(std::max(h00, h10), h11) -
			        std::min(std::min(h00, h10), h11);
		}
	}

	// Compute fractional slope (being careful to avoid intermediate overflows)
	return fixed::FromInt(delta / TERRAIN_TILE_SIZE) / (int)HEIGHT_UNITS_PER_METRE;
}

float CTerrain::GetFilteredGroundLevel(float x, float z, float radius) const
{
	// convert to [0,1] interval
	float nx = x / (TERRAIN_TILE_SIZE*m_MapSize);
	float nz = z / (TERRAIN_TILE_SIZE*m_MapSize);
	float nr = radius / (TERRAIN_TILE_SIZE*m_MapSize);

	// get trilinear filtered mipmap height
	return HEIGHT_SCALE * m_HeightMipmap.GetTrilinearGroundLevel(nx, nz, nr);
}

float CTerrain::GetExactGroundLevel(float x, float z) const
{
	// Clamp to size-2 so we can use the tiles (xi,zi)-(xi+1,zi+1)
	const ssize_t xi = clamp((ssize_t)floor(x/TERRAIN_TILE_SIZE), (ssize_t)0, m_MapSize-2);
	const ssize_t zi = clamp((ssize_t)floor(z/TERRAIN_TILE_SIZE), (ssize_t)0, m_MapSize-2);

	const float xf = clamp(x/TERRAIN_TILE_SIZE-xi, 0.0f, 1.0f);
	const float zf = clamp(z/TERRAIN_TILE_SIZE-zi, 0.0f, 1.0f);

	float h00 = m_Heightmap[zi*m_MapSize + xi];
	float h01 = m_Heightmap[(zi+1)*m_MapSize + xi];
	float h10 = m_Heightmap[zi*m_MapSize + (xi+1)];
	float h11 = m_Heightmap[(zi+1)*m_MapSize + (xi+1)];

	// Determine which terrain triangle this point is on,
	// then compute the linearly-interpolated height on that triangle's plane

	if (GetTriangulationDir(xi, zi))
	{
		if (xf + zf <= 1.f)
		{
			// Lower-left triangle (don't use h11)
			return HEIGHT_SCALE * (h00 + (h10-h00)*xf + (h01-h00)*zf);
		}
		else
		{
			// Upper-right triangle (don't use h00)
			return HEIGHT_SCALE * (h11 + (h01-h11)*(1-xf) + (h10-h11)*(1-zf));
		}
	}
	else
	{
		if (xf <= zf)
		{
			// Upper-left triangle (don't use h10)
			return HEIGHT_SCALE * (h00 + (h11-h01)*xf + (h01-h00)*zf);
		}
		else
		{
			// Lower-right triangle (don't use h01)
			return HEIGHT_SCALE * (h00 + (h10-h00)*xf + (h11-h10)*zf);
		}
	}
}

fixed CTerrain::GetExactGroundLevelFixed(fixed x, fixed z) const
{
	// Clamp to size-2 so we can use the tiles (xi,zi)-(xi+1,zi+1)
	const ssize_t xi = clamp((ssize_t)(x / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), (ssize_t)0, m_MapSize-2);
	const ssize_t zi = clamp((ssize_t)(z / (int)TERRAIN_TILE_SIZE).ToInt_RoundToZero(), (ssize_t)0, m_MapSize-2);

	const fixed one = fixed::FromInt(1);

	const fixed xf = clamp((x / (int)TERRAIN_TILE_SIZE) - fixed::FromInt(xi), fixed::Zero(), one);
	const fixed zf = clamp((z / (int)TERRAIN_TILE_SIZE) - fixed::FromInt(zi), fixed::Zero(), one);

	u16 h00 = m_Heightmap[zi*m_MapSize + xi];
	u16 h01 = m_Heightmap[(zi+1)*m_MapSize + xi];
	u16 h10 = m_Heightmap[zi*m_MapSize + (xi+1)];
	u16 h11 = m_Heightmap[(zi+1)*m_MapSize + (xi+1)];

	// Intermediate scaling of xf, so we don't overflow in the multiplications below
	// (h00 <= 65535, xf <= 1, max fixed is < 32768; divide by 2 here so xf1*h00 <= 32767.5)
	const fixed xf0 = xf / 2;
	const fixed xf1 = (one - xf) / 2;

	// Linearly interpolate
	return ((one - zf).Multiply(xf1 * h00 + xf0 * h10)
	              + zf.Multiply(xf1 * h01 + xf0 * h11)) / (int)(HEIGHT_UNITS_PER_METRE / 2);

	// TODO: This should probably be more like GetExactGroundLevel()
	// in handling triangulation properly
}

bool CTerrain::GetTriangulationDir(ssize_t i, ssize_t j) const
{
	// Clamp to size-2 so we can use the tiles (i,j)-(i+1,j+1)
	i = clamp(i, (ssize_t)0, m_MapSize-2);
	j = clamp(j, (ssize_t)0, m_MapSize-2);

	int h00 = m_Heightmap[j*m_MapSize + i];
	int h01 = m_Heightmap[(j+1)*m_MapSize + i];
	int h10 = m_Heightmap[j*m_MapSize + (i+1)];
	int h11 = m_Heightmap[(j+1)*m_MapSize + (i+1)];

	// Prefer triangulating in whichever direction means the midpoint of the diagonal
	// will be the highest. (In particular this means a diagonal edge will be straight
	// along the top, and jagged along the bottom, which makes sense for terrain.)
	int mid1 = h00+h11;
	int mid2 = h01+h10;
	return (mid1 < mid2);
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
		memcpy(dst,src,copysize*sizeof(u16));
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
			memcpy(dst,src,newMapSize*sizeof(u16));
			dst+=newMapSize;
		}
	}

	// now build new patches
	for (ssize_t j=0;j<size;j++) {
		for (ssize_t i=0;i<size;i++) {
			// copy over texture data from existing tiles, if possible
			if (i<m_MapSizePatches && j<m_MapSizePatches) {
				memcpy(newPatches[j*size+i].m_MiniPatches,m_Patches[j*m_MapSizePatches+i].m_MiniPatches,sizeof(CMiniPatch)*PATCH_SIZE*PATCH_SIZE);
			}
		}

		if (j<m_MapSizePatches && size>m_MapSizePatches) {
			// copy over the last tile from each column
			for (ssize_t n=0;n<size-m_MapSizePatches;n++) {
				for (ssize_t m=0;m<PATCH_SIZE;m++) {
					CMiniPatch& src=m_Patches[j*m_MapSizePatches+m_MapSizePatches-1].m_MiniPatches[m][15];
					for (ssize_t k=0;k<PATCH_SIZE;k++) {
						CMiniPatch& dst=newPatches[j*size+m_MapSizePatches+n].m_MiniPatches[m][k];
						dst = src;
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
						dst = src;
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

	// initialise mipmap
	m_HeightMipmap.Initialize(m_MapSize,m_Heightmap);
}

///////////////////////////////////////////////////////////////////////////////
// InitialisePatches: initialise patch data
void CTerrain::InitialisePatches()
{
	for (ssize_t j = 0; j < m_MapSizePatches; j++)
	{
		for (ssize_t i = 0; i < m_MapSizePatches; i++)
		{
			CPatch* patch = GetPatch(i, j);	// can't fail
			patch->Initialize(this, i, j);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// SetHeightMap: set up a new heightmap from 16-bit source data;
// assumes heightmap matches current terrain size
void CTerrain::SetHeightMap(u16* heightmap)
{
	// keep a copy of the given heightmap
	memcpy(m_Heightmap, heightmap, m_MapSize*m_MapSize*sizeof(u16));

	// recalculate patch bounds, invalidate vertices
	for (ssize_t j = 0; j < m_MapSizePatches; j++)
	{
		for (ssize_t i = 0; i < m_MapSizePatches; i++)
		{
			CPatch* patch = GetPatch(i, j);	// can't fail
			patch->InvalidateBounds();
			patch->SetDirty(RENDERDATA_UPDATE_VERTICES);
		}
	}

	// update mipmap
	m_HeightMipmap.Update(m_Heightmap);
}


///////////////////////////////////////////////////////////////////////////////

void CTerrain::MakeDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1, int dirtyFlags)
{
	// Finds the inclusive limits of the patches that include the specified range of tiles
	ssize_t pi0 = clamp( i0   /PATCH_SIZE, (ssize_t)0, m_MapSizePatches-1);
	ssize_t pi1 = clamp((i1-1)/PATCH_SIZE, (ssize_t)0, m_MapSizePatches-1);
	ssize_t pj0 = clamp( j0   /PATCH_SIZE, (ssize_t)0, m_MapSizePatches-1);
	ssize_t pj1 = clamp((j1-1)/PATCH_SIZE, (ssize_t)0, m_MapSizePatches-1);

	for (ssize_t j = pj0; j <= pj1; j++)
	{
		for (ssize_t i = pi0; i <= pi1; i++)
		{
			CPatch* patch = GetPatch(i, j);	// can't fail (i,j were clamped)
			if (dirtyFlags & RENDERDATA_UPDATE_VERTICES)
				patch->CalcBounds();
			patch->SetDirty(dirtyFlags);
		}
	}

	if (m_Heightmap)
	{
		m_HeightMipmap.Update(m_Heightmap,
			clamp(i0, (ssize_t)0, m_MapSize-1),
			clamp(j0, (ssize_t)0, m_MapSize-1),
			clamp(i1, (ssize_t)1, m_MapSize),
			clamp(j1, (ssize_t)1, m_MapSize)
		);
	}
}

void CTerrain::MakeDirty(int dirtyFlags)
{
	for (ssize_t j = 0; j < m_MapSizePatches; j++)
	{
		for (ssize_t i = 0; i < m_MapSizePatches; i++)
		{
			CPatch* patch = GetPatch(i, j);	// can't fail
			if (dirtyFlags & RENDERDATA_UPDATE_VERTICES)
				patch->CalcBounds();
			patch->SetDirty(dirtyFlags);
		}
	}

	if (m_Heightmap)
		m_HeightMipmap.Update(m_Heightmap);
}

CBoundingBoxAligned CTerrain::GetVertexesBound(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1)
{
	i0 = clamp(i0, (ssize_t)0, m_MapSize-1);
	j0 = clamp(j0, (ssize_t)0, m_MapSize-1);
	i1 = clamp(i1, (ssize_t)0, m_MapSize-1);
	j1 = clamp(j1, (ssize_t)0, m_MapSize-1);

	u16 minH = 65535;
	u16 maxH = 0;

	for (ssize_t j = j0; j <= j1; ++j)
	{
		for (ssize_t i = i0; i <= i1; ++i)
		{
			minH = std::min(minH, m_Heightmap[j*m_MapSize + i]);
			maxH = std::max(maxH, m_Heightmap[j*m_MapSize + i]);
		}
	}

	CBoundingBoxAligned bound;
	bound[0].X = (float)(i0*TERRAIN_TILE_SIZE);
	bound[0].Y = (float)(minH*HEIGHT_SCALE);
	bound[0].Z = (float)(j0*TERRAIN_TILE_SIZE);
	bound[1].X = (float)(i1*TERRAIN_TILE_SIZE);
	bound[1].Y = (float)(maxH*HEIGHT_SCALE);
	bound[1].Z = (float)(j1*TERRAIN_TILE_SIZE);
	return bound;
}
