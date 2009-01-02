#include "precompiled.h"

#include "Player.h"
#include "network/NetMessage.h"
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "ps/scripting/JSCollection.h"
#include "simulation/LOSManager.h"

CPlayer::CPlayer(size_t playerID):
	m_PlayerID(playerID),
	m_Name(CStrW(L"Player #")+CStrW((unsigned)playerID)),
	m_Civilization(L""),
	m_Colour(0.7f, 0.7f, 0.7f),
	m_UpdateCB(0),
	m_Active(false)
{
	m_LOSToken = LOS_GetTokenFor(playerID);
	
	// Initialize diplomacy: we need to be neutral to Gaia and enemy to everyone else;
	// however, if we are Gaia, we'll just be neutral to everyone; finally, everyone
	// will be allied with themselves.
	m_DiplomaticStance[0] = DIPLOMACY_NEUTRAL;
	for(int i=1; i<=PS_MAX_PLAYERS; i++)
	{
		m_DiplomaticStance[i] = (m_PlayerID==0 ? DIPLOMACY_NEUTRAL : DIPLOMACY_ENEMY);
	}
	m_DiplomaticStance[m_PlayerID] = DIPLOMACY_ALLIED;

	AddSynchedProperty( L"name", &m_Name );
	AddSynchedProperty( L"civilization", &m_Civilization );

	// HACK - since we have to use setColour to update this, we don't want to
	// expose a colour property. Meanwhile, we want to have a property "colour"
	// available to be able to use the update/sync system.
	// So, this is only added to the SynchedProperties list and not also passed
	// to CJSObject's list
	ISynchedJSProperty *prop=new CSynchedJSProperty<SPlayerColour>(L"colour", &m_Colour, this);
	m_SynchedProperties[L"colour"]=prop;

	// HACK - maintain diplomacy synced in the same way, by adding a dummy property for each stance
	for(int i=0; i<=PS_MAX_PLAYERS; i++)
	{
		CStrW name = CStrW(L"diplomaticStance_") + CStrW(i);
		ISynchedJSProperty *prop=new CSynchedJSProperty<int>(name, &m_DiplomaticStance[i], this);
		m_SynchedProperties[name]=prop;
	}
}

CPlayer::~CPlayer()
{
	// Side-effect of HACKs - since these properties are not passed to CJSObject's list,
	// they don't get freed automatically

	delete m_SynchedProperties[L"colour"];
	
	for(int i=0; i<=PS_MAX_PLAYERS; i++)
	{
		CStrW name = CStrW(L"diplomaticStance_") + CStrW(i);
		delete m_SynchedProperties[name];
	}
}

void CPlayer::ScriptingInit()
{
	g_ScriptingHost.DefineConstant("DIPLOMACY_ENEMY", DIPLOMACY_ENEMY);
	g_ScriptingHost.DefineConstant("DIPLOMACY_NEUTRAL", DIPLOMACY_NEUTRAL);
	g_ScriptingHost.DefineConstant("DIPLOMACY_ALLIED", DIPLOMACY_ALLIED);

	AddMethod<CStrW, &CPlayer::JSI_ToString>("toString", 0);
	AddMethod<void, &CPlayer::JSI_SetColour>("setColour", 1);
	AddMethod<jsval_t, &CPlayer::JSI_GetColour>("getColour", 0);
	AddMethod<void, &CPlayer::JSI_SetDiplomaticStance>("setDiplomaticStance", 2);
	AddMethod<jsval_t, &CPlayer::JSI_GetDiplomaticStance>("getDiplomaticStance", 1);
	
	AddProperty( L"id", &CPlayer::m_PlayerID, true );
	AddProperty( L"active", &CPlayer::m_Active, true );
	AddProperty( L"name", &CPlayer::m_Name, true );
	// MT: Work out how this fits with the Synched stuff...

	// AddClassProperty( L"name", &CPlayer::m_Name );
	// AddClassProperty( L"colour", &CPlayer::m_Colour );
	AddProperty( L"controlled", &CPlayer::JSI_GetControlledEntities );

	CJSObject<CPlayer>::ScriptingInit( "Player" );
}

void CPlayer::Update(const CStrW& name, ISynchedJSProperty *prop)
{
	if (m_UpdateCB)
		m_UpdateCB(name, prop->ToString(), this, m_UpdateCBData);
}

void CPlayer::SetValue(const CStrW& name, const CStrW& value)
{
	ISynchedJSProperty *prop=GetSynchedProperty(name);
	if (prop)
	{
		prop->FromString(value);
	}
}

bool CPlayer::ValidateCommand(CNetMessage* UNUSED(pMsg))
{
	return true;
}

static bool ControllerPredicate( CEntity* entity, void* userdata )
{
	return( entity->GetPlayer() == userdata );
}

void CPlayer::GetControlledEntities(std::vector<HEntity>& controlled_entities)
{
	g_EntityManager.GetMatchingAsHandles( controlled_entities, ControllerPredicate, this );
}

CStrW CPlayer::JSI_ToString( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	return L"[object Player: " + m_Name + L"]";
}

jsval CPlayer::JSI_GetControlledEntities(JSContext* UNUSED(cx))
{
	std::vector<HEntity> controlledSet;
	GetControlledEntities(controlledSet);
	jsval vp = OBJECT_TO_JSVAL( EntityCollection::Create( controlledSet ) );
	return( vp );
}

void CPlayer::JSI_SetColour( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* argv )
{
	m_Colour=*( ToNative<SPlayerColour>(argv[0]) );
	ISynchedJSProperty *prop=GetSynchedProperty(L"colour");
	Update(L"colour", prop);
}

jsval_t CPlayer::JSI_GetColour( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	//ISynchedJSProperty *prop=GetSynchedProperty(L"colour");
	//return prop->Get(cx, this);
	SPlayerColour* col = &m_Colour;
	return ToJSVal(col);
}

void CPlayer::JSI_SetDiplomaticStance(JSContext *cx, uintN UNUSED(argc), jsval *argv)
{
	try
	{
		CPlayer* player = ToPrimitive<CPlayer*>( argv[0] );
		int stance = ToPrimitive<int>( argv[1] );
		if (! (stance==DIPLOMACY_ENEMY || stance==DIPLOMACY_NEUTRAL || stance==DIPLOMACY_ALLIED))
		{
			JS_ReportError(cx, "Argument 2 must be a valid stance ID");
			return;
		}

		m_DiplomaticStance[player->m_PlayerID] = (EDiplomaticStance) stance;
		
		CStrW name = CStrW(L"diplomaticStance_") + CStrW((unsigned)player->m_PlayerID);
		ISynchedJSProperty *prop=GetSynchedProperty(name);
		Update(name, prop);
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Could not convert argument 1 to a Player object" );
	}
}

jsval_t CPlayer::JSI_GetDiplomaticStance(JSContext *cx, uintN UNUSED(argc), jsval *argv)
{	
	try
	{
		CPlayer* player = ToPrimitive<CPlayer*>( argv[0] );
		return ToJSVal( (int) m_DiplomaticStance[player->m_PlayerID] );
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Could not convert argument 1 to a Player object" );
		return JSVAL_VOID;
	}
}
