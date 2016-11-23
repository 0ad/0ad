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

/*
 * Describes ground via heightmap and array of CPatch.
 */

#ifndef INCLUDED_TERRAIN
#define INCLUDED_TERRAIN

#include "maths/Vector3D.h"
#include "maths/Fixed.h"
#include "graphics/SColor.h"
#include "graphics/HeightMipmap.h"

class CPatch;
class CMiniPatch;
class CFixedVector3D;
class CStr8;
class CBoundingBoxAligned;

///////////////////////////////////////////////////////////////////////////////
// Terrain Constants:

/// metres [world space units] per tile in x and z
const ssize_t TERRAIN_TILE_SIZE = 4;

/// number of u16 height units per metre
const ssize_t HEIGHT_UNITS_PER_METRE = 92;

/// metres per u16 height unit
const float HEIGHT_SCALE = 1.f / HEIGHT_UNITS_PER_METRE;

///////////////////////////////////////////////////////////////////////////////
// CTerrain: main terrain class; contains the heightmap describing elevation
// data, and the smaller subpatches that form the terrain
class CTerrain
{
public:
	CTerrain();
	~CTerrain();

	// Coordinate naming convention: world-space coordinates are float x,z;
	// tile-space coordinates are ssize_t i,j. rationale: signed types can
	// more efficiently be converted to/from floating point. use ssize_t
	// instead of int/long because these are sizes.

	bool Initialize(ssize_t patchesPerSide, const u16* ptr);

	// return number of vertices along edge of the terrain
	ssize_t GetVerticesPerSide() const { return m_MapSize; }
	// return number of tiles along edge of the terrain
	ssize_t GetTilesPerSide() const { return GetVerticesPerSide()-1; }
	// return number of patches along edge of the terrain
	ssize_t GetPatchesPerSide() const { return m_MapSizePatches; }

	float GetMinX() const { return 0.0f; }
	float GetMinZ() const { return 0.0f; }
	float GetMaxX() const { return (float)((m_MapSize-1) * TERRAIN_TILE_SIZE); }
	float GetMaxZ() const { return (float)((m_MapSize-1) * TERRAIN_TILE_SIZE); }

	bool IsOnMap(float x, float z) const
	{
		return ((x >= GetMinX()) && (x < GetMaxX())
		     && (z >= GetMinZ()) && (z < GetMaxZ()));
	}

	CStr8 GetMovementClass(ssize_t i, ssize_t j) const;

	float GetVertexGroundLevel(ssize_t i, ssize_t j) const;
	fixed GetVertexGroundLevelFixed(ssize_t i, ssize_t j) const;
	float GetExactGroundLevel(float x, float z) const;
	fixed GetExactGroundLevelFixed(fixed x, fixed z) const;
	float GetFilteredGroundLevel(float x, float z, float radius) const;

	// get the approximate slope of a tile
	// (0 = horizontal, 0.5 = 30 degrees, 1.0 = 45 degrees, etc)
	fixed GetSlopeFixed(ssize_t i, ssize_t j) const;

	// get the precise slope of a point, accounting for triangulation direction
	fixed GetExactSlopeFixed(fixed x, fixed z) const;

	// Returns true if the triangulation diagonal for tile (i, j)
	// should be in the direction (1,-1); false if it should be (1,1)
	bool GetTriangulationDir(ssize_t i, ssize_t j) const;

	// resize this terrain such that each side has given number of patches
	void Resize(ssize_t size);

	// set up a new heightmap from 16 bit data; assumes heightmap matches current terrain size
	void SetHeightMap(u16* heightmap);
	// return a pointer to the heightmap
	u16* GetHeightMap() const { return m_Heightmap; }

	// get patch at given coordinates, expressed in patch-space; return 0 if
	// coordinates represent patch off the edge of the map
	CPatch* GetPatch(ssize_t i, ssize_t j) const;
	// get tile at given coordinates, expressed in tile-space; return 0 if
	// coordinates represent tile off the edge of the map
	CMiniPatch* GetTile(ssize_t i, ssize_t j) const;

	// calculate the position of a given vertex
	void CalcPosition(ssize_t i, ssize_t j, CVector3D& pos) const;
	void CalcPositionFixed(ssize_t i, ssize_t j, CFixedVector3D& pos) const;
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(const CVector3D& pos, ssize_t& i, ssize_t& j)
	{
		i = (ssize_t)(pos.X/TERRAIN_TILE_SIZE);
		j = (ssize_t)(pos.Z/TERRAIN_TILE_SIZE);
	}
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(float x, float z, ssize_t& i, ssize_t& j)
	{
		i = (ssize_t)(x/TERRAIN_TILE_SIZE);
		j = (ssize_t)(z/TERRAIN_TILE_SIZE);
	}
	// calculate the normal at a given vertex
	void CalcNormal(ssize_t i, ssize_t j, CVector3D& normal) const;
	void CalcNormalFixed(ssize_t i, ssize_t j, CFixedVector3D& normal) const;

	CVector3D CalcExactNormal(float x, float z) const;

	// Mark a specific square of tiles (inclusive lower bound, exclusive upper bound)
	// as dirty - use this after modifying the heightmap.
	// If you modify a vertex (i,j), you should dirty tiles
	// from (i-1, j-1) [inclusive] to (i+1, j+1) [exclusive]
	// since their geometry depends on that vertex.
	// If you modify a tile (i,j), you should dirty tiles
	// from (i-1, j-1) [inclusive] to (i+2, j+2) [exclusive]
	// since their texture blends depend on that tile.
	void MakeDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1, int dirtyFlags);
	// mark the entire map as dirty
	void MakeDirty(int dirtyFlags);

	/**
	 * Returns a 3D bounding box encompassing the given vertex range (inclusive)
	 */
	CBoundingBoxAligned GetVertexesBound(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1);

	// get the base color for the terrain (typically pure white - other colors
	// will interact badly with LOS - but used by the Actor Viewer tool)
	SColor4ub GetBaseColor() const { return m_BaseColor; }
	// set the base color for the terrain
	void SetBaseColor(SColor4ub color) { m_BaseColor = color; }

	const CHeightMipmap& GetHeightMipmap() const { return m_HeightMipmap; }

private:
	// delete any data allocated by this terrain
	void ReleaseData();
	// setup patch pointers etc
	void InitialisePatches();

	// size of this map in each direction, in vertices; ie. total tiles = sqr(m_MapSize-1)
	ssize_t m_MapSize;
	// size of this map in each direction, in patches; total patches = sqr(m_MapSizePatches)
	ssize_t m_MapSizePatches;
	// the patches comprising this terrain
	CPatch*	m_Patches;
	// 16-bit heightmap data
	u16* m_Heightmap;
	// base color (usually white)
	SColor4ub m_BaseColor;
	// heightmap mipmap
	CHeightMipmap m_HeightMipmap;
};

#endif
