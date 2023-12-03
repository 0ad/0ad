/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_ENTITY
#define INCLUDED_ENTITY

#include "lib/types.h"

class IComponent;

/**
 * Entity ID type.
 * ID numbers are never reused within a simulation run.
 */
typedef u32 entity_id_t;

/**
 * Invalid entity ID. Used as an error return value by some functions.
 * No valid entity will have this ID.
 */
const entity_id_t INVALID_ENTITY = 0;

/**
 * Entity ID for singleton 'system' components.
 * Use with QueryInterface to get the component instance.
 * (This allows those systems to make convenient use of the common infrastructure
 * for message-passing, scripting, serialisation, etc.)
 */
const entity_id_t SYSTEM_ENTITY = 1;

// Entities are split into two kinds:
// "Normal" (for most entities)
// "Local" (for entities that only exist on the local machine, aren't synchronised across the network,
// aren't retained in saved games, etc)
// The distinction is encoded in the entity ID, so that they're easily distinguished.
//
// We want all entity_id_ts to fit in an integer JS::Value, i.e. 1-2^30 .. 2^30-1 (inclusive)
// We want them to be unsigned ints (actually it shouldn't matter but unsigned seems simpler)
// We want 1 tag bit
// So we have 1 JS-reserved bit, 1 unused sign bit, 1 local tag bit, 29 counter bits
// (0.5B entities should be plenty)

#define ENTITY_TAGMASK (1 << 29)
#define ENTITY_IS_NORMAL(id) (((id) & ENTITY_TAGMASK) == 0)
#define ENTITY_IS_LOCAL(id) (((id) & ENTITY_TAGMASK) == ENTITY_TAGMASK)
const entity_id_t FIRST_LOCAL_ENTITY = ENTITY_TAGMASK;

struct SEntityComponentCache
{
	size_t numInterfaces;
	IComponent* interfaces[1]; /* variable length array */
};

/**
 * Object wrapping an entity_id_t, with a SEntityComponentCache to support fast
 * QueryInterface() / CmpPtr<>() calls.
 *
 * Components can use IComponent::GetSystemEntity() and IComponent::GetEntityHandle()
 * to get handles to SYSTEM_ENTITY and themselves, and then pass those handles around.
 *
 * Be careful not to store a CEntityHandle for longer than the lifetime of the entity
 * (listen to MT_Destroy messages to know when to release it) - otherwise the handle will
 * contain a dangling pointer and will probably crash.
 */
class CEntityHandle
{
public:
	CEntityHandle() : m_Id(INVALID_ENTITY), m_ComponentCache(NULL) { }
	CEntityHandle(entity_id_t id, SEntityComponentCache* componentCache)
		: m_Id(id), m_ComponentCache(componentCache)
	{
	}

	entity_id_t GetId() const { return m_Id; }
	SEntityComponentCache* GetComponentCache() const { return m_ComponentCache; }

private:
	entity_id_t m_Id;
	SEntityComponentCache* m_ComponentCache;
};

#endif // INCLUDED_ENTITY
