#include "precompiled.h"

#include "CStr.h"
#include "CLogger.h"
#include "Errors.h"

#include "World.h"
#include "MapReader.h"
#include "Game.h"
#include "Terrain.h"

CTerrain g_Terrain;

void CWorld::Initialize(CGameAttributes *pAttribs)
{
	// load a map if we were given one
	if (pAttribs->m_MapFile) {
		CStr mapfilename("mods/official/maps/scenarios/");
		mapfilename+=pAttribs->m_MapFile;
		try {
			CMapReader reader;
			reader.LoadMap(mapfilename);
		} catch (...) {
			LOG(ERROR, "Failed to load map %s", mapfilename.c_str());
			throw PSERROR_Game_World_MapLoadFailed();
		}
	}
}
