//***********************************************************
//
// Name:		MathUtil.H
// Last Update:	28/1/02
// Author:		Poya Manouchehri
//
// Description: This file contains some maths related
//				utility macros and fucntions.
//
//***********************************************************

#ifndef MATHUTIL_H
#define MATHUTIL_H

#define PI							3.14159265358979323846f

#define DEGTORAD(a)					(((a) * PI) / 180.0f)
#define SQR(x)						((x) * (x))
#define MAX(a,b)					((a < b) ? (b) : (a))
#define MIN(a,b)					((a < b) ? (a) : (b))
#define MAX3(a,b,c)					( MAX (MAX(a,b), c) )
#define ABS(a)						((a > 0) ? (a) : (-a))

//extern unsigned int F2DW (float f);

#endif