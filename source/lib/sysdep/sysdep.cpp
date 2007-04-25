/**
 * =========================================================================
 * File        : sysdep.cpp
 * Project     : 0 A.D.
 * Description : various system-specific function implementations
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2003-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"
#include "sysdep.h"


// emulate C99 functionality
#if !HAVE_C99_MATH

// fallback versions in case ia32 optimized versions are unavailable.
#if !HAVE_MS_ASM

inline float rintf(float f)
{
	return (float)(int)f;
}

inline double rint(double d)
{
	return (double)(int)d;
}

float fminf(float a, float b)
{
	return (a < b)? a : b;
}

float fmaxf(float a, float b)
{
	return (a > b)? a : b;
}

uint fpclassify(double d)
{
	// really sucky stub implementation; doesn't attempt to cover all cases.

	if(d != d)
		return FP_NAN;
	else
		return FP_NORMAL;
}

uint fpclassifyf(float f)
{
	return fpclassify((double)f);
}

#endif

#endif	// #if !HAVE_C99_MATH
