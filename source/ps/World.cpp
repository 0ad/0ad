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

extern CLightEnv g_LightEnv;

void CWorld::Initialize(CGameAttributes *pAttribs)
{
	g_EntityTemplateCollection.loadTemplates();

	CStr mapfilename("mods/official/maps/scenarios/");
	mapfilename+=pAttribs->m_MapFile;
	try {
		CMapReader reader;
		reader.LoadMap(mapfilename, &m_Terrain, &m_UnitManager, &g_LightEnv);
	} catch (...) {
		LOG(ERROR, "Failed to load map %s", mapfilename.c_str());
		throw PSERROR_Game_World_MapLoadFailed();
	}
}
