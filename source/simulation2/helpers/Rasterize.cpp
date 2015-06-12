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
	CFixedVector2D halfSize(shape.hw, shape.hh);
	CFixedVector2D halfBound = Geometry::GetHalfBoundingBox(shape.u, shape.v, halfSize);
	// add 1 to at least have 1 tile out of reach
	i16 iMax = (i16)( (halfBound.X + clearance) / cellSize).ToInt_RoundToInfinity() + 1;
	i16 jMax = (i16)( (halfBound.Y + clearance) / cellSize).ToInt_RoundToInfinity() + 1;

	i16 offsetX = (i16)(shape.x / cellSize).ToInt_RoundToNearest();
	i16 offsetZ = (i16)(shape.z / cellSize).ToInt_RoundToNearest();

	i16 i0 = -iMax;
	i16 i1 = iMax;

	if (jMax <= 0)
		return; // empty bounds - this shouldn't happen

	spans.reserve(jMax * 2);

	// TODO: Compare the squared distance to avoid sqrting
#define IS_IN_SQUARE(i, j) (Geometry::DistanceToSquare(CFixedVector2D(cellSize*i, cellSize*j),	shape.u, shape.v, halfSize, true) <= clearance)

	// The rasterization is finished when for one row, all columns are visited and
	// no tile in-range is found.
	bool finished = false;

	// Loop over half of the rows
	// Other rows can be added easily due to rectangle symmetry
	// For each row, search the outer bounds, using the bounds of the previous row
	// as an estimation
	for (i16 j = 0; j <= jMax; ++j)
	{
		bool foundI0 = false;
		// check if the estimation is in or out the square, and move accordingly
		bool isI0InSquare = IS_IN_SQUARE(i0, j);
		while (!foundI0 && !finished)
		{
			if (isI0InSquare && !IS_IN_SQUARE(--i0, j))
			{
				foundI0 = true;
				++i0; // add one to bring i0 back in the square
			}
			else if (!isI0InSquare && IS_IN_SQUARE(++i0, j))
				foundI0 = true;

			// when this row has no obstructions, we're done
			if (i0 > iMax)
				finished = true;
			ENSURE(i0 >= -iMax);
		}

		if (finished)
			break;

		bool foundI1 = false;
		// check if the estimation is in or out the square, and move accordingly
		bool isI1InSquare = IS_IN_SQUARE(i1, j);
		while (!foundI1)
		{
			if (isI1InSquare && !IS_IN_SQUARE(++i1, j))
			{
				foundI1 = true;
				--i1; // subtract 1 to bring i1 back in the square
			}
			else if (!isI1InSquare && IS_IN_SQUARE(--i1, j))
				foundI1 = true;
			// this row will have obstructions, or we will have stopped earlier
			ENSURE(i1 >= i0 && i1 <= iMax);
		}

		spans.emplace_back(Span{ (i16)(offsetX + i0), (i16)(offsetX + i1), (i16)(offsetZ + j) });
		// add symmetrical row from j == 1 onwards
		if (j > 0)
			spans.emplace_back(Span{ (i16)(offsetX - i1), (i16)(offsetX - i0), (i16)(offsetZ - j) });
	}

	// ensure that the entire bound was found
	ENSURE(finished);
#undef IS_IN_SQUARE
}
