#include "precompiled.h"

#include "Player.h"
#include "Network/NetMessage.h"
#include "Entity.h"
#include "EntityManager.h"
#include "scripting/JSCollection.h"

CPlayer::CPlayer(uint playerID):
	m_PlayerID(playerID),
	m_Name(CStrW(L"Player #")+CStrW(playerID)),
	m_Colour(0.7f, 0.7f, 0.7f),
	m_UpdateCB(0)
{
	AddReadOnlyProperty( L"id", &m_PlayerID );
	AddProperty( L"controlled", (IJSObject::GetFn)&CPlayer::GetControlledEntities_JS );
	AddSynchedProperty( L"name", &m_Name );
	AddSynchedProperty( L"colour", &m_Colour );
}

void CPlayer::Update(CStrW name, ISynchedJSProperty *prop)
{
	if (m_UpdateCB)
		m_UpdateCB(name, prop->ToString(), this, m_UpdateCBData);
}

bool CPlayer::ValidateCommand(CNetMessage *pMsg)
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

jsval CPlayer::ToString( JSContext* cx, uintN argc, jsval* argv )
{
	wchar_t buffer[256];
	swprintf( buffer, 256, L"[object Player: %ls]", m_Name.c_str() );
	buffer[255] = 0;
	utf16string str16(buffer, buffer+wcslen(buffer));
	return( STRING_TO_JSVAL( JS_NewUCStringCopyZ( cx, str16.c_str() ) ) );
}

jsval CPlayer::GetControlledEntities_JS()
{
	std::vector<HEntity>* controlledSet = GetControlledEntities();
	jsval vp = OBJECT_TO_JSVAL( EntityCollection::Create( *controlledSet ) );
	delete( controlledSet );
	return( vp );
}

void CPlayer::ScriptingInit()
{
	AddMethod<jsval, &CPlayer::ToString>( "toString", 0 );
	CJSObject<CPlayer>::ScriptingInit( "Player" );
}
