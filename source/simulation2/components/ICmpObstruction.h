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

	enum EFoundationCheck {
		FOUNDATION_CHECK_SUCCESS,
		FOUNDATION_CHECK_FAIL_ERROR,
		FOUNDATION_CHECK_FAIL_NO_OBSTRUCTION,
		FOUNDATION_CHECK_FAIL_OBSTRUCTS_FOUNDATION,
		FOUNDATION_CHECK_FAIL_TERRAIN_CLASS
	};

	virtual ICmpObstructionManager::tag_t GetObstruction() = 0;

	/**
	 * Gets the square corresponding to this obstruction shape.
	 * @return true and updates @p out on success;
	 *         false on failure (e.g. object not in the world).
	 */
	virtual bool GetObstructionSquare(ICmpObstructionManager::ObstructionSquare& out) = 0;

	/**
	 * Same as the method above, but returns an obstruction shape for the previous turn
	 */
	virtual bool GetPreviousObstructionSquare(ICmpObstructionManager::ObstructionSquare& out) = 0;

	virtual entity_pos_t GetSize() = 0;

	virtual entity_pos_t GetUnitRadius() = 0;

	virtual void SetUnitClearance(const entity_pos_t& clearance) = 0;

	virtual bool IsControlPersistent() = 0;

	/**
	 * Test whether this entity is colliding with any obstruction that are set to
	 * block the creation of foundations.
	 * @param ignoredEntities List of entities to ignore during the test.
	 * @return FOUNDATION_CHECK_SUCCESS if check passes, else an EFoundationCheck
	 *	value describing the type of failure.
	 */
	virtual EFoundationCheck CheckFoundation(std::string className) = 0;
	virtual EFoundationCheck CheckFoundation(std::string className, bool onlyCenterPoint) = 0;

	/**
	 * CheckFoundation wrapper for script calls, to return friendly strings instead of an EFoundationCheck.
	 * @return "success" if check passes, else a string describing the type of failure.
	 */
	virtual std::string CheckFoundation_wrapper(const std::string& className, bool onlyCenterPoint);

	/**
	 * Test whether this entity is colliding with any obstructions that share its
	 * control groups and block the creation of foundations.
	 * @return true if foundation is valid (not obstructed)
	 */
	virtual bool CheckDuplicateFoundation() = 0;

	/**
	 * Returns a list of units that are colliding with this entity,
	 * @return vector of blocking units
	 */
	virtual std::vector<entity_id_t> GetUnitCollisions() = 0;

	/**
	 * Detects collisions between foundation-blocking entities and
	 * tries to fix them by setting control groups, if appropriate.
	 */
	virtual void ResolveFoundationCollisions() = 0;

	virtual void SetActive(bool active) = 0;

	virtual void SetMovingFlag(bool enabled) = 0;

	virtual void SetDisableBlockMovementPathfinding(bool movementDisabled, bool pathfindingDisabled, int32_t shape) = 0;

	virtual bool GetBlockMovementFlag() = 0;

	/**
	 * Change the control group that the entity belongs to.
	 * Control groups are used to let units ignore collisions with other units from
	 * the same group. Default is the entity's own ID.
	 */
	virtual void SetControlGroup(entity_id_t group) = 0;

	/// See SetControlGroup.
	virtual entity_id_t GetControlGroup() = 0;

	virtual void SetControlGroup2(entity_id_t group2) = 0;
	virtual entity_id_t GetControlGroup2() = 0;

	DECLARE_INTERFACE_TYPE(Obstruction)
};

#endif // INCLUDED_ICMPOBSTRUCTION
