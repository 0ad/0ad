#ifndef _MAPREADER_H
#define _MAPREADER_H

#include "MapIO.h"
#include "CStr.h"
#include "LightEnv.h"
#include "FileUnpacker.h"

class CObjectEntry;

class CMapReader : public CMapIO
{
public:
	// constructor
	CMapReader();
	// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
	void LoadMap(const char* filename);

private:
	// UnpackMap: unpack the given data from the raw data stream into local variables
	void UnpackMap(CFileUnpacker& unpacker);
	// UnpackTerrain: unpack the terrain from the input stream
	void UnpackTerrain(CFileUnpacker& unpacker);
	// UnpackObjects: unpack world objects from the input stream
	void UnpackObjects(CFileUnpacker& unpacker);
	// UnpackObjects: unpack lighting parameters from the input stream
	void UnpackLightEnv(CFileUnpacker& unpacker);

	// ApplyData: take all the input data, and rebuild the scene from it
	void ApplyData(CFileUnpacker& unpacker);

	// size of map 
	u32 m_MapSize;
	// heightmap for map
	std::vector<u16> m_Heightmap;
	// list of terrain textures used by map
	std::vector<Handle> m_TerrainTextures;
	// tile descriptions for each tile
	std::vector<STileDesc> m_Tiles;
	// list of object types used by map
	std::vector<CObjectEntry*> m_ObjectTypes;
	// descriptions for each objects
	std::vector<SObjectDesc> m_Objects;
	// lightenv stored in file
	CLightEnv m_LightEnv;
};

#endif