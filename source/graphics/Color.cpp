/* Copyright (C) 2019 Wildfire Games.
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

#include "graphics/Color.h"

#include "graphics/SColor.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"

#if HAVE_SSE
# include <xmmintrin.h>
# include "lib/sysdep/arch/x86_x64/x86_x64.h"
#endif

static SColor4ub fallback_ConvertRGBColorTo4ub(const RGBColor& src)
{
	SColor4ub result;
	result.R = Clamp(static_cast<int>(src.X * 255), 0, 255);
	result.G = Clamp(static_cast<int>(src.Y * 255), 0, 255);
	result.B = Clamp(static_cast<int>(src.Z * 255), 0, 255);
	result.A = 255;
	return result;
}

// on IA32, this is replaced by an SSE assembly version in ia32.cpp
SColor4ub (*ConvertRGBColorTo4ub)(const RGBColor& src) = fallback_ConvertRGBColorTo4ub;


// Assembler-optimized function for color conversion
#if HAVE_SSE
static SColor4ub sse_ConvertRGBColorTo4ub(const RGBColor& src)
{
	const __m128 zero = _mm_setzero_ps();
	const __m128 _255 = _mm_set_ss(255.0f);
	__m128 r = _mm_load_ss(&src.X);
	__m128 g = _mm_load_ss(&src.Y);
	__m128 b = _mm_load_ss(&src.Z);

	// C = min(255, 255*max(C, 0)) ( == Clamp(255*C, 0, 255) )
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
#if HAVE_SSE
	else if (x86_x64::Cap(x86_x64::CAP_SSE))
	{
		ConvertRGBColorTo4ub = sse_ConvertRGBColorTo4ub;
	}
#endif
	else
	{
		debug_printf("No SSE available. Slow fallback routines will be used.\n");
	}
}

/**
 * Important: This function does not modify the value if parsing fails.
 */
bool CColor::ParseString(const CStr8& value, int defaultAlpha)
{
	const size_t NUM_VALS = 4;
	int values[NUM_VALS] = { 0, 0, 0, defaultAlpha };
	std::stringstream stream;
	stream.str(value);
	// Parse each value
	size_t i;
	for (i = 0; i < NUM_VALS; ++i)
	{
		if (stream.eof())
			break;

		stream >> values[i];
		if ((stream.rdstate() & std::stringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CColor parameters. Your input: '%s'", value.c_str());
			return false;
		}
		if (values[i] < 0 || values[i] > 255)
		{
			LOGWARNING("Invalid value (<0 or >255) when parsing CColor parameters. Your input: '%s'", value.c_str());
			return false;
		}
	}

	if (i < 3)
	{
		LOGWARNING("Not enough parameters when parsing as CColor. Your input: '%s'", value.c_str());
		return false;
	}
	if (!stream.eof())
	{
		LOGWARNING("Too many parameters when parsing as CColor. Your input: '%s'", value.c_str());
		return false;
	}

	r = values[0] / 255.f;
	g = values[1] / 255.f;
	b = values[2] / 255.f;
	a = values[3] / 255.f;

	return true;
}

bool CColor::operator==(const CColor& color) const
{
	return
		r == color.r &&
		g == color.g &&
		b == color.b &&
		a == color.a;
}
