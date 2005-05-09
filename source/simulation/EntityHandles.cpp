#include "precompiled.h"

#include "EntityHandles.h"
#include "EntityManager.h"
#include "CStr.h"

CHandle::CHandle()
{
	m_entity = NULL;
	m_refcount = 0;
}

HEntity::HEntity( u16 index )
{
	m_handle = index;
	addRef();
}

HEntity::HEntity( const HEntity& copy )
{
	m_handle = copy.m_handle;
	addRef();
}

HEntity::HEntity()
{
	m_handle = INVALID_HANDLE;
}

HEntity::~HEntity()
{
	if( CEntityManager::extant() )
		decRef();
}

void HEntity::operator=( const HEntity& copy )
{
	decRef();
	m_handle = copy.m_handle;
	addRef();
}

bool HEntity::operator ==( const HEntity& test ) const
{
	return( m_handle == test.m_handle );
}

HEntity::operator bool() const
{ 
	if( m_handle == INVALID_HANDLE )
		return( false );
	
	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( !g_EntityManager.m_entities[m_handle].m_entity->m_destroyed );
}

bool HEntity::operator!() const
{
	if( m_handle == INVALID_HANDLE )
		return( true );
	
	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity->m_destroyed );
}

void HEntity::addRef()
{
	if( m_handle != INVALID_HANDLE )
	{
		assert( m_handle < MAX_HANDLES );
		g_EntityManager.m_entities[m_handle].m_refcount++;
	}
}

void HEntity::decRef()
{
	if( m_handle != INVALID_HANDLE )
	{
		if( --g_EntityManager.m_entities[m_handle].m_refcount == 0 )
			g_EntityManager.destroy( m_handle );
	}
}

CEntity* HEntity::operator->() const
{
	assert( m_handle != INVALID_HANDLE );
	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity );
}

HEntity::operator CEntity*() const
{
	if( m_handle == INVALID_HANDLE )
		return( NULL );

	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity );
}
CEntity& HEntity::operator*() const
{
	assert( m_handle != INVALID_HANDLE );
	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( *g_EntityManager.m_entities[m_handle].m_entity );
}

uint HEntity::GetSerializedLength() const
{
	return 2;
}

u8 *HEntity::Serialize(u8 *buffer) const
{
	Serialize_int_2(buffer, m_handle);
	return buffer;
}

const u8 *HEntity::Deserialize(const u8 *buffer, const u8 *end)
{
	Deserialize_int_2(buffer, m_handle);
	// We can't let addRef assert just because someone sent us bogus data
	if (m_handle < MAX_HANDLES && m_handle != INVALID_HANDLE)
		addRef();
	return buffer;
}

HEntity::operator CStr() const
{
	char buf[16];
	sprintf(buf, "Entity#%04x", m_handle);
	return CStr(buf);
}
