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

#ifndef INCLUDED_ICMPTERRITORYINFLUENCE
#define INCLUDED_ICMPTERRITORYINFLUENCE

#include "simulation2/system/Interface.h"

class ICmpTerritoryInfluence : public IComponent
{
public:
	/**
	 * Returns either -1 to indicate no special terrain cost, or a value
	 * in [0, 255] to indicate overriding the normal cost of the terrain
	 * under the entity's obstruction.
	 */
	virtual i32 GetCost() = 0;

	virtual u32 GetWeight() = 0;

	virtual u32 GetRadius() = 0;

	DECLARE_INTERFACE_TYPE(TerritoryInfluence)
};

#endif // INCLUDED_ICMPTERRITORYINFLUENCE
