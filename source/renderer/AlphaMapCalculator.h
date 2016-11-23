/* Copyright (C) 2009 Wildfire Games.
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

/*
 * Determine which alpha blend map fits a given shape.
 */

#ifndef INCLUDED_ALPHAMAPCALCULATOR
#define INCLUDED_ALPHAMAPCALCULATOR

#include <string.h>
#include "BlendShapes.h"

// defines for blendmap flipping/rotating
#define	BLENDMAP_FLIPV		0x01
#define	BLENDMAP_FLIPU		0x02
#define	BLENDMAP_ROTATE90	0x04
#define	BLENDMAP_ROTATE180	0x08
#define	BLENDMAP_ROTATE270	0x10

///////////////////////////////////////////////////////////////////////////////
// CAlphaMapCalculator: functionality for calculating which alpha blend map
// fits a given shape
namespace CAlphaMapCalculator {
	// Calculate: return the index of the blend map that fits the given shape,
	// and the set of flip/rotation flags to get the shape correctly oriented
	int Calculate(BlendShape8 shape,unsigned int& flags);
}

#endif
