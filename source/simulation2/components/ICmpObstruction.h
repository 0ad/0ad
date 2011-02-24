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

	virtual bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out) = 0;

	virtual entity_pos_t GetUnitRadius() = 0;

	/**
	 * Test whether this entity is colliding with any obstruction that are set to
	 * block the creation of foundations.
	 * @return true if there is a collision
	 */
	virtual bool CheckFoundationCollisions() = 0;

	/**
	 * Returns a list of entities that are colliding with this entity, and that
	 * are set to block construction.
	 * @return true if there is a collision
	 */
	virtual std::vector<entity_id_t> GetConstructionCollisions() = 0;

	virtual void SetActive(bool active) = 0;

	virtual void SetMovingFlag(bool enabled) = 0;

	virtual void SetDisableBlockMovementPathfinding(bool disabled) = 0;

	/**
	 * Change the control group that the entity belongs to.
	 * Control groups are used to let units ignore collisions with other units from
	 * the same group. Default is the entity's own ID.
	 */
	virtual void SetControlGroup(entity_id_t group) = 0;

	DECLARE_INTERFACE_TYPE(Obstruction)
};

#endif // INCLUDED_ICMPOBSTRUCTION
