//***********************************************************
//
// Name:		RenderPrims.H
// Author:		Poya Manouchehri
//
// Description: Primitive classes for rendering
//
//***********************************************************

#ifndef RENDERPRIMS_H
#define RENDERPRIMS_H

#include "Vector3D.H"
#include "Types.H"


//shader register constants
#define CONST_VERTREG_WORLDMATRIX	(0)
#define CONST_VERTREG_WVPMATRIX		(4)
#define CONST_VERTREG_LIGHTPOS		(10)
#define CONST_VERTREG_LIGHTRANGE	(11)
#define CONST_VERTREG_ZERO			(14)
#define CONST_VERTREG_HALF			(15)
#define CONST_VERTREG_ONE			(16)
#define CONST_VERTREG_EIGHT			(17)
#define CONST_VERTREG_EYEPOS		(18)

#define CONST_PIXREG_A				(0)
#define CONST_PIXREG_B				(1)

#define CONST_PIXREG_GAMB			(2)
#define CONST_PIXREG_MAMB			(3)
#define CONST_PIXREG_MEMM			(4)

#define CONST_PIXREG_MDIFF			(3)
#define CONST_PIXREG_MSPEC			(4)
#define CONST_PIXREG_LDIFF			(5)
#define CONST_PIXREG_LSPEC			(6)


extern inline DWORD RGBA (BYTE B, BYTE G, BYTE R, BYTE A)
{
	return (A << 24 | B << 16 | G << 8 | R);
}

extern inline DWORD BGRA (BYTE B, BYTE G, BYTE R, BYTE A)
{
	return (A << 24 | R << 16 | G << 8 | B);
}


#endif