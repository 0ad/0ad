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

#include "precompiled.h"

#include "AlphaMapCalculator.h"
#include <string.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// CAlphaMapCalculator: functionality for calculating which alpha blend map
// fits a given shape
namespace CAlphaMapCalculator {	

///////////////////////////////////////////////////////////////////////////////
// Blend4: structure mapping a blend shape for N,E,S,W to a particular map
struct Blend4 {
	Blend4(BlendShape4 shape,int alphamap) : m_Shape(shape), m_AlphaMap(alphamap) {}

	BlendShape4 m_Shape;
	int m_AlphaMap;
};

///////////////////////////////////////////////////////////////////////////////
// Blend8: structure mapping a blend shape for N,NE,E,SE,S,SW,W,NW to a
// particular map
struct Blend8 {
	Blend8(BlendShape8 shape,int alphamap) : m_Shape(shape), m_AlphaMap(alphamap) {}

	BlendShape8 m_Shape;
	int m_AlphaMap;
};

///////////////////////////////////////////////////////////////////////////////
// Data tables for mapping between shapes and blend maps
///////////////////////////////////////////////////////////////////////////////

const Blend4 Blends1Neighbour[] =
{
	Blend4(BlendShape4(1,0,0,0), 12)
};


const Blend4 Blends2Neighbour[] =
{
	Blend4(BlendShape4(0,1,1,0), 7),
	Blend4(BlendShape4(1,0,1,0), 10)
};

const Blend8 Blends2Neighbour8[] =
{
	Blend8(BlendShape8(1,1,0,0,0,0,0,0), 12),
	Blend8(BlendShape8(1,0,0,0,0,1,0,0), 12),
	Blend8(BlendShape8(0,1,0,1,0,0,0,0), 0) ,
	Blend8(BlendShape8(0,1,0,0,0,1,0,0), 0)
};

const Blend4 Blends3Neighbour[] =
{
	Blend4(BlendShape4(1,1,1,0), 4)
};

const Blend8 Blends3Neighbour8[] =
{
	Blend8(BlendShape8(1,1,0,0,1,0,0,0), 10),
	Blend8(BlendShape8(1,1,0,0,0,0,0,1), 12),
	Blend8(BlendShape8(1,1,1,0,0,0,0,0), 1),
	Blend8(BlendShape8(0,1,1,0,1,0,0,0), 7),
	Blend8(BlendShape8(0,0,1,0,1,0,1,0), 4),
	Blend8(BlendShape8(1,1,0,0,0,1,0,0), 12),
	Blend8(BlendShape8(1,1,0,1,0,0,0,0), 12),
	Blend8(BlendShape8(0,0,1,0,1,0,0,1), 7),
	Blend8(BlendShape8(1,0,0,1,0,1,0,0), 12),
	Blend8(BlendShape8(0,1,0,1,0,1,0,0), 0)
};

const Blend8 Blends4Neighbour8[] =
{
	Blend8(BlendShape8(1,1,0,0,1,0,0,1), 10),
	Blend8(BlendShape8(1,1,0,1,1,0,0,0), 10),
	Blend8(BlendShape8(1,1,0,0,1,1,0,0), 10),
	Blend8(BlendShape8(1,1,0,1,0,0,0,1), 12),
	Blend8(BlendShape8(0,1,1,0,1,1,0,0), 7),
	Blend8(BlendShape8(1,1,1,1,0,0,0,0), 1),
	Blend8(BlendShape8(1,1,1,0,1,0,0,0), 3),
	Blend8(BlendShape8(0,0,1,0,1,1,0,1), 7),
	Blend8(BlendShape8(1,0,1,0,1,1,0,0), 4),
	Blend8(BlendShape8(1,1,1,0,0,1,0,0), 1),
	Blend8(BlendShape8(1,1,0,1,0,1,0,0), 12),
	Blend8(BlendShape8(0,1,0,1,0,1,0,1), 0)
};

const Blend8 Blends5Neighbour8[] =
{
	Blend8(BlendShape8(1,1,1,1,1,0,0,0), 2),
	Blend8(BlendShape8(1,1,1,1,0,0,0,1), 1),
	Blend8(BlendShape8(1,1,1,0,1,0,0,1), 3),
	Blend8(BlendShape8(1,1,1,0,1,0,1,0), 11),
	Blend8(BlendShape8(1,1,1,0,0,1,0,1), 1),
	Blend8(BlendShape8(1,1,0,1,1,1,0,0), 10),
	Blend8(BlendShape8(1,1,1,0,1,1,0,0), 3),
	Blend8(BlendShape8(1,0,1,0,1,1,0,1), 4),
	Blend8(BlendShape8(1,1,0,1,0,1,0,1), 12),
	Blend8(BlendShape8(0,1,1,0,1,1,0,1), 7)
};

const Blend8 Blends6Neighbour8[] =
{
	Blend8(BlendShape8(1,1,1,1,1,1,0,0), 2),
	Blend8(BlendShape8(1,1,1,1,1,0,1,0), 8),
	Blend8(BlendShape8(1,1,1,1,0,1,0,1), 1),
	Blend8(BlendShape8(1,1,1,0,1,1,1,0), 6),
	Blend8(BlendShape8(1,1,1,0,1,1,0,1), 3),
	Blend8(BlendShape8(1,1,0,1,1,1,0,1), 10)
};

const Blend8 Blends7Neighbour8[] =
{
	Blend8(BlendShape8(1,1,1,1,1,1,0,1), 2),
	Blend8(BlendShape8(1,1,1,1,1,1,1,0), 9)
};

///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// MatchBlendShapeFlipped: test if the given shape can be made to fit the
// template in either unflipped state, or by flipping the shape in U or V
template<class T>
bool MatchBlendShapeFlipped(const T& templateshape,const T& shape,unsigned int& flags)
{
	// test unrotated shape
	if (shape==templateshape) {
		return true;
	}

	// test against shape flipped in U
	T tstShape;
	templateshape.FlipU(tstShape);
	if (shape==tstShape) {
		flags|=BLENDMAP_FLIPU;
		return true;
	}

	// test against shape flipped in V
	templateshape.FlipV(tstShape);
	if (shape==tstShape) {
		flags|=BLENDMAP_FLIPV;
		return true;
	}
	
	// no joy; no match by flipping
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// MatchBlendShape: try and find a matching blendmap, and the required flip/
// rotation flags, to fit the given shape to the template
template<class T>
int MatchBlendShape(const T& templateshape,const T& shape,unsigned int& flags)
{
	// try matching unrotated shape first using just flipping
	if (MatchBlendShapeFlipped(templateshape,shape,flags)) {
		return true;
	}

	// now try iterating through rotations of 90,180,270 degrees
	T tstShape;
	templateshape.Rotate90(tstShape);
	if (MatchBlendShapeFlipped(tstShape,shape,flags)) {
		// update flags - note if we've flipped in u or v, we need to rotate in
		// the opposite direction
		flags|=flags ? BLENDMAP_ROTATE270 : BLENDMAP_ROTATE90;
		return true;
	}
	
	templateshape.Rotate180(tstShape);
	if (MatchBlendShapeFlipped(tstShape,shape,flags)) {
		flags|=BLENDMAP_ROTATE180;
		return true;
	}

	templateshape.Rotate270(tstShape);
	if (MatchBlendShapeFlipped(tstShape,shape,flags)) {
		// update flags - note if we've flipped in u or v, we need to rotate in
		// the opposite direction
		flags|=flags ? BLENDMAP_ROTATE90 : BLENDMAP_ROTATE270;
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
// LookupBlend: find and return the blendmap fitting the given shape by
// iterating through the given data table and testing each shape in flipped and
// rotated forms until a match is found
template<class S,class T>
int LookupBlend(int tableSize,const S* table,const T& shape,unsigned int& flags)
{
	// iterate through known blend shapes
	for (int b=0;b<tableSize;b++) {
		const S& blend=table[b];
		if (MatchBlendShape(blend.m_Shape,shape,flags)) {
			return blend.m_AlphaMap;
		}
	}

	// eh? shouldn't get here if we've correctly considered all possible cases;
	// keep the compiler happy, and, while we're still debugging possible shapes,
	// return bad blend to highlight suspect alphamap logic
	return 13;
}


///////////////////////////////////////////////////////////////////////////////
// Calculate: return the index of the blend map that fits the given shape,
// and the set of flip/rotation flags to get the shape correctly oriented
int Calculate(BlendShape8 shape,unsigned int& flags)
{	
	// assume we're not going to require flipping or rotating
	flags=0;

	// count number of neighbours
	int count=0;
	for (int i=0;i<8;i++) {
		if (shape[i]) count++;
	}

	if (count==0) {
		// no neighbours, just the centre tile has the given texture; use blend circle
		return 0;
	} else if (count==8) {
		// all neighbours have same texture; return code to signal no alphamap required
		return -1;
	} else {
		if (count<=4) {
			// check if we can consider this a BlendShape4 - ie are any of the diagonals (NE,SE,SW,NW) set?
			if (!shape[1] && !shape[3] && !shape[5] && !shape[7]) {
				// ok, build a BlendShape4 and use that
				BlendShape4 shape4;
				shape4[0]=shape[0];
				shape4[1]=shape[2];
				shape4[2]=shape[4];
				shape4[3]=shape[6];

				switch (count) {
					case 1:
						return LookupBlend(sizeof(Blends1Neighbour)/sizeof(Blend4),Blends1Neighbour,shape4,flags);		

					case 2:
						return LookupBlend(sizeof(Blends2Neighbour)/sizeof(Blend4),Blends2Neighbour,shape4,flags);		

					case 3:
						return LookupBlend(sizeof(Blends3Neighbour)/sizeof(Blend4),Blends3Neighbour,shape4,flags);		

					case 4:
						// N,S,E,W have same texture, NE,SE,SW,NW don't; use a blend 4 corners
						return 5;
				}
			}
		}


		// we've got this far, so now we've got to consider the remaining choices, all containing
		// diagonal elements
		switch (count) {
			case 1:
				// trivial case - just return a circle blend
				return 0;

			case 2:
				return LookupBlend(sizeof(Blends2Neighbour8)/sizeof(Blend8),Blends2Neighbour8,shape,flags);		

			case 3:
				return LookupBlend(sizeof(Blends3Neighbour8)/sizeof(Blend8),Blends3Neighbour8,shape,flags);		

			case 4:
				return LookupBlend(sizeof(Blends4Neighbour8)/sizeof(Blend8),Blends4Neighbour8,shape,flags);		

			case 5:
				return LookupBlend(sizeof(Blends5Neighbour8)/sizeof(Blend8),Blends5Neighbour8,shape,flags);		

			case 6:
				return LookupBlend(sizeof(Blends6Neighbour8)/sizeof(Blend8),Blends6Neighbour8,shape,flags);		

			case 7:
				return LookupBlend(sizeof(Blends7Neighbour8)/sizeof(Blend8),Blends7Neighbour8,shape,flags);		
		}

	}

	// Shouldn't get here if we've correctly considered all possible cases;
	// keep the compiler happy, and, while we're still debugging possible shapes,
	// return bad blend to highlight suspect alphamap logic
	return 13;
}

} // end of namespace
