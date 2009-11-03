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
 * Convert float RGB(A) colors to unsigned byte
 */

#include "precompiled.h"

#include "graphics/Color.h"

#include "maths/MathUtil.h"
#include "graphics/SColor.h"

#if ARCH_X86_X64
# include <xmmintrin.h>
# include "lib/sysdep/arch/x86_x64/x86_x64.h"
#endif

static SColor4ub fallback_ConvertRGBColorTo4ub(const RGBColor& src)
{
	SColor4ub result;
	result.R=clamp(int(src.X*255),0,255);
	result.G=clamp(int(src.Y*255),0,255);
	result.B=clamp(int(src.Z*255),0,255);
	result.A=0xff;
	return result;
}

// on IA32, this is replaced by an SSE assembly version in ia32.cpp
SColor4ub (*ConvertRGBColorTo4ub)(const RGBColor& src) = fallback_ConvertRGBColorTo4ub;


// Assembler-optimized function for color conversion
#if ARCH_X86_X64
static SColor4ub sse_ConvertRGBColorTo4ub(const RGBColor& src)
{
	const __m128 zero = _mm_setzero_ps();
	const __m128 _255 = _mm_set_ss(255.0f);
	__m128 r = _mm_load_ss(&src.X);
	__m128 g = _mm_load_ss(&src.Y);
	__m128 b = _mm_load_ss(&src.Z);

	// C = min(255, 255*max(C, 0)) ( == clamp(255*C, 0, 255) )
	r = _mm_max_ss(r, zero);
	g = _mm_max_ss(g, zero);
	b = _mm_max_ss(b, zero);

	r = _mm_mul_ss(r, _255);
	g = _mm_mul_ss(g, _255);
	b = _mm_mul_ss(b, _255);

	r = _mm_min_ss(r, _255);
	g = _mm_min_ss(g, _255);
	b = _mm_min_ss(b, _255);

	// convert to integer and combine channels using bit logic
	int ri = _mm_cvtss_si32(r);
	int gi = _mm_cvtss_si32(g);
	int bi = _mm_cvtss_si32(b);

	return SColor4ub(ri, gi, bi, 0xFF);
}
#endif

void ColorActivateFastImpl()
{
	if(0)
	{
	}
#if ARCH_X86_X64
	else if (x86_x64_cap(X86_X64_CAP_SSE))
	{
		ConvertRGBColorTo4ub = sse_ConvertRGBColorTo4ub;
	}
#endif
	else
	{
		debug_printf(L"No SSE available. Slow fallback routines will be used.\n");
	}
}
