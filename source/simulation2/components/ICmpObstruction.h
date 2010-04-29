/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_ICMPOBSTRUCTION
#define INCLUDED_ICMPOBSTRUCTION

#include "simulation2/system/Interface.h"

#include "simulation2/components/ICmpObstructionManager.h"

/**
 * Flags an entity as obstructing movement for other units,
 * and handles the processing of collision queries.
 */
class ICmpObstruction : public IComponent
{
public:

	virtual ICmpObstructionManager::tag_t GetObstruction() = 0;

	virtual entity_pos_t GetUnitRadius() = 0;

	/**
	 * Test whether this entity's footprint is colliding with any other's.
	 * @return true if there is a collision
	 */
	virtual bool CheckCollisions() = 0;

	DECLARE_INTERFACE_TYPE(Obstruction)
};

#endif // INCLUDED_ICMPOBSTRUCTION
