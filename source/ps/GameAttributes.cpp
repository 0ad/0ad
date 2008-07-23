#include "precompiled.h"

#include <sstream>

#include "GameAttributes.h"
#include "Game.h"
#include "ConfigDB.h"
#include "network/NetSession.h"
#include "CLogger.h"
#include "ps/XML/Xeromyces.h"
#include "simulation/LOSManager.h"


CPlayerSlot::CPlayerSlot(size_t slotID, CPlayer *pPlayer):
	m_SlotID(slotID),
	m_Assignment(SLOT_OPEN),
	m_pSession(NULL),
	m_SessionID(0),
	m_pPlayer(pPlayer),
	m_Callback(NULL)
{
	//AddProperty(L"session", (GetFn)&CPlayerSlot::JSI_GetSession);
	AddLocalProperty(L"session", &m_pSession, true );
	AddLocalProperty(L"player", &m_pPlayer, true );
}

CPlayerSlot::~CPlayerSlot()
{}

void CPlayerSlot::ScriptingInit()
{
	AddMethod<bool, &CPlayerSlot::JSI_AssignClosed>("assignClosed", 0);
	AddMethod<bool, &CPlayerSlot::JSI_AssignToSession>("assignToSession", 1);
	AddMethod<bool, &CPlayerSlot::JSI_AssignLocal>("assignLocal", 0);
	AddMethod<bool, &CPlayerSlot::JSI_AssignOpen>("assignOpen", 0);
	AddProperty(L"assignment", &CPlayerSlot::JSI_GetAssignment);
//	AddMethod<bool, &CPlayerSlot::JSI_AssignAI>("assignAI", <num_args>);

	CJSObject<CPlayerSlot>::ScriptingInit("PlayerSlot");
}

jsval CPlayerSlot::JSI_GetSession(JSContext* UNUSED(cx))
{
	if (m_pSession)
		return OBJECT_TO_JSVAL(m_pSession->GetScript());
	else
		return JSVAL_NULL;
}

jsval CPlayerSlot::JSI_GetAssignment(JSContext* UNUSED(cx))
{
	switch (m_Assignment)
	{
		case SLOT_CLOSED:
			return g_ScriptingHost.UCStringToValue(L"closed");
		case SLOT_OPEN:
			return g_ScriptingHost.UCStringToValue(L"open");
		case SLOT_SESSION:
			return g_ScriptingHost.UCStringToValue(L"session");
/*		case SLOT_AI:*/
		default:
			return INT_TO_JSVAL(m_Assignment);
	}
}

bool CPlayerSlot::JSI_AssignClosed(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	AssignClosed();
	return true;
}

bool CPlayerSlot::JSI_AssignOpen(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	AssignOpen();
	return true;
}

bool CPlayerSlot::JSI_AssignToSession(JSContext* UNUSED(cx), uintN argc, jsval* argv)
{
	if (argc != 1)
		return false;
	CNetSession* pSession = ToNative<CNetSession>(argv[0]);
	if (pSession)
	{
		AssignToSession(pSession);
		return true;
	}
	else
		return true;
}

bool CPlayerSlot::JSI_AssignLocal(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	AssignToSessionID(1);
	return true;
}

void CPlayerSlot::CallCallback()
{
	if (m_Callback)
		m_Callback(m_CallbackData, this);
}

void CPlayerSlot::SetAssignment(EPlayerSlotAssignment assignment,
	CNetSession *pSession, int sessionID)
{
	m_Assignment=assignment;
	m_pSession=pSession;
	m_SessionID=sessionID;
	
	CallCallback();
}

void CPlayerSlot::AssignClosed()
{
	SetAssignment(SLOT_CLOSED, NULL, -1);
}

void CPlayerSlot::AssignOpen()
{
	SetAssignment(SLOT_OPEN, NULL, -1);
}

void CPlayerSlot::AssignToSession(CNetSession *pSession)
{
	SetAssignment(SLOT_SESSION, pSession, pSession->GetID());
	m_pPlayer->SetName(pSession->GetName());
}

void CPlayerSlot::AssignToSessionID(int id)
{
	SetAssignment(SLOT_SESSION, NULL, id);
}

void CPlayerSlot::AssignLocal()
{
	AssignToSessionID(1);
}

namespace PlayerSlotArray_JS
{
	JSBool GetProperty( JSContext* cx, JSObject* obj, jsval id, jsval* vp )
	{
		CGameAttributes *pInstance=(CGameAttributes *)JS_GetPrivate(cx, obj);
		if (!JSVAL_IS_INT(id))
			return JS_FALSE;
		size_t index=ToPrimitive<size_t>(id);
		
		if (index >= pInstance->m_NumSlots)
			return JS_FALSE;

		*vp=OBJECT_TO_JSVAL(pInstance->m_PlayerSlots[index]->GetScript());
		return JS_TRUE;
	}

