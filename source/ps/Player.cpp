#include "precompiled.h"

#include "Player.h"
#include "Network/NetMessage.h"
#include "Entity.h"
#include "EntityManager.h"
#include "scripting/JSCollection.h"
#include "simulation/LOSManager.h"

CPlayer::CPlayer(uint playerID):
	m_PlayerID(playerID),
	m_Name(CStrW(L"Player #")+CStrW(playerID)),
	m_Colour(0.7f, 0.7f, 0.7f),
	m_UpdateCB(0)
{
	m_LOSToken = LOS_GetTokenFor(playerID);

	AddSynchedProperty( L"name", &m_Name );
	// HACK - since we have to use setColour to update this, we don't want to
	// expose a colour property. Meanwhile, we want to have a property "colour"
	// available to be able to use the update/sync system.
	// So, this is only added to the SynchedProperties list and not also passed
	// to CJSObject's list
	ISynchedJSProperty *prop=new CSynchedJSProperty<SPlayerColour>(L"colour", &m_Colour, this);
	m_SynchedProperties[L"colour"]=prop;

}

CPlayer::~CPlayer()
{
	// Side-effect of HACK - since it's not passed to CJSObject's list, it
	// doesn't get freed automatically
	delete m_SynchedProperties[L"colour"];
}

void CPlayer::ScriptingInit()
{
	AddMethod<jsval, &CPlayer::JSI_ToString>( "toString", 0 );
	AddMethod<jsval, &CPlayer::JSI_SetColour>( "setColour", 1);
	
	AddProperty( L"id", &CPlayer::m_PlayerID, true );
	// MT: Work out how this fits with the Synched stuff...

	// AddClassProperty( L"name", &CPlayer::m_Name );
	// AddClassProperty( L"colour", &CPlayer::m_Colour );
	AddProperty( L"controlled", &CPlayer::JSI_GetControlledEntities );

	CJSObject<CPlayer>::ScriptingInit( "Player" );
}

void CPlayer::Update(CStrW name, ISynchedJSProperty *prop)
{
	if (m_UpdateCB)
		m_UpdateCB(name, prop->ToString(), this, m_UpdateCBData);
}

void CPlayer::SetValue(CStrW name, CStrW value)
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
