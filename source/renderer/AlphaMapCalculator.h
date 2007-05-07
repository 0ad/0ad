/**
 * =========================================================================
 * File        : AlphaMapCalculator.h
 * Project     : 0 A.D.
 * Description : Determine which alpha blend map fits a given shape.
 * =========================================================================
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