	JSBool SetProperty( JSContext* UNUSED(cx), JSObject* UNUSED(obj), jsval UNUSED(id), jsval* UNUSED(vp) )
	{
		return JS_FALSE;
	}

	JSClass Class = {
		"PlayerSlotArray", JSCLASS_HAS_PRIVATE,
		JS_PropertyStub, JS_PropertyStub,
		GetProperty, SetProperty,
		JS_EnumerateStub, JS_ResolveStub,
		JS_ConvertStub, JS_FinalizeStub
	};

	JSBool Construct( JSContext* cx, JSObject* obj, uintN argc, jsval* UNUSED(argv), jsval* rval )
	{
		if (argc != 0)
			return JS_FALSE;

		JSObject *newObj=JS_NewObject(cx, &Class, NULL, obj);
		*rval=OBJECT_TO_JSVAL(newObj);
		return JS_TRUE;
	}
};

CGameAttributes::CGameAttributes():
	m_MapFile("test01.pmp"),
	m_ResourceLevel("default"),
	m_StartingPhase("default"),
	m_LOSSetting(LOS_SETTING_NORMAL),
	m_FogOfWar(true),
	m_GameMode("default"),
	m_ScreenshotMode(false),
	m_NumSlots(8),
	m_UpdateCB(NULL),
	m_PlayerUpdateCB(NULL),
	m_PlayerSlotAssignmentCB(NULL)
{
	m_PlayerSlotArrayJS=g_ScriptingHost.CreateCustomObject("PlayerSlotArray");
	JS_AddRoot(g_ScriptingHost.GetContext(), &m_PlayerSlotArrayJS);
	JS_SetPrivate(g_ScriptingHost.GetContext(), m_PlayerSlotArrayJS, this);

	AddSynchedProperty(L"mapFile", &m_MapFile);
	AddSynchedProperty(L"resourceLevel", &m_ResourceLevel);
	AddSynchedProperty(L"startingPhase", &m_StartingPhase);
	AddSynchedProperty(L"numSlots", &m_NumSlots, &CGameAttributes::OnNumSlotsUpdate);
	AddSynchedProperty(L"losSetting", &m_LOSSetting);
	AddSynchedProperty(L"fogOfWar", &m_FogOfWar);
	AddSynchedProperty(L"gameMode", &m_GameMode);
	AddSynchedProperty(L"screenshotMode", &m_ScreenshotMode);

	CXeromyces XeroFile;
	if (XeroFile.Load("temp/players.xml") != PSRETURN_OK)
	{
		LOG(CLogger::Error, "", "Failed to load players list (temp/players.xml)");

		// Basic default players

		m_Players.push_back(new CPlayer(0));
		m_Players.back()->SetName(L"Gaia");
		m_Players.back()->SetColour(SPlayerColour(1.0f, 1.0f, 1.0f));

		m_Players.push_back(new CPlayer(1));
		m_Players.back()->SetName(L"Player 1");
		m_Players.back()->SetColour(SPlayerColour(0.4375f, 0.4375f, 0.8125f));
	}
	else
	{
		int at_name = XeroFile.GetAttributeID("name");
		int at_rgb = XeroFile.GetAttributeID("rgb");

		XMBElement root = XeroFile.GetRoot();
		XERO_ITER_EL(root, player)
		{
			XMBAttributeList attr = player.GetAttributes();
			m_Players.push_back(new CPlayer((int)m_Players.size()));
			m_Players.back()->SetName(attr.GetNamedItem(at_name));

			std::stringstream str;
			str << (CStr)attr.GetNamedItem(at_rgb);
			int r, g, b;
			if (str >> r >> g >> b)
				m_Players.back()->SetColour(SPlayerColour(r/255.0f, g/255.0f, b/255.0f));
		}
	}
	
	std::vector<CPlayer *>::iterator it=m_Players.begin();
	++it; // Skip Gaia - gaia doesn't account for a slot
	for (;it != m_Players.end();++it)
	{
		m_PlayerSlots.push_back(new CPlayerSlot((*it)->GetPlayerID()-1, *it));
	}
	// The first player is always the server player in MP, or the local player
	// in SP
	m_PlayerSlots[0]->AssignToSessionID(1);
}

CGameAttributes::~CGameAttributes()
{
	std::vector<CPlayerSlot *>::iterator it=m_PlayerSlots.begin();
	while (it != m_PlayerSlots.end())
	{
		delete (*it)->GetPlayer();
		delete *it;
		++it;
	}

	// PT: ??? - Gaia isn't a player slot, but still needs to be deleted; but
	// this feels rather unpleasantly inconsistent with the above code, so maybe
	// there's a better way to avoid the memory leak.
	delete m_Players[0];

	JS_RemoveRoot(g_ScriptingHost.GetContext(), &m_PlayerSlotArrayJS);
}

