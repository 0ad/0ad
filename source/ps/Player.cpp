#include "precompiled.h"

#include "Player.h"
#include "network/NetMessage.h"
#include "simulation/Entity.h"
#include "simulation/EntityManager.h"
#include "ps/scripting/JSCollection.h"
#include "simulation/LOSManager.h"

CPlayer::CPlayer(uint playerID):
	m_PlayerID(playerID),
	m_Name(CStrW(L"Player #")+CStrW(playerID)),
	m_Civilization(L""),
	m_Colour(0.7f, 0.7f, 0.7f),
	m_UpdateCB(0)
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
		ISynchedJSProperty *prop=new CSynchedJSProperty<int>(name, (int*)&m_DiplomaticStance[i], this);
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

	AddMethod<jsval, &CPlayer::JSI_ToString>( "toString", 0 );
	AddMethod<jsval, &CPlayer::JSI_SetColour>( "setColour", 1);
	AddMethod<jsval, &CPlayer::JSI_GetColour>( "getColour", 0);
	AddMethod<jsval, &CPlayer::JSI_SetDiplomaticStance>( "setDiplomaticStance", 2);
	AddMethod<jsval, &CPlayer::JSI_GetDiplomaticStance>( "getDiplomaticStance", 1);
	
	AddProperty( L"id", &CPlayer::m_PlayerID, true );
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

std::vector<HEntity>* CPlayer::GetControlledEntities()
{
	return( g_EntityManager.matches( ControllerPredicate, this ) );
}

jsval CPlayer::JSI_ToString( JSContext* cx, uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object Player: %ls]", m_Name.c_str() );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}

jsval CPlayer::JSI_GetControlledEntities(JSContext* UNUSED(cx))
{
	std::vector<HEntity>* controlledSet = GetControlledEntities();
	jsval vp = OBJECT_TO_JSVAL( EntityCollection::Create( *controlledSet ) );
	delete( controlledSet );
	return( vp );
}

jsval CPlayer::JSI_SetColour( JSContext* UNUSED(cx), uintN argc, jsval* argv )
{
	if (argc != 1)
		return JSVAL_NULL;

	m_Colour=*( ToNative<SPlayerColour>(argv[0]) );
	ISynchedJSProperty *prop=GetSynchedProperty(L"colour");
	Update(L"colour", prop);
	
	// Return something that isn't null, so users can check whether this function succeeded
	return argv[0];
}

jsval CPlayer::JSI_GetColour( JSContext* UNUSED(cx), uintN UNUSED(argc), jsval* UNUSED(argv) )
{
	//ISynchedJSProperty *prop=GetSynchedProperty(L"colour");
	//return prop->Get(cx, this);
	SPlayerColour* col = &m_Colour;
	return ToJSVal(col);
}

jsval CPlayer::JSI_SetDiplomaticStance(JSContext *cx, uintN argc, jsval *argv)
{
	JSU_ASSERT(argc==2, "2 arguments required");
	JSU_ASSERT( JSVAL_IS_INT(argv[1]), "Argument 2 must be a valid stance ID" );
	try
	{
		CPlayer* player = ToPrimitive<CPlayer*>( argv[0] );
		int stance = ToPrimitive<int>( argv[1] );
		JSU_ASSERT( stance==DIPLOMACY_ENEMY || stance==DIPLOMACY_NEUTRAL || stance==DIPLOMACY_ALLIED,
			"Argument 2 must be a valid stance ID" );

		m_DiplomaticStance[player->m_PlayerID] = (EDiplomaticStance) stance;
		
		CStrW name = CStrW(L"diplomaticStance_") + CStrW(player->m_PlayerID);
		ISynchedJSProperty *prop=GetSynchedProperty(name);
		Update(name, prop);

		return JSVAL_VOID;
	}
	catch( PSERROR_Scripting_ConversionFailed )
	{
		JS_ReportError( cx, "Could not convert argument 1 to a Player object" );
		return JSVAL_VOID;
	}
}

jsval CPlayer::JSI_GetDiplomaticStance(JSContext *cx, uintN argc, jsval *argv)
{	
	JSU_ASSERT(argc==1, "1 argument required");
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
