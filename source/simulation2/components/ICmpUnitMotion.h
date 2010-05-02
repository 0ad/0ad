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

#ifndef INCLUDED_ICMPUNITMOTION
#define INCLUDED_ICMPUNITMOTION

#include "simulation2/system/Interface.h"

#include "ICmpPosition.h" // for entity_pos_t

/**
 * Motion interface for entities with complex movement capabilities.
 * (Simpler motion is handled by ICmpMotion instead.)
 *
 * Currently this is limited to telling the entity to walk to a point.
 * Eventually it should support different movement speeds, moving to areas
 * instead of points, moving as part of a group, moving as part of a formation,
 * etc.
 */
class ICmpUnitMotion : public IComponent
{
public:
	/**
	 * Attempt to walk to a given point, or as close as possible.
	 * If the unit cannot move anywhere at all, or if there is some other error, then
	 * returns false.
	 * Otherwise, sends a MotionChanged message and returns true; it will send another
	 * MotionChanged message (with speed 0) once it has reached the target or otherwise
	 * given up trying to reach it.
	 */
	virtual bool MoveToPoint(entity_pos_t x, entity_pos_t z) = 0;

	/**
	 * Determine whether the target is within the given range, using the same measurement
	 * as MoveToAttackRange.
	 */
	virtual bool IsInAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) = 0;

	/**
	 * Attempt to walk into range of a given target, or as close as possible.
	 * If the unit is already in range, or cannot move anywhere at all, or if there is
	 * some other error, then returns false.
	 * Otherwise, sends a MotionChanged message and returns true; it will send another
	 * MotionChanged message (with speed 0) once it has reached the target range (such that
	 * IsInAttackRange should return true) or otherwise given up trying to reach it.
	 */
	virtual bool MoveToAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) = 0;

	/**
	 * Get the default speed that this unit will have when walking, in metres per second.
	 */
	virtual fixed GetSpeed() = 0;

	/**
	 * Toggle the rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	DECLARE_INTERFACE_TYPE(UnitMotion)
};

#endif // INCLUDED_ICMPUNITMOTION