void CGameAttributes::ScriptingInit()
{
	g_ScriptingHost.DefineCustomObjectType(&PlayerSlotArray_JS::Class,
		PlayerSlotArray_JS::Construct, 0, NULL, NULL, NULL, NULL);
	
	AddMethod<jsval_t, &CGameAttributes::JSI_GetOpenSlot>("getOpenSlot", 0);
	AddProperty(L"slots", &CGameAttributes::JSI_GetPlayerSlots);

	CJSObject<CGameAttributes>::ScriptingInit("GameAttributes");
}

jsval_t CGameAttributes::JSI_GetOpenSlot(JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv))
{
	std::vector <CPlayerSlot *>::iterator it;
	for (it = m_PlayerSlots.begin();it != m_PlayerSlots.end();++it)
	{
		if ((*it)->GetAssignment() == SLOT_OPEN)
			return OBJECT_TO_JSVAL((*it)->GetScript());
	}
	return JSVAL_NULL;
}

jsval CGameAttributes::JSI_GetPlayerSlots(JSContext* UNUSED(cx))
{
	return OBJECT_TO_JSVAL(m_PlayerSlotArrayJS);
}

void CGameAttributes::OnNumSlotsUpdate(CSynchedJSObjectBase *owner)
{
	CGameAttributes *pInstance=(CGameAttributes*)owner;
	
	// Clamp to a preset upper bound.
	CConfigValue *val=g_ConfigDB.GetValue(CFG_MOD, "max_players");
	uint maxPlayers=PS_MAX_PLAYERS;
	if (val)
		val->GetUnsignedInt(maxPlayers);
	if (pInstance->m_NumSlots > maxPlayers)
		pInstance->m_NumSlots = maxPlayers;

	// Resize array according to new number of slots (note that the array
	// size will go up to maxPlayers (above), but will never be made smaller -
	// this to avoid losing player pointers and make sure they are all
	// reclaimed in the end - it's just simpler that way ;-) )
	if (pInstance->m_NumSlots > pInstance->m_PlayerSlots.size())
	{
		for (size_t i=pInstance->m_PlayerSlots.size();i<=pInstance->m_NumSlots;i++)
		{
			CPlayer *pNewPlayer=new CPlayer(i+1);
			pNewPlayer->SetUpdateCallback(pInstance->m_PlayerUpdateCB,
										  pInstance->m_PlayerUpdateCBData);
			pInstance->m_Players.push_back(pNewPlayer);

			CPlayerSlot *pNewSlot=new CPlayerSlot(i, pNewPlayer);
			pNewSlot->SetCallback(pInstance->m_PlayerSlotAssignmentCB,
								  pInstance->m_PlayerSlotAssignmentCBData);
			pInstance->m_PlayerSlots.push_back(pNewSlot);
		}
	}
}

CPlayer *CGameAttributes::GetPlayer(size_t id)
{
	if (id < m_Players.size())
		return m_Players[id];
	else
	{
		LOG(CLogger::Error, "", "CGameAttributes::GetPlayer(): Attempt to get player %d (while there only are %d players)", id, m_Players.size());
		return m_Players[0];
	}
}

CPlayerSlot *CGameAttributes::GetSlot(size_t index)
{
	if (index < m_PlayerSlots.size())
		return m_PlayerSlots[index];
	else
	{
		LOG(CLogger::Error, "", "CGameAttributes::GetSlot(): Attempt to get slot %d (while there only are %d slots)", index, m_PlayerSlots.size());
		return m_PlayerSlots[0];
	}
}

void CGameAttributes::FinalizeSlots()
{
	for (size_t i=0; i<m_PlayerSlots.size(); i++) {
		CPlayerSlot *slot = m_PlayerSlots[i];
		EPlayerSlotAssignment assignment = slot->GetAssignment();
		if (assignment != SLOT_OPEN && assignment != SLOT_CLOSED)
		{
			slot->GetPlayer()->SetActive(true);
		}
	}
}

void CGameAttributes::SetValue(const CStrW& name, const CStrW& value)
{
	ISynchedJSProperty *prop=GetSynchedProperty(name);
	if (prop)
	{
		prop->FromString(value);
	}
}

void CGameAttributes::Update(const CStrW& name, ISynchedJSProperty *attrib)
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

void CGameAttributes::SetPlayerSlotAssignmentCallback(PlayerSlotAssignmentCB *cb, void *userdata)
{
	m_PlayerSlotAssignmentCB=cb;
	m_PlayerSlotAssignmentCBData=userdata;
	
	for (size_t i=0;i<m_PlayerSlots.size();i++)
	{
		m_PlayerSlots[i]->SetCallback(cb, userdata);
	}
}
