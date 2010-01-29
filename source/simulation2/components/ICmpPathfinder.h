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

#include "simulation2/system/Position.h"

/**
 * Pathfinder algorithm.
 *
 * The pathfinder itself does not depend on other components. Instead, it contains an abstract
 * view of the game world, based a series of collision shapes (circles and squares), which is
 * updated by calls from other components (typically CCmpObstruction).
 *
 * Internally it quantises the shapes onto a grid and computes paths over the grid, but the interface
 * does not expose that detail.
 */
class ICmpPathfinder : public IComponent
{
public:
	/**
	 * Returned paths are currently represented as a series of waypoints.
	 * These happen to correspond to the centers of horizontally/vertically adjacent tiles
	 * along the path, but it's probably best not to rely on that.
	 */
	struct Waypoint
	{
		entity_pos_t x, z;
		u32 cost; // currently a meaningless number
	};

	struct Path
	{
		std::vector<Waypoint> m_Waypoints;
	};

	/**
	 * The pathfinder maintains an internal list of shapes; tags are the external identifiers of these
	 * shapes, allowing them to be manipulated and removed.
	 * Valid tags are guaranteed to be non-zero.
	 */
	typedef u32 tag_t;

	virtual tag_t AddCircle(entity_pos_t x, entity_pos_t z, entity_pos_t r) = 0;

	virtual tag_t AddSquare(entity_pos_t x, entity_pos_t z, entity_angle_t a, entity_pos_t w, entity_pos_t h) = 0;

	virtual void MoveShape(tag_t tag, entity_pos_t x, entity_pos_t z, entity_angle_t a) = 0;

	virtual void RemoveShape(tag_t tag) = 0;

	/**
	 * Compute a path between the given points, and draw the latest such path as a terrain overlay.
	 */
	virtual void SetDebugPath(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1) = 0;

	DECLARE_INTERFACE_TYPE(Pathfinder)
};

#endif // INCLUDED_ICMPPATHFINDER
