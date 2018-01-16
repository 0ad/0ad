/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_PATHGOAL
#define INCLUDED_PATHGOAL

#include "maths/FixedVector2D.h"
#include "simulation2/helpers/Position.h"

/**
 * Pathfinder goal.
 * The goal can be either a point, a circle, or a square (rectangle).
 * For circles/squares, any point inside the shape is considered to be
 * part of the goal.
 * Also, it can be an 'inverted' circle/square, where any point outside
 * the shape is part of the goal.
 */
class PathGoal
{
public:
	enum Type {
		POINT,           // single point
		CIRCLE,          // the area inside a circle
		INVERTED_CIRCLE, // the area outside a circle
		SQUARE,          // the area inside a square
		INVERTED_SQUARE  // the area outside a square
	} type;

	entity_pos_t x, z; // position of center

	entity_pos_t hw, hh; // if [INVERTED_]SQUARE, then half width & height; if [INVERTED_]CIRCLE, then hw is radius

	CFixedVector2D u, v; // if [INVERTED_]SQUARE, then orthogonal unit axes

	entity_pos_t maxdist; // maximum distance wanted between two path waypoints

	/**
	 * Returns true if the given navcell contains a part of the goal area.
	 */
	bool NavcellContainsGoal(int i, int j) const;

	/**
	 * Returns true if any navcell (i, j) where
	 * min(i0,i1) <= i <= max(i0,i1)
	 * min(j0,j1) <= j <= max(j0,j1),
	 * contains a part of the goal area.
	 * If so, arguments i and j (if not NULL) are set to the goal navcell nearest
	 * to (i0, j0), assuming the rect has either width or height = 1.
	 */
	bool NavcellRectContainsGoal(int i0, int j0, int i1, int j1, int* i, int* j) const;

	/**
	 * Returns true if the rectangle defined by (x0,z0)-(x1,z1) (inclusive)
	 * contains a part of the goal area.
	 */
	bool RectContainsGoal(entity_pos_t x0, entity_pos_t z0, entity_pos_t x1, entity_pos_t z1) const;

	/**
	 * Returns the minimum distance from the point pos to any point on the goal shape.
	 */
	fixed DistanceToPoint(CFixedVector2D pos) const;

	/**
	 * Returns the coordinates of the point on the goal that is closest to the point pos.
	 */
	CFixedVector2D NearestPointOnGoal(CFixedVector2D pos) const;
};

#endif // INCLUDED_PATHGOAL
