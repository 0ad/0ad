#include "precompiled.h"

#include "CStr.h"
#include "CLogger.h"
#include "Errors.h"

#include "World.h"
#include "MapReader.h"
#include "Game.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"

#define LOG_CATEGORY "world"

extern CLightEnv g_LightEnv;

void CWorld::Initialize(CGameAttributes *pAttribs)
{
	g_EntityTemplateCollection.loadTemplates();

	CStr mapfilename("maps/scenarios/");
	mapfilename+=pAttribs->m_MapFile;
	try {
		CMapReader reader;
		reader.LoadMap(mapfilename, &m_Terrain, &m_UnitManager, &g_LightEnv);
	} catch (...) {
		LOG(ERROR, LOG_CATEGORY, "Failed to load map %s", mapfilename.c_str());
		throw PSERROR_Game_World_MapLoadFailed();
	}
}

CWorld::~CWorld()
{
	// The Entity Manager should perhaps be converted into a CWorld member..
	// But for now, we'll just create and delete the global singleton instance
	// following the creation and deletion of CWorld.
	// The reason for not keeping the instance around is that we require a
	// clean slate for each game start.
	delete &m_EntityManager;
}
