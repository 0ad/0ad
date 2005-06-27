#include "precompiled.h"

#include "Game.h"
#include "CLogger.h"
#ifndef NO_GUI
#include "gui/CGUI.h"
#endif
#include "timer.h"
#include "Profile.h"
#include "Loader.h"

CGame *g_Game=NULL;

// Disable "warning C4355: 'this' : used in base member initializer list".
//   "The base-class constructors and class member constructors are called before
//   this constructor. In effect, you've passed a pointer to an unconstructed
//   object to another constructor. If those other constructors access any
//   members or call member functions on this, the result will be undefined."
// In this case, the pointers are simply stored for later use, so there
// should be no problem.
#ifdef _MSC_VER
# pragma warning (disable: 4355)
#endif

CGame::CGame():
	m_World(this),
	m_Simulation(this),
	m_GameView(this),
	m_pLocalPlayer(NULL),
	m_GameStarted(false)
{
	debug_printf("CGame::CGame(): Game object CREATED; initializing..\n");
}

#ifdef _MSC_VER
# pragma warning (default: 4355)
#endif

CGame::~CGame()
{
	// Again, the in-game call tree is going to be different to the main menu one.
	g_Profiler.StructuralReset();
	debug_printf("CGame::~CGame(): Game object DESTROYED\n");
}



PSRETURN CGame::RegisterInit(CGameAttributes* pAttribs)
{
	LDR_BeginRegistering();

	// RC, 040804 - GameView needs to be initialised before World, otherwise GameView initialisation
	// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
	// values.  At the minute, it's just lighting settings, but could be extended to store camera position.  
	// Storing lighting settings in the gameview seems a little odd, but it's no big deal; maybe move it at 
	// some point to be stored in the world object?
	m_GameView.RegisterInit(pAttribs);
	m_World.RegisterInit(pAttribs);
	m_Simulation.RegisterInit(pAttribs);

	LDR_EndRegistering();
	return 0;
}

PSRETURN CGame::ReallyStartGame()
{
#ifndef NO_GUI

	// Call the reallyStartGame function, but only if it exists
	jsval fval, rval;
	JSBool ok = JS_GetProperty(g_ScriptingHost.getContext(), g_GUI.GetScriptObject(), "reallyStartGame", &fval);
	assert(ok);
	if (ok && !JSVAL_IS_VOID(fval))
	{
		ok = JS_CallFunctionValue(g_ScriptingHost.getContext(), g_GUI.GetScriptObject(), fval, 0, NULL, &rval);
		assert(ok);
	}
#endif

	debug_printf("GAME STARTED, ALL INIT COMPLETE\n");
	m_GameStarted=true;

	// The call tree we've built for pregame probably isn't useful in-game.
	g_Profiler.StructuralReset();

#ifndef NO_GUI
	g_GUI.SendEventToAll("sessionstart");
#endif

	return 0;
}

PSRETURN CGame::StartGame(CGameAttributes *pAttribs)
{
	try
	{
		pAttribs->FinalizeSlots();
		m_NumPlayers=pAttribs->GetSlotCount();

		// Player 0 = Gaia - allocate one extra
		m_Players.resize(m_NumPlayers + 1);

		for (uint i=0;i <= m_NumPlayers;i++)
			m_Players[i]=pAttribs->GetPlayer(i);
		
		m_pLocalPlayer=m_Players[1];

		RegisterInit(pAttribs);
	}
	catch (PSERROR_Game& e)
	{
		return e.getCode();
	}
	return 0;
}

void CGame::Update(double deltaTime)
{
	m_Simulation.Update(deltaTime);
	
	// TODO Detect game over and bring up the summary screen or something
}


CPlayer *CGame::GetPlayer(uint idx)
{
	if (idx > m_NumPlayers)
	{
//		debug_warn("Invalid player ID");
//		LOG(ERROR, "", "Invalid player ID %d (outside 0..%d)", idx, m_NumPlayers);
		return m_Players[0];
	}
	// Be a bit more paranoid - maybe m_Players hasn't been set large enough
	else if (idx >= m_Players.size())
	{
		debug_warn("Invalid player ID");
		LOG(ERROR, "", "Invalid player ID %d (not <=%d - internal error?)", idx, m_Players.size());

		if (m_Players.size() == 0)
		{
			// Hmm. This is a bit of a problem.
			assert2(! "### ### ### ### ERROR: Tried to access the players list when there aren't any players. That really isn't going to work, so I'll give up. ### ###");
			abort();
			return NULL; // else VC2005 warns about not returning a value
		}
		else
			return m_Players[0];
	}
	else
		return m_Players[idx];
}
