/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_HELPER_RASTERIZE
#define INCLUDED_HELPER_RASTERIZE

/**
 * @file
 * Helper functions related to rasterizing geometric shapes to grids.
 */

#include "simulation2/components/ICmpObstructionManager.h"

namespace SimRasterize
{

/**
 * Represents the set of cells (i,j) where i0 <= i < i1
 */
struct Span
{
	i16 i0;
	i16 i1;
	i16 j;
};

typedef std::vector<Span> Spans;

/**
 * Converts an ObstructionSquare @p shape (a rotated rectangle),
 * expanded by the given @p clearance,
 * into a list of spans of cells that are strictly inside the shape.
 */
void RasterizeRectWithClearance(Spans& spans,
	const ICmpObstructionManager::ObstructionSquare& shape,
	entity_pos_t clearance, entity_pos_t cellSize);

}

#endif // INCLUDED_HELPER_RASTERIZE
