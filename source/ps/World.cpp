#include "precompiled.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapReader.h"
#include "graphics/MapWriter.h"
#include "graphics/Terrain.h"
#include "graphics/Terrain.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Errors.h"
#include "ps/Game.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"
#include "ps/LoaderThunks.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/EntityManager.h"
#include "simulation/EntityManager.h"
#include "simulation/LOSManager.h"
#include "simulation/TerritoryManager.h"
#include "simulation/Projectile.h"

#define LOG_CATEGORY "world"

// global light settings. this is not a member of CWorld because it is
// passed to the renderer before CWorld exists.
CLightEnv g_LightEnv;


CWorld::CWorld(CGame *pGame):
	m_pGame(pGame),
	m_Terrain(new CTerrain()),
	m_UnitManager(&g_UnitMan),
	m_EntityManager(new CEntityManager()),
	m_ProjectileManager(new CProjectileManager()),
	m_LOSManager(new CLOSManager()),
	m_TerritoryManager(new CTerritoryManager())
{}

void CWorld::Initialize(CGameAttributes *pAttribs)
{
	// TODO: Find a better way of handling these global things
	ONCE(RegMemFun(CEntityTemplateCollection::GetSingletonPtr(), &CEntityTemplateCollection::loadTemplates, L"loadTemplates", 15));

	// Load the map, if one was specified
	if (pAttribs->m_MapFile.Length())
	{
		CStr mapfilename("maps/scenarios/");

		mapfilename += (CStr)pAttribs->m_MapFile;

		CMapReader* reader = 0;

		try {
			reader = new CMapReader;
			reader->LoadMap(mapfilename, m_Terrain, m_UnitManager, g_Renderer.GetWaterManager(),
				g_Renderer.GetSkyManager(), &g_LightEnv, m_pGame->GetView()->GetCamera(), m_pGame->GetView()->GetCinema());
				// fails immediately, or registers for delay loading
		} catch (PSERROR_File&) {
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
	delete m_Terrain;
	delete m_EntityManager;
	delete m_ProjectileManager;
	delete m_LOSManager;
	delete m_TerritoryManager;
}


void CWorld::RewriteMap()
{
	CMapWriter::RewriteAllMaps(m_Terrain, m_UnitManager,
		g_Renderer.GetWaterManager(), g_Renderer.GetSkyManager(),
		&g_LightEnv, m_pGame->GetView()->GetCamera(), 
								m_pGame->GetView()->GetCinema());
}
