#include "precompiled.h"

#include "CStr.h"
#include "CLogger.h"
#include "ps/Errors.h"

#include "World.h"
#include "MapReader.h"
#include "Game.h"
#include "GameAttributes.h"
#include "Terrain.h"
#include "LightEnv.h"
#include "BaseEntityCollection.h"
#include "EntityManager.h"
#include "timer.h"
#include "Loader.h"
#include "LoaderThunks.h"
#include "graphics/MapWriter.h"
#include "UnitManager.h"
#include "EntityManager.h"
#include "Projectile.h"
#include "LOSManager.h"

#define LOG_CATEGORY "world"

// global light settings. this is not a member of CWorld because it is
// passed to the renderer before CWorld exists.
CLightEnv g_LightEnv;


CWorld::CWorld(CGame *pGame):
	m_pGame(pGame),
	m_Terrain(),
	m_UnitManager(g_UnitMan),
	m_EntityManager(*(new CEntityManager())),
	m_ProjectileManager(*(new CProjectileManager())),
	m_LOSManager(*(new CLOSManager()))
{}

void CWorld::Initialize(CGameAttributes *pAttribs)
{
	// TODO: Find a better way of handling these global things
	ONCE(RegMemFun(CBaseEntityCollection::GetSingletonPtr(), &CBaseEntityCollection::loadTemplates, L"LoadTemplates", 6));

	// Load the map, if one was specified
	if (pAttribs->m_MapFile.Length())
	{
		CStr mapfilename("maps/scenarios/");

		mapfilename += (CStr)pAttribs->m_MapFile;

		CMapReader* reader = 0;

		try {
			reader = new CMapReader;
			reader->LoadMap(mapfilename, &m_Terrain, &m_UnitManager, &g_LightEnv);
				// fails immediately, or registers for delay loading
		} catch (...) {
			delete reader;
			LOG(ERROR, LOG_CATEGORY, "Failed to load map %s", mapfilename.c_str());
			throw PSERROR_Game_World_MapLoadFailed();
		}
	}
}


void CWorld::RegisterInit(CGameAttributes *pAttribs)
{
	Initialize(pAttribs);
}


CWorld::~CWorld()
{
	// The Entity Manager should perhaps be converted into a CWorld member..
	// But for now, we'll just create and delete the global singleton instance
	// following the creation and deletion of CWorld.
	// The reason for not keeping the instance around is that we require a
	// clean slate for each game start.
	delete &m_EntityManager;
	delete &m_ProjectileManager;
}


void CWorld::RewriteMap()
{
	CMapWriter::RewriteAllMaps(&m_Terrain, &m_UnitManager, &g_LightEnv);
}
