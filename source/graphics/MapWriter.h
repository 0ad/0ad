/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_MAPWRITER
#define INCLUDED_MAPWRITER

#include <vector>
#include <list>
#include "MapIO.h"
#include "ps/CStr.h"
#include "ps/FileIo.h"


class CLightEnv;
class CTerrain;
class CCamera;
class CCinemaManager;
class CPostprocManager;
class CTriggerManager;
class WaterManager;
class SkyManager;
class CSimulation2;
struct MapTrigger;
struct MapTriggerGroup;
class XMLWriter_File;

class CMapWriter : public CMapIO
{
public:
	// constructor
	CMapWriter();
	// SaveMap: try to save the current map to the given file
	void SaveMap(const VfsPath& pathname, CTerrain* pTerr,
									WaterManager* pWaterMan, SkyManager* pSkyMan, 
									CLightEnv* pLightEnv, CCamera* pCamera, 
									CCinemaManager* pCinema, CPostprocManager* pPostproc,
									CSimulation2* pSimulation2);

private:
	// PackMap: pack the current world into a raw data stream
	void PackMap(CFilePacker& packer, CTerrain* pTerrain);
	// PackTerrain: pack the terrain onto the end of the data stream
	void PackTerrain(CFilePacker& packer, CTerrain* pTerrain);

	// EnumTerrainTextures: build lists of textures used by map, and indices into this list 
	// for each tile on the terrain
	void EnumTerrainTextures(CTerrain* pTerrain, std::vector<CStr>& textures,
		std::vector<STileDesc>& tileIndices);

	// WriteXML: output some other data (entities, etc) in XML format
	void WriteXML(const VfsPath& pathname, WaterManager* pWaterMan,
								SkyManager* pSkyMan, CLightEnv* pLightEnv, CCamera* pCamera, 
								CCinemaManager* pCinema, CPostprocManager* pPostproc,
								CSimulation2* pSimulation2);
};

#endif
