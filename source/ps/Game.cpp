#include "precompiled.h"

#include "Game.h"
#include "CLogger.h"

CGame *g_Game=NULL;

JSBool CGameAttributes::FillFromJS(JSContext *cx, JSObject *obj)
{
#define GETVAL(_name) jsval _name=g_ScriptingHost.GetObjectProperty(obj, #_name)
	GETVAL(mapFile);

#define TOSTRING(_jsval) g_ScriptingHost.ValueToString(_jsval)
	if (JSVAL_IS_STRING(mapFile))
		m_MapFile=TOSTRING(mapFile);

	return JS_TRUE;
}

CGame::CGame():
	m_World(this),
	m_Simulation(this),
	m_GameView(this),
	m_pLocalPlayer(NULL)
{
	debug_out("CGame::CGame(): Game object CREATED\n");
}

CGame::~CGame()
{
	debug_out("CGame::~CGame(): Game object DESTROYED\n");
}

PSRETURN CGame::Initialize(CGameAttributes *pAttribs)
{
	try
	{
		// RC, 040804 - GameView needs to be initialised before World, otherwise GameView initialisation
		// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
		// values.  At the minute, it's just lighting settings, but could be extended to store camera position.  
		// Storing lighting settings in the gameview seems a little odd, but it's no big deal; maybe move it at 
		// some point to be stored in the world object?
		m_GameView.Initialize(pAttribs);
		m_World.Initialize(pAttribs);
		m_Simulation.Initialize(pAttribs);
	}
	catch (PSERROR_Game e)
	{
		return e.code;
	}
	return 0;
}

void CGame::Update(double deltaTime)
{
	m_Simulation.Update(deltaTime);
	
	// TODO Detect game over and bring up the summary screen or something
}
