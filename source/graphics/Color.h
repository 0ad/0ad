///////////////////////////////////////////////////////////////////////////////
//
// Name:		Color.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _COLOR_H
#define _COLOR_H

#include "Vector3D.h"
#include "Vector4D.h"
#include "lib/types.h"

// simple defines for 3 and 4 component floating point colors - just map to 
// corresponding vector types
typedef CVector3D RGBColor;
typedef CVector4D RGBAColor;

// SColor3ub: structure for packed RGB colors
struct SColor3ub
{
	u8 R;
	u8 G;
	u8 B;
};

// SColor4ub: structure for packed RGBA colors
struct SColor4ub
{
	u8 R;
	u8 G;
	u8 B;
	u8 A;
};


#endif
