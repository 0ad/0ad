//***********************************************************
//
// Name:		Terrain.h
// Last Update: 23/2/02
// Author:		Poya Manouchehri
//
// Description: CTerrain handles the terrain portion of the
//				engine. It holds open the file to the terrain
//				information, so terrain data can be loaded
//				dynamically. We use a ROAM method to render 
//				the terrain, ie using binary triangle trees.
//				The terrain consists of smaller PATCHS, which
//				do most of the work.
//
//***********************************************************

#ifndef TERRAIN_H
#define TERRAIN_H

#include "Patch.h"
#include "Vector3D.h"
#include "TerrGlobals.h"

class CLightEnv;
class CSHCoeffs;


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

	// resize this terrain such that each side has given number of patches
	void Resize(u32 size);

	// set up a new heightmap from 16 bit data; assumes heightmap matches current terrain size
	void SetHeightMap(u16* heightmap);
	// return a pointer to the heightmap
	u16* GetHeightMap() const { return m_Heightmap; }

	// get patch at given coordinates, expressed in patch-space; return 0 if
	// coordinates represent patch off the edge of the map
	CPatch* GetPatch(int32 x,int32 z); 
	// get tile at given coordinates, expressed in tile-space; return 0 if
	// coordinates represent tile off the edge of the map
	CMiniPatch* GetTile(int32 x,int32 z);

	// calculate the position of a given vertex
	void CalcPosition(u32 i,u32 j,CVector3D& pos);
	// calculate the normal at a given vertex
	void CalcNormal(u32 i,u32 j,CVector3D& normal);

private:
	// clean up terrain data
	void Reset();
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
