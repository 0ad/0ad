#include "precompiled.h"

#include "EntityHandles.h"
#include "EntityManager.h"
#include "ps/CStr.h"

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
	
	debug_assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( !g_EntityManager.m_entities[m_handle].m_entity->m_destroyed );
}

bool HEntity::operator!() const
{
	if( m_handle == INVALID_HANDLE )
		return( true );
	
	debug_assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity->m_destroyed );
}

void HEntity::addRef()
{
	if( m_handle != INVALID_HANDLE )
	{
		debug_assert( m_handle < MAX_HANDLES );
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
	debug_assert( m_handle != INVALID_HANDLE );
	debug_assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity );
}

HEntity::operator CEntity*() const
{
	if( m_handle == INVALID_HANDLE )
		return( NULL );

	debug_assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( g_EntityManager.m_entities[m_handle].m_entity );
}
CEntity& HEntity::operator*() const
{
	debug_assert( m_handle != INVALID_HANDLE );
	debug_assert( g_EntityManager.m_entities[m_handle].m_refcount );
	return( *g_EntityManager.m_entities[m_handle].m_entity );
}

uint HEntity::GetSerializedLength() const
{
	return 2;
}

u8 *HEntity::Serialize(u8* buffer) const
{
	Serialize_int_2(buffer, m_handle);
	return buffer;
}

const u8* HEntity::Deserialize(const u8* buffer, const u8* UNUSED(end))
{
	Deserialize_int_2(buffer, m_handle);
	// We can't let addRef debug_assert just because someone sent us bogus data
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

uint CEntityList::GetSerializedLength() const
{
	return (uint)(2*size());
}

u8 *CEntityList::Serialize(u8 *buffer) const
{
	for (size_t i=0;i<(size()-1);i++)
		Serialize_int_2(buffer, at(i).m_handle);
	Serialize_int_2(buffer, back().m_handle | HANDLE_SENTINEL_BIT);
	return buffer;
}

const u8 *CEntityList::Deserialize(const u8* buffer, const u8* UNUSED(end))
{
	u16 n=0, handle;
	while (!(n & HANDLE_SENTINEL_BIT))
	{
		Deserialize_int_2(buffer, n);
		handle = n & ~HANDLE_SENTINEL_BIT;
		// We have to validate the data, or the HEntity constructor will debug_assert
		// on us.
		// FIXME 4096 shouldn't be hard-coded
		// FIXME We should also check that the entity actually exists
		if (handle < 4096 && handle != INVALID_HANDLE)
			push_back(HEntity(handle));
	}
	return buffer;
}

CEntityList::operator CStr() const
{
	if (size() == 0)
	{
		return CStr("Entities {}");
	}
	else if (size() == 1)
	{
		return front().operator CStr();
	}
	else
	{
		CStr buf="{ ";
		buf += front().operator CStr();
		for (const_iterator it=begin();it != end();++it)
		{
			buf += ", ";
			buf += it->operator CStr();
		}
		buf += " }";
		return buf;
	}
}
