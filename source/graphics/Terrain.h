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
	u32 GetVerticesPerSide() { return m_MapSize; }
	// return number of patches along edge of the terrain
	u32 GetPatchesPerSide() { return m_MapSizePatches; }

	inline bool isOnMap( float x, float y ) const 
	{
		return( ( x >= 0.0f ) && ( x <= (float)m_MapSize ) && ( y >= 0.0f ) && ( y <= (float)m_MapSize ) );
	}
	inline bool isOnMap( const CVector2D& v ) const
	{
		return( ( v.x >= 0.0f ) && ( v.x <= (float)m_MapSize ) && ( v.y >= 0.0f ) && ( v.y <= (float)m_MapSize ) );
	}
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
	CPatch* GetPatch(int32_t x,int32_t z); 
	// get tile at given coordinates, expressed in tile-space; return 0 if
	// coordinates represent tile off the edge of the map
	CMiniPatch* GetTile(int32_t x,int32_t z);

	// calculate the position of a given vertex
	void CalcPosition(u32 i,u32 j,CVector3D& pos);
	// calculate the normal at a given vertex
	void CalcNormal(u32 i,u32 j,CVector3D& normal);

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

extern CTerrain g_Terrain;

#endif
