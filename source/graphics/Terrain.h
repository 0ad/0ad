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

#ifndef INCLUDED_TERRAIN
#define INCLUDED_TERRAIN

#include "maths/Vector3D.h"
#include "graphics/SColor.h"
#include "lib/sysdep/cpu.h"

class HEntity;
class CEntity;
class CPatch;
class CMiniPatch;
class CVector2D;


///////////////////////////////////////////////////////////////////////////////
// Terrain Constants:
//
// CELL_SIZE: size of each tile in x and z
const ssize_t CELL_SIZE = 4;	
// HEIGHT_SCALE: vertical scale of terrain - terrain has a coordinate range of 
// 0 to 65536*HEIGHT_SCALE
const float HEIGHT_SCALE = 0.35f/256.0f;

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

	bool IsOnMap(float x, float z) const
	{
		return ((x >= 0.0f) && (x < (float)((m_MapSize-1) * CELL_SIZE))
		     && (z >= 0.0f) && (z < (float)((m_MapSize-1) * CELL_SIZE)));
	}

	bool IsOnMap(const CVector2D& v) const;

	bool IsPassable(const CVector2D& tileSpaceLoc, HEntity entity) const;

	float GetVertexGroundLevel(ssize_t i, ssize_t j) const;
	float GetExactGroundLevel(float x, float z) const;
	float GetExactGroundLevel(const CVector2D& v) const;

	float GetSlope(float x, float z) const ;
	//Find the slope of in X and Z axes depending on the way the entity is facing
	CVector2D GetSlopeAngleFace(CEntity* entity) const;	
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
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(const CVector3D& pos, ssize_t& i, ssize_t& j)
	{
		i = (ssize_t)(pos.X/CELL_SIZE);
		j = (ssize_t)(pos.Z/CELL_SIZE);
	}
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(float x, float z, ssize_t& i, ssize_t& j)
	{
		i = (ssize_t)(x/CELL_SIZE);
		j = (ssize_t)(z/CELL_SIZE);
	}
	// calculate the normal at a given vertex
	void CalcNormal(ssize_t i, ssize_t j, CVector3D& normal) const;

	// flatten out an area of terrain (specified in world space coords); return
	// the average height of the flattened area
	float FlattenArea(float x0, float x1, float z0, float z1);

	// mark a specific square of tiles as dirty - use this after modifying the heightmap
	void MakeDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1, int dirtyFlags);
	// mark the entire map as dirty
	void MakeDirty(int dirtyFlags);

	// get the base colour for the terrain (typically pure white - other colours
	// will interact badly with LOS - but used by the Actor Viewer tool)
	SColor4ub GetBaseColour() const { return m_BaseColour; }
	// set the base colour for the terrain
	void SetBaseColour(SColor4ub colour) { m_BaseColour = colour; }

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
	// base colour (usually white)
	SColor4ub m_BaseColour;
};

#endif
