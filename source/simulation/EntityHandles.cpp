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

void HEntity::addRef()
{
	if( m_handle != INVALID_HANDLE )
		g_EntityManager.m_entities[m_handle].m_refcount++;
}

void HEntity::decRef()
{
	if( m_handle != INVALID_HANDLE )
	{
		assert( g_EntityManager.m_entities[m_handle].m_refcount != 0 );
		if( --g_EntityManager.m_entities[m_handle].m_refcount == 0 )
		{
			delete( g_EntityManager.m_entities[m_handle].m_entity );
		}
	}
}

CEntity* HEntity::operator->()
{
	assert( m_handle != INVALID_HANDLE );
	assert( g_EntityManager.m_entities[m_handle].m_refcount != 0 );
	return( g_EntityManager.m_entities[m_handle].m_entity );
}

CEntity& HEntity::operator*()
{
	assert( m_handle != INVALID_HANDLE );
	assert( g_EntityManager.m_entities[m_handle].m_refcount != 0 );
	return( *g_EntityManager.m_entities[m_handle].m_entity );
}

