/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "EntityHandles.h"
#include "EntityManager.h"
#include "Entity.h"
#include "ps/CStr.h"
#include "network/Serialization.h"

CHandle::CHandle()
{
	m_entity = NULL;
	m_refcount = 0;
}

HEntity::HEntity( size_t index )
{
	m_handle = (u16)index;
	AddRef();
}

HEntity::HEntity( const HEntity& copy )
{
	m_handle = copy.m_handle;
	AddRef();
}

HEntity::HEntity()
{
	m_handle = INVALID_HANDLE;
}

HEntity::~HEntity()
{
	if( CEntityManager::IsExtant() )
		DecRef();
}

void HEntity::operator=( const HEntity& copy )
{
	DecRef();
	m_handle = copy.m_handle;
	AddRef();
}

bool HEntity::operator ==( const HEntity& test ) const
{
	return( m_handle == test.m_handle );
}

bool HEntity::IsValid() const
{ 
	if( m_handle == INVALID_HANDLE )
		return( false );
	
	debug_assert( g_EntityManager.m_entities[m_handle].m_refcount );
	//return( !g_EntityManager.m_entities[m_handle].m_entity->entf_get(ENTF_DESTROYED) );
	return( true );
}

bool HEntity::IsAlive() const
{ 
	return( IsValid() && !g_EntityManager.m_entities[m_handle].m_entity->entf_get(ENTF_DESTROYED) );
}

void HEntity::AddRef()
{
	if( m_handle != INVALID_HANDLE )
	{
		debug_assert( m_handle < MAX_HANDLES );
		g_EntityManager.m_entities[m_handle].m_refcount++;
		g_EntityManager.m_refd[m_handle] = true;
	}
}

void HEntity::DecRef()
{
	if( m_handle != INVALID_HANDLE )
	{
		if( --g_EntityManager.m_entities[m_handle].m_refcount == 0 )
		{
			g_EntityManager.m_refd[m_handle] = false;
			g_EntityManager.Destroy( m_handle );
		}
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

size_t HEntity::GetSerializedLength() const
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
	// We can't let AddRef debug_assert just because someone sent us bogus data
	if (m_handle < MAX_HANDLES && m_handle != INVALID_HANDLE)
		AddRef();
	return buffer;
}

HEntity::operator CStr() const
{
	char buf[16];
	sprintf(buf, "Entity#%04x", m_handle);
	return CStr(buf);
}

size_t CEntityList::GetSerializedLength() const
{
	return (size_t)(2*(size()+1));
}

u8 *CEntityList::Serialize(u8 *buffer) const
{
	Serialize_int_2(buffer, size());
	for (size_t i=0; i<size(); i++)
		Serialize_int_2(buffer, at(i).m_handle);
	return buffer;
}

const u8 *CEntityList::Deserialize(const u8* buffer, const u8* UNUSED(end))
{
	const int MAX_SIZE = 500; // TODO: Don't allow selecting more than this many entities
	u16 size, handle;
	Deserialize_int_2(buffer, size);
	debug_assert(size > 0 && size < MAX_SIZE && "Inalid size when deserializing CEntityList");
	for (int i=0; i<size; i++)
	{
		Deserialize_int_2(buffer, handle);
		// We have to validate the data, or the HEntity constructor will debug_assert
		// on us.
		// FIXME We should also check that the entity actually exists
		if (handle < MAX_HANDLES)
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
