/* Copyright (C) 2010 Wildfire Games.
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
#include "simulation2/system/Entity.h"

class CObjectEntry;
class CTerrain;
class WaterManager;
class SkyManager;
class CLightEnv;
class CCinemaManager;
class CTriggerManager;
class CSimulation2;
class CTerrainTextureEntry;
class CScriptValRooted;
class ScriptInterface;
class CGameView;

class CXMLReader;

class CMapReader : public CMapIO
{
	friend class CXMLReader;

public:
	// constructor
	CMapReader();
	// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
	void LoadMap(const VfsPath& pathname, CTerrain*,
		WaterManager*, SkyManager*, CLightEnv*, CGameView*,
		CCinemaManager*, CTriggerManager*, CSimulation2*, int playerID);

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
	std::vector<CTerrainTextureEntry*> m_TerrainTextures;
	// tile descriptions for each tile
	std::vector<STileDesc> m_Tiles;
	// lightenv stored in file
	CLightEnv m_LightEnv;
	// startup script
	CStrW m_Script;

	// state latched by LoadMap and held until DelayedLoadFinished
	CFileUnpacker unpacker;
	CTerrain* pTerrain;
	WaterManager* pWaterMan;
	SkyManager* pSkyMan;
	CLightEnv* pLightEnv;
	CGameView* pGameView;
	CCinemaManager* pCinema;
	CTriggerManager* pTrigMan;
	CSimulation2* pSimulation2;
	int m_PlayerID;
	VfsPath filename_xml;
	bool only_xml;
	u32 file_format_version;
	entity_id_t m_CameraStartupTarget;

	// UnpackTerrain generator state
	size_t cur_terrain_tex;
	size_t num_terrain_tex;

	CXMLReader* xml_reader;
};

/**
 * A restricted map reader that returns various summary information
 * for use by scripts (particularly the GUI).
 */
class CMapSummaryReader
{
public:
	/**
	 * Try to load a map file.
	 * @param pathname Path to .pmp or .xml file
	 */
	PSRETURN LoadMap(const VfsPath& pathname);

	/**
	 * Returns a value of the form:
	 * @code
	 * {
	 *   "settings": { ... contents of the map's <ScriptSettings> ... }
	 * }
	 * @endcode
	 */
	CScriptValRooted GetScriptData(ScriptInterface& scriptInterface);

private:
	utf16string m_ScriptSettings;
};

#endif
