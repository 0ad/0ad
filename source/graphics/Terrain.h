///////////////////////////////////////////////////////////////////////////////
//
// Name:		Terrain.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _TERRAIN_H
#define _TERRAIN_H

#include "Patch.h"
#include "Vector3D.h"
#include "Vector2D.h"

///////////////////////////////////////////////////////////////////////////////
// CTerrain: main terrain class; contains the heightmap describing elevation
// data, and the smaller subpatches that form the terrain
class CTerrain
{
public:
	CTerrain();
	~CTerrain();

	bool Initialize(u32 size,const u16* ptr);

	// return number of vertices along edge of the terrain
	u32 GetVerticesPerSide() const { return m_MapSize; }
	// return number of patches along edge of the terrain
	u32 GetPatchesPerSide() const { return m_MapSizePatches; }

	inline bool isOnMap( float x, float y ) const 
	{
		return( ( x >= 0.0f ) && ( x < (float)( (m_MapSize-1) * CELL_SIZE ) ) && ( y >= 0.0f ) && ( y < (float)( (m_MapSize-1) * CELL_SIZE ) ) );
	}
	inline bool isOnMap( const CVector2D& v ) const
	{
		return( ( v.x >= 0.0f ) && ( v.x < (float)( (m_MapSize-1) * CELL_SIZE ) ) && ( v.y >= 0.0f ) && ( v.y < (float)( (m_MapSize-1) * CELL_SIZE ) ) );
	}
	float getVertexGroundLevel( int x, int y ) const ;
	float getExactGroundLevel( float x, float y ) const ;
	inline float getExactGroundLevel( const CVector2D& v ) const { return( getExactGroundLevel( v.x, v.y ) ); }

	// resize this terrain such that each side has given number of patches
	void Resize(u32 size);

	// set up a new heightmap from 16 bit data; assumes heightmap matches current terrain size
	void SetHeightMap(u16* heightmap);
	// return a pointer to the heightmap
	u16* GetHeightMap() const { return m_Heightmap; }

	// get patch at given coordinates, expressed in patch-space; return 0 if
	// coordinates represent patch off the edge of the map
	CPatch* GetPatch(i32 x, i32 z) const; 
	// get tile at given coordinates, expressed in tile-space; return 0 if
	// coordinates represent tile off the edge of the map
	CMiniPatch* GetTile(i32 x, i32 z) const;

	// calculate the position of a given vertex
	void CalcPosition(i32 i, i32 j, CVector3D& pos) const;
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(const CVector3D& pos, i32& i, i32& j)
	{
		i = i32_from_float(pos.X/CELL_SIZE);
		j = i32_from_float(pos.Z/CELL_SIZE);
	}
	// calculate the vertex under a given position (rounding down coordinates)
	static void CalcFromPosition(float x, float y, i32& i, i32& j)
	{
		i = i32_from_float(x/CELL_SIZE);
		j = i32_from_float(y/CELL_SIZE);
	}
	// calculate the normal at a given vertex
	void CalcNormal(u32 i, u32 j, CVector3D& normal) const;

	// flatten out an area of terrain (specified in world space coords); return
	// the average height of the flattened area
	float FlattenArea(float x0,float x1,float z0,float z1);

	// mark a specific square of tiles as dirty - use this after modifying the heightmap
	void MakeDirty(int x0, int z0, int x1, int z1, int dirtyFlags);
	// mark the entire map as dirty
	void MakeDirty(int dirtyFlags);

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
};

#endif
