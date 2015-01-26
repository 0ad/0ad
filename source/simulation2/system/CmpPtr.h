/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_CMPPTR
#define INCLUDED_CMPPTR

#include "simulation2/system/Entity.h"

class CSimContext;
class CSimulation2;
class IComponent;

// Helper functions for CmpPtr
IComponent* QueryInterface(const CSimContext& context, entity_id_t ent, int iid);
IComponent* QueryInterface(const CSimulation2& simulation, entity_id_t ent, int iid);

/**
 * A simplified syntax for accessing entity components.
 * E.g. to get the @c Position component, write:
 *
 * @code
 * CmpPtr<ICmpPosition> cmpPosition(context, ent);
 * if (!cmpPosition)
 *     // do something (maybe just silently abort; you should never crash if the
 *     // component is missing, even if you're sure it should never be missing)
 * @endcode
 *
 * where @c context is (if you're writing component code) a CSimContext object, or
 * (if you're writing external engine code that makes use of the simulation system)
 * a CSimulation2 object; and @c ent is the entity ID.
 *
 * @c ent can be CComponentManager::SYSTEM_ENTITY (if you're writing a component), or
 * CSimulation2::SYSTEM_ENTITY (for external code), if you want to access the global
 * singleton system components.
 *
 * You should never hold onto a component pointer outside of the method in which you acquire
 * it, because it might get deleted and invalidate your pointer. (Components will never be
 * deleted while inside a simulation method.)
 */
template<typename T>
class CmpPtr
{
private:
	T* m;

public:
	CmpPtr(const CSimContext& context, entity_id_t ent)
	{
		m = static_cast<T*>(QueryInterface(context, ent, T::GetInterfaceId()));
	}

	CmpPtr(const CSimulation2& simulation, entity_id_t ent)
	{
		m = static_cast<T*>(QueryInterface(simulation, ent, T::GetInterfaceId()));
	}

	CmpPtr(CEntityHandle ent)
	{
		SEntityComponentCache* cache = ent.GetComponentCache();
		if (cache != NULL && T::GetInterfaceId() < (int)cache->numInterfaces)
			m = static_cast<T*>(cache->interfaces[T::GetInterfaceId()]);
		else
			m = NULL;
	}

	T* operator->() { return m; }

	explicit operator bool() const
	{
		return m != NULL;
	}
};

#endif // INCLUDED_CMPPTR
