#ifndef _MAPWRITER_H
#define _MAPWRITER_H

#include <vector>
#include "MapIO.h"
#include "CStr.h"
#include "FilePacker.h"

class CMapWriter : public CMapIO
{
public:
	// constructor
	CMapWriter();
	// SaveMap: try to save the current map to the given file
	void SaveMap(const char* filename);

private:
	// PackMap: pack the current world into a raw data stream
	void PackMap(CFilePacker& packer);
	// PackTerrain: pack the terrain onto the end of the data stream
	void PackTerrain(CFilePacker& packer);
	// PackObjects: pack world objects onto the end of the output data stream
	void PackObjects(CFilePacker& packer);
	// PackLightEnv: pack lighting parameters onto the end of the output data stream
	void PackLightEnv(CFilePacker& packer);

	// EnumTerrainTextures: build lists of textures used by map, and indices into this list 
	// for each tile on the terrain
	void EnumTerrainTextures(std::vector<CStr>& textures,std::vector<STileDesc>& tileIndices);

	// EnumObjects: build lists of object types used by map, and object descriptions for 
	// each object in the world
	void EnumObjects(std::vector<CStr>& objectTypes,std::vector<SObjectDesc>& objects);
};

#endif
