#include "AlphaMapCalculator.h"
#include <string.h>
#include <stdio.h>

namespace CAlphaMapCalculator {	

struct Blend4 {
	Blend4(BlendShape4 shape,int alphamap) : m_Shape(shape), m_AlphaMap(alphamap) {}

	BlendShape4 m_Shape;
	int m_AlphaMap;
};

struct Blend8 {
	Blend8(BlendShape8 shape,int alphamap) : m_Shape(shape), m_AlphaMap(alphamap) {}

	BlendShape8 m_Shape;
	int m_AlphaMap;
};

Blend4 Blends1Neighbour[] = 
{ 
	Blend4(BlendShape4(1,0,0,0), 12)			// u shape blend
};


Blend4 Blends2Neighbour[] = 
{ 
	Blend4(BlendShape4(0,1,1,0), 7), // l shaped corner blend
	Blend4(BlendShape4(1,0,1,0), 10)  // two edges blend
};

Blend8 Blends2Neighbour8[] = 
{ 
	Blend8(BlendShape8(1,1,0,0,0,0,0,0), 12), // u shaped blend
	Blend8(BlendShape8(1,0,0,0,0,1,0,0), 12), // u shaped blend
	Blend8(BlendShape8(0,1,0,1,0,0,0,0), 0) ,
	Blend8(BlendShape8(0,1,0,0,0,1,0,0), 0) 
};

Blend4 Blends3Neighbour[] = 
{ 
	Blend4(BlendShape4(1,1,1,0), 4) // l shaped corner blend
};

Blend8 Blends3Neighbour8[] = 
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

Blend8 Blends4Neighbour8[] = 
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

Blend8 Blends5Neighbour8[] = 
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

Blend8 Blends6Neighbour8[] = 
{ 
	Blend8(BlendShape8(1,1,1,1,1,1,0,0), 2),
	Blend8(BlendShape8(1,1,1,1,1,0,1,0), 8),
	Blend8(BlendShape8(1,1,1,1,0,1,0,1), 1),
	Blend8(BlendShape8(1,1,1,0,1,1,1,0), 6),
	Blend8(BlendShape8(1,1,1,0,1,1,0,1), 3),
	Blend8(BlendShape8(1,1,0,1,1,1,0,1), 10)
};

Blend8 Blends7Neighbour8[] = 
{ 
	Blend8(BlendShape8(1,1,1,1,1,1,0,1), 2),
	Blend8(BlendShape8(1,1,1,1,1,1,1,0), 9)
};



template<class T>
bool MatchBlendShapeRotated(const T& templateshape,const T& shape,unsigned int& flipflags)
{
	// try to match shapes by testing the template shape in normal, and flipped u and v configurations
	// test unrotated shape
	if (shape==templateshape) {
		return true;
	} 

	T tstShape;
	templateshape.FlipU(tstShape);
	if (shape==tstShape) {
		flipflags|=0x02;
		return true;
	} 

	templateshape.FlipV(tstShape);
	if (shape==tstShape) {
		flipflags|=0x01;
		return true;
	}
	
	return false;
}

template<class T>
int MatchBlendShape(const T& templateshape,const T& shape,unsigned int& flipflags)
{
	// try matching unrotated shape first
	if (MatchBlendShapeRotated(templateshape,shape,flipflags)) {
		return true;
	}

	// now try iterating through rotations of 90,180,270 degrees
	T tstShape;
	templateshape.Rotate90(tstShape);
	if (MatchBlendShapeRotated(tstShape,shape,flipflags)) {
		flipflags|=flipflags ? 0x10 : 0x04;
		return true;
	}
	
	templateshape.Rotate180(tstShape);
	if (MatchBlendShapeRotated(tstShape,shape,flipflags)) {
		flipflags|=0x08;
		return true;
	}

	templateshape.Rotate270(tstShape);
	if (MatchBlendShapeRotated(tstShape,shape,flipflags)) {
		flipflags|=flipflags ? 0x04 : 0x10;
		return true;
	}

/*
	FlipBlendU(templateshape,tstShape);
	if (shape==tstShape) {
		flipflags|=0x01;
		return true;
	} 

	FlipBlendV(templateshape,tstShape);
	if (shape==tstShape) {
		flipflags|=0x02;
		return true;
	}
*/
	return false;
}

template<class S,class T>
int LookupBlend(int tableSize,const S* table,const T& shape,unsigned int& flipflags)
{
	// iterate through known blend shapes
	for (int b=0;b<tableSize;b++) {
		const S& blend=table[b];
		if (MatchBlendShape(blend.m_Shape,shape,flipflags)) {
			return blend.m_AlphaMap;
		}
	}

	// eh? shouldn't get here if we've correctly considered all possible cases; keep the compiler happy, and, while
	// we're still debugging possible shapes, return bad blend to highlight suspect alphamap logic
	return 13;
}


int Calculate(BlendShape8 shape,unsigned int& flipflags)
{	
	// assume we're not going to require flipping
	flipflags=0;

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
						return LookupBlend(sizeof(Blends1Neighbour)/sizeof(Blend4),Blends1Neighbour,shape4,flipflags);		

					case 2:
						return LookupBlend(sizeof(Blends2Neighbour)/sizeof(Blend4),Blends2Neighbour,shape4,flipflags);		

					case 3:
						return LookupBlend(sizeof(Blends3Neighbour)/sizeof(Blend4),Blends3Neighbour,shape4,flipflags);		

					case 4:
						// N,S,E,W have same texture, NE,SE,SW,NW don't; use a blend 4 corners
						return 5;
				}
			}
		}


		// we've got this far, so now we've got to consider the remaining choices, all containing diagonal elements
		switch (count) {
			case 1:
				// trivial case - just return a circle blend
				return 0;

			case 2:
				return LookupBlend(sizeof(Blends2Neighbour8)/sizeof(Blend8),Blends2Neighbour8,shape,flipflags);		

			case 3:
				return LookupBlend(sizeof(Blends3Neighbour8)/sizeof(Blend8),Blends3Neighbour8,shape,flipflags);		

			case 4:
				return LookupBlend(sizeof(Blends4Neighbour8)/sizeof(Blend8),Blends4Neighbour8,shape,flipflags);		

			case 5: 
				return LookupBlend(sizeof(Blends5Neighbour8)/sizeof(Blend8),Blends5Neighbour8,shape,flipflags);		

			case 6:
				return LookupBlend(sizeof(Blends6Neighbour8)/sizeof(Blend8),Blends6Neighbour8,shape,flipflags);		

			case 7:
				return LookupBlend(sizeof(Blends7Neighbour8)/sizeof(Blend8),Blends7Neighbour8,shape,flipflags);		
		}

	}

	// Shouldn't get here if we've correctly considered all possible cases; keep the compiler happy, and, while
	// we're still debugging possible shapes, return bad blend to highlight suspect alphamap logic
	return 13;
}

} // end of namespace
