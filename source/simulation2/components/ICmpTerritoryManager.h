/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ICMPTERRITORYMANAGER
#define INCLUDED_ICMPTERRITORYMANAGER

#include "simulation2/helpers/Grid.h"
#include "simulation2/system/Interface.h"
#include "simulation2/components/ICmpPosition.h"

class ICmpTerritoryManager : public IComponent
{
public:
	virtual bool NeedUpdate(size_t* dirtyID) = 0;

	virtual const Grid<u8>& GetTerritoryGrid() = 0;

	/**
	 * Get owner of territory at given position
	 *
	 * @return player ID of owner; 0 if neutral territory
	 */
	virtual int32_t GetOwner(entity_pos_t x, entity_pos_t z) = 0;

	DECLARE_INTERFACE_TYPE(TerritoryManager)
};

#endif // INCLUDED_ICMPTERRITORYMANAGER
