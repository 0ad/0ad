#include "precompiled.h"

#include "Game.h"
#include "CLogger.h"
#ifndef NO_GUI
#include "gui/CGUI.h"
#endif

CGame *g_Game=NULL;

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
		
		ScriptingInit("GameAttributes");
	);

	m_PlayerArrayJS=g_ScriptingHost.CreateCustomObject("PlayerArray");
	JS_SetPrivate(g_ScriptingHost.GetContext(), m_PlayerArrayJS, this);

	AddSynchedProperty(L"mapFile", &m_MapFile);
	AddSynchedProperty(L"numPlayers", &m_NumPlayers);
	
	AddProperty(L"players", (GetFn)&CGameAttributes::JSGetPlayers);

	m_Players.resize(3);
	for (int i=0;i<3;i++)
		m_Players[i]=new CPlayer(i);
		
	m_Players[0]->SetName(L"Gaia");
	m_Players[0]->SetColour(SPlayerColour(0.2f, 0.7f, 0.2f));

	m_Players[1]->SetName(L"Acumen");
	m_Players[1]->SetColour(SPlayerColour(1.0f, 0.0f, 0.0f));

	m_Players[2]->SetName(L"Boco the Insignificant");
	m_Players[2]->SetColour(SPlayerColour(0.0f, 0.0f, 1.0f));
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
}*/

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
	debug_out("CGame::CGame(): Game object CREATED\n");
}

#ifdef _MSC_VER
# pragma warning (default: 4355)
#endif

CGame::~CGame()
{
	debug_out("CGame::~CGame(): Game object DESTROYED\n");
}

PSRETURN CGame::StartGame(CGameAttributes *pAttribs)
{
	try
	{
		m_NumPlayers=pAttribs->m_NumPlayers;

		// Note: If m_Players is resized after this point (causing a reallocation)
		// various bits of code will still contain pointers to data at the original
		// locations. This is seldom a good thing. Make it big enough here.

		// Player 0 = Gaia - allocate one extra
		m_Players.resize(m_NumPlayers + 1);

		for (uint i=0;i <= m_NumPlayers;i++)
			m_Players[i]=pAttribs->m_Players[i];
		
		m_pLocalPlayer=m_Players[1];

		// RC, 040804 - GameView needs to be initialised before World, otherwise GameView initialisation
		// overwrites anything stored in the map file that gets loaded by CWorld::Initialize with default
		// values.  At the minute, it's just lighting settings, but could be extended to store camera position.  
		// Storing lighting settings in the gameview seems a little odd, but it's no big deal; maybe move it at 
		// some point to be stored in the world object?
		m_GameView.Initialize(pAttribs);
		m_World.Initialize(pAttribs);
		m_Simulation.Initialize(pAttribs);

		m_GameStarted=true;

#ifndef NO_GUI
		g_GUI.SendEventToAll("sessionstart");
#endif
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
