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

/*
<<<<<<< .mine
namespace PlayerArray_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CGameAttributes *pInstance=(CGameAttributes *)JS_GetPrivate(cx, obj);
		if (!JSVAL_IS_INT(id))
			return JS_FALSE;
		uint index=g_ScriptingHost.ValueToInt(id);
		
		// Clamp to a preset upper bound.
		// FIXME I guess we'll ultimately have max players as a config variable
		if (pInstance->m_NumPlayers > PS_MAX_PLAYERS)
			pInstance->m_NumPlayers = PS_MAX_PLAYERS;

		// Resize array according to new number of players (note that the array
		// size will go up to PS_MAX_PLAYERS, but will never be made smaller -
		// this to avoid losing player pointers and make sure they are all
		// reclaimed in the end - it's just simpler that way ;-) )
		if (pInstance->m_NumPlayers+1 > pInstance->m_Players.size())
		{
			for (size_t i=pInstance->m_Players.size();i<=index;i++)
			{
				CPlayer *pNewPlayer=new CPlayer((uint)i);
				pNewPlayer->SetUpdateCallback(pInstance->m_PlayerUpdateCB, pInstance->m_PlayerUpdateCBData);
				pInstance->m_Players.push_back(pNewPlayer);
			}
		}

		if (index > pInstance->m_NumPlayers)
			return JS_FALSE;

		*vp=OBJECT_TO_JSVAL(pInstance->m_Players[index]->GetScript());
		return JS_TRUE;
	}

	JSBool SetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		return JS_FALSE;
	}

	JSClass Class = {
		"PlayerArray", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSBool Construct( JSContext* cx, JSObject* obj, uint argc, jsval* argv, jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, obj);
		*rval=OBJECT_TO_JSVAL(newObj);
		return JS_TRUE;
	}
};

CGameAttributes::CGameAttributes():
	m_UpdateCB(NULL),
	m_MapFile("test01.pmp"),
	m_NumPlayers(2)
{
	ONCE(
		g_ScriptingHost.DefineCustomObjectType(&PlayerArray_JS::Class,
			PlayerArray_JS::Construct, 0, NULL, NULL, NULL, NULL);
		
		ScriptingInit();
	);

	m_PlayerArrayJS=g_ScriptingHost.CreateCustomObject("PlayerArray");
	JS_SetPrivate(g_ScriptingHost.GetContext(), m_PlayerArrayJS, this);

	AddSynchedProperty(L"mapFile", &m_MapFile);
	AddSynchedProperty(L"numPlayers", &m_NumPlayers);

	m_Players.resize(4);
	for (int i=0;i<4;i++)
		m_Players[i]=new CPlayer(i);
		
	m_Players[0]->SetName(L"Gaia");
	m_Players[0]->SetColour(SPlayerColour(0.2f, 0.7f, 0.2f));

	m_Players[1]->SetName(L"Acumen");
	m_Players[1]->SetColour(SPlayerColour(1.0f, 0.0f, 0.0f));

	m_Players[2]->SetName(L"Boco the Insignificant");
	m_Players[2]->SetColour(SPlayerColour(0.0f, 0.0f, 1.0f));

	m_Players[3]->SetName(L"NoMonkey");
	m_Players[3]->SetColour(SPlayerColour(0.5f, 0.0f, 1.0f));
}

CGameAttributes::~CGameAttributes()
{
	std::vector<CPlayer *>::iterator it=m_Players.begin();
	while (it != m_Players.end())
	{
		delete *it;
		++it;
	}
}

jsval CGameAttributes::JSGetPlayers()
{
	return OBJECT_TO_JSVAL(m_PlayerArrayJS);
}

void CGameAttributes::SetValue(CStrW name, CStrW value)
{
	ISynchedJSProperty *prop=GetSynchedProperty(name);
	if (prop)
	{
		prop->FromString(value);
	}
}

void CGameAttributes::Update(CStrW name, ISynchedJSProperty *attrib)
{
	if (m_UpdateCB)
		m_UpdateCB(name, attrib->ToString(), m_UpdateCBData);
}

void CGameAttributes::SetPlayerUpdateCallback(CPlayer::UpdateCallback *cb, void *userdata)
{
	m_PlayerUpdateCB=cb;
	m_PlayerUpdateCBData=userdata;
	
	for (size_t i=0;i<m_Players.size();i++)
	{
		m_Players[i]->SetUpdateCallback(cb, userdata);
	}
}

void CGameAttributes::ScriptingInit()
{
	AddClassProperty(L"players", (GetFn)&CGameAttributes::JSGetPlayers);
	CJSObject<CGameAttributes>::ScriptingInit( "Game" );

}

/*void CGameAttributes::CreateJSObject()
{
	CAttributeMap::CreateJSObject();

	ONCE(
		g_ScriptingHost.DefineCustomObjectType(&PlayerArray_JS::Class,
			PlayerArray_JS::Construct, 0, NULL, NULL, NULL, NULL);
	);

	m_PlayerArrayJS=g_ScriptingHost.CreateCustomObject("PlayerArray");
	JS_SetPrivate(g_ScriptingHost.GetContext(), m_PlayerArrayJS, this);
	int flags=JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT;
	JS_DefineProperty(g_ScriptingHost.GetContext(), m_JSObject, "players",
		OBJECT_TO_JSVAL(m_PlayerArrayJS), NULL, NULL, flags);
}

JSBool CGameAttributes::GetJSProperty(jsval id, jsval *ret)
{
	CStr name=g_ScriptingHost.ValueToString(id);
	if (name == CStr("players"))
		return JS_TRUE;
	return CAttributeMap::GetJSProperty(id, ret);
}*//*

=======
>>>>>>> .r2037
*/
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
	jsval rval;

	JSBool ok = JS_CallFunctionName(g_ScriptingHost.getContext(),
		g_GUI.GetScriptObject(), "reallyStartGame", 0, NULL, &rval);

	assert(ok);
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
