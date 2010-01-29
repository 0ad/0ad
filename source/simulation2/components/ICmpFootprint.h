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

#ifndef INCLUDED_ICMPFOOTPRINT
#define INCLUDED_ICMPFOOTPRINT

#include "simulation2/system/Interface.h"

#include "simulation2/system/Position.h"

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

	virtual void GetShape(EShape& shape, entity_pos_t& size0, entity_pos_t& size1, entity_pos_t& height) = 0;

	DECLARE_INTERFACE_TYPE(Footprint)
};

#endif // INCLUDED_ICMPFOOTPRINT
