//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/25/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_internal.c
//
// Description: Internal stuff for ILU
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"


ILdouble IL_PI      = 3.1415926535897932384626;
ILdouble IL_DEGCONV = 0.0174532925199432957692;
//#if !defined(__APPLE__)
	ILimage *iluCurImage = NULL;
//#endif


// Anyway we can inline these next 3 functions in pure C without making them macros?
INLINE extern ILfloat ilCos(ILfloat Angle)
{
	return (ILfloat)(cos(Angle * IL_DEGCONV));
}


INLINE extern ILfloat ilSin(ILfloat Angle)
{
	return (ILfloat)(sin(Angle * IL_DEGCONV));
}


INLINE extern ILint ilRound(ILfloat Num)
{
	return (ILint)(Num + 0.5);
}
