#ifndef _MAPWRITER_H
#define _MAPWRITER_H

#include <vector>
#include "MapIO.h"
#include "CStr.h"
#include "FilePacker.h"

class CLightEnv;
class CTerrain;
class CUnitManager;

class CMapWriter : public CMapIO
{
public:
	// constructor
	CMapWriter();
	// SaveMap: try to save the current map to the given file
	void SaveMap(const char* filename, CTerrain *pTerr, CLightEnv *pLightEnv, CUnitManager *pUnitMan);

private:
	// PackMap: pack the current world into a raw data stream
	void PackMap(CFilePacker& packer, CTerrain *pTerr, CLightEnv *pLightEnv, CUnitManager *pUnitMan);
	// PackTerrain: pack the terrain onto the end of the data stream
	void PackTerrain(CFilePacker& packer, CTerrain *pTerrain);
	// PackObjects: pack world objects onto the end of the output data stream
	void PackObjects(CFilePacker& packer, CUnitManager *pUnitMan);
	// PackLightEnv: pack lighting parameters onto the end of the output data stream
	void PackLightEnv(CFilePacker& packer, CLightEnv *pLightEnv);

	// EnumTerrainTextures: build lists of textures used by map, and indices into this list 
	// for each tile on the terrain
	void EnumTerrainTextures(CTerrain *pTerrain, std::vector<CStr>& textures,
		std::vector<STileDesc>& tileIndices);

	// EnumObjects: build lists of object types used by map, and object descriptions for 
	// each object in the world
	void EnumObjects(CUnitManager *pUnitMan, std::vector<CStr>& objectTypes,
		std::vector<SObjectDesc>& objects);
};

#endif
