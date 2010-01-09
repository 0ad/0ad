/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
	void LoadMap(const VfsPath& pathname, CTerrain *pTerrain, CUnitManager *pUnitMan,
		WaterManager* pWaterMan, SkyManager* pSkyMan, CLightEnv *pLightEnv, CCamera *pCamera, 
																CCinemaManager* pCinema);

private:
	// UnpackTerrain: unpack the terrain from the input stream
	int UnpackTerrain();
	//UnpackCinema: unpack the cinematic tracks from the input stream
	int UnpackCinema();

	// UnpackMap: unpack the given data from the raw data stream into local variables
	int UnpackMap();

	// ApplyData: take all the input data, and rebuild the scene from it
	int ApplyData();

	// ReadXML: read some other data (entities, etc) in XML format
	int ReadXML();

	// clean up everything used during delayed load
	int DelayLoadFinished();

	// size of map 
	ssize_t m_PatchesPerSide;
	// heightmap for map
	std::vector<u16> m_Heightmap;
	// list of terrain textures used by map
	std::vector<Handle> m_TerrainTextures;
	// tile descriptions for each tile
	std::vector<STileDesc> m_Tiles;
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
	VfsPath filename_xml;

	// UnpackTerrain generator state
	size_t cur_terrain_tex;
	size_t num_terrain_tex;

	CXMLReader* xml_reader;
};

#endif
