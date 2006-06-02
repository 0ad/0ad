#ifndef _MAPWRITER_H
#define _MAPWRITER_H

#include <vector>
#include "MapIO.h"
#include "ps/CStr.h"
#include "ps/FilePacker.h"

class CLightEnv;
class CTerrain;
class CUnitManager;
class CCamera;

class CMapWriter : public CMapIO
{
public:
	// constructor
	CMapWriter();
	// SaveMap: try to save the current map to the given file
	void SaveMap(const char* filename, CTerrain *pTerr, CUnitManager *pUnitMan, CLightEnv *pLightEnv, CCamera *pCamera);

	// RewriteAllMaps: for use during development: load/save all maps, to
	// update them to the newest format.
	static void RewriteAllMaps(CTerrain *pTerrain, CUnitManager *pUnitMan, CLightEnv *pLightEnv, CCamera *pCamera);

private:
	// PackMap: pack the current world into a raw data stream
	void PackMap(CFilePacker& packer, CTerrain *pTerr, CUnitManager *pUnitMan, CLightEnv *pLightEnv);
	// PackTerrain: pack the terrain onto the end of the data stream
	void PackTerrain(CFilePacker& packer, CTerrain *pTerrain);

	// EnumTerrainTextures: build lists of textures used by map, and indices into this list 
	// for each tile on the terrain
	void EnumTerrainTextures(CTerrain *pTerrain, std::vector<CStr>& textures,
		std::vector<STileDesc>& tileIndices);

	// WriteXML: output some other data (entities, etc) in XML format
	void WriteXML(const char* filename, CUnitManager* pUnitMan, CLightEnv *pLightEnv, CCamera *pCamera);
};

#endif
