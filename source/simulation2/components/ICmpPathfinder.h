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

#ifndef INCLUDED_ICMPPATHFINDER
#define INCLUDED_ICMPPATHFINDER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"

#include "maths/FixedVector2D.h"

#include <vector>

class IObstructionTestFilter;

/**
 * Pathfinder algorithms.
 *
 * There are two different modes: a tile-based pathfinder that works over long distances and
 * accounts for terrain costs but ignore units, and a 'short' vertex-based pathfinder that
 * provides precise paths and avoids other units.
 *
 * Both use the same concept of a Goal: either a point, circle or square.
 * (If the starting point is inside the goal shape then the path will move outwards
 * to reach the shape's outline.)
 *
 * The output is a list of waypoints.
 */
class ICmpPathfinder : public IComponent
{
public:
	struct Goal
	{
		enum {
			POINT,
			CIRCLE,
			SQUARE
		} type;
		entity_pos_t x, z; // position of center
		CFixedVector2D u, v; // if SQUARE, then orthogonal unit axes
		entity_pos_t hw, hh; // if SQUARE, then half width & height; if CIRCLE, then hw is radius
	};

	struct Waypoint
	{
		entity_pos_t x, z;
	};

	/**
	 * Returned path.
	 * Waypoints are in *reverse* order (the earliest is at the back of the list)
	 */
	struct Path
	{
		std::vector<Waypoint> m_Waypoints;
	};

	/**
	 * Get the list of all known passability class names.
	 */
	virtual std::vector<std::string> GetPassabilityClasses() = 0;

	/**
	 * Get the tag for a given passability class name.
	 * Logs an error and returns something acceptable if the name is unrecognised.
	 */
	virtual u8 GetPassabilityClass(const std::string& name) = 0;

	/**
	 * Get the tag for a given movement cost class name.
	 * Logs an error and returns something acceptable if the name is unrecognised.
	 */
	virtual u8 GetCostClass(const std::string& name) = 0;

	/**
	 * Compute a tile-based path from the given point to the goal, and return the set of waypoints.
	 * The waypoints correspond to the centers of horizontally/vertically adjacent tiles
	 * along the path.
	 */
	virtual void ComputePath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass, Path& ret) = 0;

	/**
	 * If the debug overlay is enabled, render the path that will computed by ComputePath.
	 */
	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, const Goal& goal, u8 passClass, u8 costClass) = 0;

	/**
	 * Compute a precise path from the given point to the goal, and return the set of waypoints.
	 * The path is based on the full set of obstructions that pass the filter, such that
	 * a unit of radius 'r' will be able to follow the path with no collisions.
	 * The path is restricted to a box of radius 'range' from the starting point.
	 */
	virtual void ComputeShortPath(const IObstructionTestFilter& filter, entity_pos_t x0, entity_pos_t z0, entity_pos_t r, entity_pos_t range, const Goal& goal, u8 passClass, Path& ret) = 0;

	/**
	 * Find the speed factor (typically around 1.0) for a unit of the given cost class
	 * at the given position.
	 */
	virtual fixed GetMovementSpeed(entity_pos_t x0, entity_pos_t z0, u8 costClass) = 0;

	/**
	 * Toggle the storage and rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	DECLARE_INTERFACE_TYPE(Pathfinder)
};

#endif // INCLUDED_ICMPPATHFINDER
