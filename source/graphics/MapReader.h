#ifndef _MAPREADER_H
#define _MAPREADER_H

#include "MapIO.h"
#include "res/handle.h"
#include "CStr.h"
#include "LightEnv.h"
#include "FileUnpacker.h"

class CObjectEntry;
class CTerrain;
class CUnitManager;
class CLightEnv;

class CMapReader : public CMapIO
{
public:
	// constructor
	CMapReader();
	// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
	void LoadMap(const char* filename, CTerrain *pTerrain, CUnitManager *pUnitMan, CLightEnv *pLightEnv);

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
	void ApplyData(CFileUnpacker& unpacker, CTerrain *pTerrain, CUnitManager *pUnitMan, CLightEnv *pLightEnv);

	// ReadXML: read some other data (entities, etc) in XML format
	void ReadXML(const char* filename);

	// size of map 
	u32 m_MapSize;
	// heightmap for map
	std::vector<u16> m_Heightmap;
	// list of terrain textures used by map
	std::vector<Handle> m_TerrainTextures;
	// tile descriptions for each tile
	std::vector<STileDesc> m_Tiles;
	// list of object types used by map
	std::vector<CStr> m_ObjectTypes;
	// descriptions for each objects
	std::vector<SObjectDesc> m_Objects;
	// lightenv stored in file
	CLightEnv m_LightEnv;
};

#endif
