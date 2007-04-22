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


// float->int conversion: not using the ia32 version; just implement as a
// cast. (see USE_IA32_FLOAT_TO_INT definition for details)
#if !USE_IA32_FLOAT_TO_INT

i32 i32_from_float(float f)
{
	return (i32)f;
}

i32 i32_from_double(double d)
{
	return (i32)d;
}

i64 i64_from_double(double d)
{
	return (i64)d;
}

#endif

#if !HAVE_STRDUP

char* strdup(const char* str)
{
	const size_t num_chars = strlen(str);
	char* new_str = malloc(num_chars*sizeof(char)+1);
	SAFE_STRCPY(new_str, str);
	return new_str;
}

wchar_t* wcsdup(const wchar_t* str)
{
	const size_t num_chars = wcslen(str);
	wchar_t* new_str = malloc(num_chars*sizeof(wchar_t)+1);
	SAFE_WCSCPY(new_str, str);
	return new_str;
}

#endif	// #if !HAVE_STRDUP
