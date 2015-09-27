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

#include "precompiled.h"

#include "Rasterize.h"

#include "simulation2/helpers/Geometry.h"

void SimRasterize::RasterizeRectWithClearance(Spans& spans,
	const ICmpObstructionManager::ObstructionSquare& shape,
	entity_pos_t clearance, entity_pos_t cellSize)
{
	// Get the bounds of cells that might possibly be within the shape
	// (We'll then test each of those cells more precisely)
	CFixedVector2D halfSize(shape.hw + clearance, shape.hh + clearance);
	CFixedVector2D halfBound = Geometry::GetHalfBoundingBox(shape.u, shape.v, halfSize);
	i16 i0 = ((shape.x - halfBound.X) / cellSize).ToInt_RoundToNegInfinity();
	i16 j0 = ((shape.z - halfBound.Y) / cellSize).ToInt_RoundToNegInfinity();
	i16 i1 = ((shape.x + halfBound.X) / cellSize).ToInt_RoundToInfinity();
	i16 j1 = ((shape.z + halfBound.Y) / cellSize).ToInt_RoundToInfinity();

	if (j1 <= j0)
		return; // empty bounds - this shouldn't happen

	spans.reserve(j1 - j0);

	for (i16 j = j0; j < j1; ++j)
	{
		// Find the min/max range of cells that are strictly inside the square+clearance.
		// (Since the square+clearance is a convex shape, we can just test each
		// corner of each cell is inside the shape.)
		// (TODO: This potentially does a lot of redundant work.)
		i16 spanI0 = std::numeric_limits<i16>::max();
		i16 spanI1 = std::numeric_limits<i16>::min();
		for (i16 i = i0; i < i1; ++i)
		{
			if (Geometry::DistanceToSquare(
				CFixedVector2D(cellSize*i, cellSize*j) - CFixedVector2D(shape.x, shape.z),
				shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh), true) > clearance)
			{
				continue;
			}

			if (Geometry::DistanceToSquare(
				CFixedVector2D(cellSize*(i+1), cellSize*j) - CFixedVector2D(shape.x, shape.z),
				shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh), true) > clearance)
			{
				continue;
			}

			if (Geometry::DistanceToSquare(
				CFixedVector2D(cellSize*i, cellSize*(j+1)) - CFixedVector2D(shape.x, shape.z),
				shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh), true) > clearance)
			{
				continue;
			}

			if (Geometry::DistanceToSquare(
				CFixedVector2D(cellSize*(i+1), cellSize*(j+1)) - CFixedVector2D(shape.x, shape.z),
				shape.u, shape.v, CFixedVector2D(shape.hw, shape.hh), true) > clearance)
			{
				continue;
			}

			spanI0 = std::min(spanI0, i);
			spanI1 = std::max(spanI1, (i16)(i+1));
		}

		// Add non-empty spans onto the list
		if (spanI0 < spanI1)
		{
			Span span = { spanI0, spanI1, j };
			spans.push_back(span);
		}
	}
}
