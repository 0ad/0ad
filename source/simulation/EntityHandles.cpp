#include "precompiled.h"

#include "EntityHandles.h"
#include "EntityManager.h"

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

void HEntity::addRef()
{
	if( m_handle != INVALID_HANDLE )
		g_EntityManager.m_entities[m_handle].m_refcount++;
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
	assert( m_handle != INVALID_HANDLE );
	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity );
}
CEntity& HEntity::operator*() const
{
	assert( m_handle != INVALID_HANDLE );
	assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( *g_EntityManager.m_entities[m_handle].m_entity );
}

