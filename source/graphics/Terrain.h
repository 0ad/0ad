/**
 * =========================================================================
 * File        : Terrain.h
 * Project     : 0 A.D.
 * Description : Describes ground via heightmap and array of CPatch.
 * =========================================================================
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
const int	CELL_SIZE = 4;	
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
	// tile-space coordinates are int i,j.

	bool Initialize(u32 size, const u16* ptr);

	// return number of vertices along edge of the terrain
	u32 GetVerticesPerSide() const { return m_MapSize; }
	// return number of tiles along edge of the terrain
	u32 GetTilesPerSide() const { return GetVerticesPerSide()-1; }
	// return number of patches along edge of the terrain
	u32 GetPatchesPerSide() const { return m_MapSizePatches; }

	bool IsOnMap(float x, float z) const
	{
		return ((x >= 0.0f) && (x < (float)((m_MapSize-1) * CELL_SIZE))
		     && (z >= 0.0f) && (z < (float)((m_MapSize-1) * CELL_SIZE)));
	}

	bool IsOnMap(const CVector2D& v) const;

	bool IsPassable(const CVector2D& tileSpaceLoc, HEntity entity) const;

	void ClampCoordToMap(int& index) const
	{
		if(index < 0)
			index = 0;
		else if(index >= (int)m_MapSize-1)
			index = m_MapSize - 2;
	}

	float GetVertexGroundLevel(int i, int j) const;
	float GetExactGroundLevel(float x, float z) const;
	float GetExactGroundLevel(const CVector2D& v) const;

	float GetSlope(float x, float z) const ;
	//Find the slope of in X and Z axes depending on the way the entity is facing
	CVector2D GetSlopeAngleFace(CEntity* entity) const;	
	// resize this terrain such that each side has given number of patches
	void Resize(u32 size);

	// set up a new heightmap from 16 bit data; assumes heightmap matches current terrain size
	void SetHeightMap(u16* heightmap);
	// return a pointer to the heightmap
	u16* GetHeightMap() const { return m_Heightmap; }

	// get patch at given coordinates, expressed in patch-space; return 0 if
	// coordinates represent patch off the edge of the map
	CPatch* GetPatch(i32 i, i32 j) const; 
	// get tile at given coordinates, expressed in tile-space; return 0 if
	// coordinates represent tile off the edge of the map
	CMiniPatch* GetTile(i32 i, i32 j) const;

	// calculate the position of a given vertex
	void CalcPosition(i32 i, i32 j, CVector3D& pos) const;
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(const CVector3D& pos, i32& i, i32& j)
	{
		i = cpu_i32FromFloat(pos.X/CELL_SIZE);
		j = cpu_i32FromFloat(pos.Z/CELL_SIZE);
	}
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(float x, float z, i32& i, i32& j)
	{
		i = cpu_i32FromFloat(x/CELL_SIZE);
		j = cpu_i32FromFloat(z/CELL_SIZE);
	}
	// calculate the normal at a given vertex
	void CalcNormal(u32 i, u32 j, CVector3D& normal) const;

	// flatten out an area of terrain (specified in world space coords); return
	// the average height of the flattened area
	float FlattenArea(float x0, float x1, float z0, float z1);

	// mark a specific square of tiles as dirty - use this after modifying the heightmap
	void MakeDirty(int i0, int j0, int i1, int j1, int dirtyFlags);
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
	u32 m_MapSize;
	// size of this map in each direction, in patches; total patches = sqr(m_MapSizePatches)
	u32 m_MapSizePatches;
	// the patches comprising this terrain
	CPatch*	m_Patches;
	// 16-bit heightmap data
	u16* m_Heightmap;
	// base colour (usually white)
	SColor4ub m_BaseColour;
};

#endif
