/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_ICMPFOOTPRINT
#define INCLUDED_ICMPFOOTPRINT

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"

class CFixedVector3D;

/**
 * Footprints - an approximation of the entity's shape, used for collision detection and for
 * rendering selection outlines.
 * A footprint is either a circle (of some radius) or square (of some width and depth (actually
 * it's a rectangle)), horizontally aligned, extruded to a given height.
 */
class ICmpFootprint : public IComponent
{
public:
	enum EShape
	{
		CIRCLE,
		SQUARE
	};

	/**
	 * Return the shape of this footprint.
	 * Shapes are horizontal circles or squares, extended vertically upwards to make cylinders or boxes.
	 * @param[out] shape either CIRCLE or SQUARE
	 * @param[out] size0 if CIRCLE then radius, else width (size in X axis)
	 * @param[out] size1 if CIRCLE then radius, else depth (size in Z axis)
	 * @param[out] height size in Y axis
	 */
	virtual void GetShape(EShape& shape, entity_pos_t& size0, entity_pos_t& size1, entity_pos_t& height) const = 0;

	/**
	 * GetShape wrapper for script calls.
	 * Returns { "type": "circle", "radius": 5.0, "height": 1.0 }
	 * or { "type": "square", "width": 5.0, "depth": 5.0, "height": 1.0 }
	 */
	JS::Value GetShape_wrapper() const;

	/**
	 * Pick a sensible position to place a newly-spawned entity near this footprint,
	 * such that it won't be in an invalid (obstructed) location regardless of the spawned unit's
	 * orientation.
	 * @return the X and Z coordinates of the spawn point, with Y = 0; or the special value (-1, -1, -1) if there's no space
	 */
	virtual CFixedVector3D PickSpawnPoint(entity_id_t spawned) const = 0;

	/**
	 * Pick a sensible position to place a newly-spawned entity near this footprint,
	 * at the intersection between the footprint passability and the entity one.
	 * @return the X and Z coordinates of the spawn point, with Y = 0; or the special value (-1, -1, -1) if there's no space
	 */
	virtual CFixedVector3D PickSpawnPointBothPass(entity_id_t spawned) const = 0;

	DECLARE_INTERFACE_TYPE(Footprint)
};

#endif // INCLUDED_ICMPFOOTPRINT
