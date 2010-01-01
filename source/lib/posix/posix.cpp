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

#include "precompiled.h"
#include "posix.h"

#if ARCH_IA32
# include "lib/sysdep/arch/ia32/ia32_asm.h"
#endif


#if !HAVE_C99_MATH

float rintf(float f)
{
#if ARCH_IA32
	return ia32_asm_rintf(f);
#else
	return (float)(int)f;
#endif
}

double rint(double d)
{
#if ARCH_IA32
	return ia32_asm_rint(d);
#else
	return (double)(int)d;
#endif
}


float fminf(float a, float b)
{
#if ARCH_IA32
	return ia32_asm_fminf(a, b);
#else
	return (a < b)? a : b;
#endif
}

float fmaxf(float a, float b)
{
#if ARCH_IA32
	return ia32_asm_fmaxf(a, b);
#else
	return (a > b)? a : b;
#endif
}


size_t fpclassifyd(double d)
{
#if ARCH_IA32
	return ia32_asm_fpclassifyd(d);
#else
	// really sucky stub implementation; doesn't attempt to cover all cases.

	if(d != d)
		return FP_NAN;
	else
		return FP_NORMAL;
#endif
}

size_t fpclassifyf(float f)
{
#if ARCH_IA32
	return ia32_asm_fpclassifyf(f);
#else
	const double d = (double)f;
	return fpclassifyd(d);
#endif
}

#endif	// #if !HAVE_C99_MATH


#if EMULATE_WCSDUP
wchar_t* wcsdup(const wchar_t* str)
{
	const size_t num_chars = wcslen(str);
	wchar_t* dst = (wchar_t*)malloc((num_chars+1)*sizeof(wchar_t));	// note: wcsdup is required to use malloc
	if(!dst)
		return 0;
	wcscpy_s(dst, num_chars, str);
	return dst;
}
#endif

#if EMULATE_WCSCASECMP
int wcscasecmp (const wchar_t* s1, const wchar_t* s2)
{
	wint_t a1, a2;

	if (s1 == s2)
		return 0;

	do
	{
		a1 = towlower(*s1++);
		a2 = towlower(*s2++);
		if (a1 == L'\0')
			break;
	}
	while (a1 == a2);

	return a1 - a2;
}
#endif
