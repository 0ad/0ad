#ifndef INCLUDED_MAPREADER
#define INCLUDED_MAPREADER

#include "MapIO.h"
#include "lib/res/handle.h"
#include "ps/CStr.h"
#include "LightEnv.h"
#include "ps/FileIo.h"

class CObjectEntry;
class CTerrain;
class CUnitManager;
class WaterManager;
class SkyManager;
class CLightEnv;
class CCamera;
class CCinemaManager;
class CTriggerManager;

class CXMLReader;

class CMapReader : public CMapIO
{
	friend class CXMLReader;

public:
	// constructor
	CMapReader();
	// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
	void LoadMap(const char* filename, CTerrain *pTerrain, CUnitManager *pUnitMan,
		WaterManager* pWaterMan, SkyManager* pSkyMan, CLightEnv *pLightEnv, CCamera *pCamera, 
																CCinemaManager* pCinema);

private:
	// UnpackTerrain: unpack the terrain from the input stream
	int UnpackTerrain();
	//UnpackCinema: unpack the cinematic tracks from the input stream
	int UnpackCinema();
	// UnpackObjects: unpack world objects from the input stream
	void UnpackObjects();
	// UnpackObjects: unpack lighting parameters from the input stream
	void UnpackLightEnv();

	// UnpackMap: unpack the given data from the raw data stream into local variables
	int UnpackMap();

	// ApplyData: take all the input data, and rebuild the scene from it
	int ApplyData();

	// ReadXML: read some other data (entities, etc) in XML format
	int ReadXML();

	// clean up everything used during delayed load
	int DelayLoadFinished();

	// size of map 
	ssize_t m_MapSize;
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

	// state latched by LoadMap and held until DelayedLoadFinished
	CFileUnpacker unpacker;
	CTerrain* pTerrain;
	CUnitManager* pUnitMan;
	WaterManager* pWaterMan;
	SkyManager* pSkyMan;
	CLightEnv* pLightEnv;
	CCamera* pCamera;
	CCinemaManager* pCinema;
	CTriggerManager* pTrigMan;
	CStr filename_xml;

	// UnpackTerrain generator state
	size_t cur_terrain_tex;
	size_t num_terrain_tex;

	CXMLReader* xml_reader;
};

#endif
