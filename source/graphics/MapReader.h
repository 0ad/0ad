/* Copyright (C) 2013 Wildfire Games.
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
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/system/Entity.h"

class CObjectEntry;
class CTerrain;
class WaterManager;
class SkyManager;
class CLightEnv;
class CCinemaManager;
class CPostprocManager;
class CTriggerManager;
class CSimulation2;
class CSimContext;
class CTerrainTextureEntry;
class CScriptValRooted;
class ScriptInterface;
class CGameView;
class CXMLReader;
class CMapGenerator;

class CMapReader : public CMapIO
{
	friend class CXMLReader;

public:
	// constructor
	CMapReader();
	~CMapReader();

	// LoadMap: try to load the map from given file; reinitialise the scene to new data if successful
	void LoadMap(const VfsPath& pathname, const CScriptValRooted& settings, CTerrain*, WaterManager*, SkyManager*, CLightEnv*, CGameView*,
		CCinemaManager*, CTriggerManager*, CPostprocManager* pPostproc, CSimulation2*, const CSimContext*, 
	        int playerID, bool skipEntities);

	void LoadRandomMap(const CStrW& scriptFile, const CScriptValRooted& settings, CTerrain*, WaterManager*, SkyManager*, CLightEnv*, CGameView*, CCinemaManager*, CTriggerManager*, CPostprocManager* pPostproc_, CSimulation2*, int playerID);

private:
	// Load script settings for use by scripts
	int LoadScriptSettings();

	// load player settings only
	int LoadPlayerSettings();

	// load map settings only
	int LoadMapSettings();

	// UnpackTerrain: unpack the terrain from the input stream
	int UnpackTerrain();
	//UnpackCinema: unpack the cinematic tracks from the input stream
	int UnpackCinema();

	// UnpackMap: unpack the given data from the raw data stream into local variables
	int UnpackMap();

	// ApplyData: take all the input data, and rebuild the scene from it
	int ApplyData();
	int ApplyTerrainData();

	// read some misc data from the XML file
	int ReadXML();

	// read entity data from the XML file
	int ReadXMLEntities();

	// clean up everything used during delayed load
	int DelayLoadFinished();
	
	// Copy random map settings over to sim
	int LoadRMSettings();

	// Generate random map
	int GenerateMap();

	// Parse script data into terrain
	int ParseTerrain();

	// Parse script data into entities
	int ParseEntities();

	// Parse script data into environment
	int ParseEnvironment();

	// Parse script data into camera
	int ParseCamera();


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

	// random map data
	CStrW m_ScriptFile;
	CScriptValRooted m_ScriptSettings;
	CScriptValRooted m_MapData;

	CMapGenerator* m_MapGen;

	// state latched by LoadMap and held until DelayedLoadFinished
	CFileUnpacker unpacker;
	CTerrain* pTerrain;
	WaterManager* pWaterMan;
	SkyManager* pSkyMan;
	CPostprocManager* pPostproc;
	CLightEnv* pLightEnv;
	CGameView* pGameView;
	CCinemaManager* pCinema;
	CTriggerManager* pTrigMan;
	CSimulation2* pSimulation2;
	const CSimContext* pSimContext;
	int m_PlayerID;
	bool m_SkipEntities;
	VfsPath filename_xml;
	bool only_xml;
	u32 file_format_version;
	entity_id_t m_StartingCameraTarget;
	CVector3D m_StartingCamera;

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
	CScriptValRooted GetMapSettings(ScriptInterface& scriptInterface);

private:
	CStr m_ScriptSettings;
};

#endif
