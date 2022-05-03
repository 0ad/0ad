/* Copyright (C) 2022 Wildfire Games.
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

#ifndef INCLUDED_COLOR
#define INCLUDED_COLOR

#include "graphics/SColor.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/containers/Span.h"
#include "ps/CStrForward.h"

// Simple defines for 3 and 4 component floating point colors - just map to
// corresponding vector types.
typedef CVector3D RGBColor;
typedef CVector4D RGBAColor;

// Convert float RGB(A) colors to unsigned byte.
// Exposed as function pointer because it is set at init-time to
// one of several implementations depending on CPU caps.
extern SColor4ub (*ConvertRGBColorTo4ub)(const RGBColor& src);

/**
 * Detects CPU caps and activates the best possible codepath.
 */
extern void ColorActivateFastImpl();

struct CColor
{
	CColor() : r(-1.f), g(-1.f), b(-1.f), a(1.f) {}
	CColor(float cr, float cg, float cb, float ca) : r(cr), g(cg), b(cb), a(ca) {}

	/**
	 * Returns whether this has been set to a valid color.
	 */
	operator bool() const
	{
		return r >= 0 && g >= 0 && b >= 0 && a >= 0;
	}

	/**
	 * Try to parse @p Value as a color. Returns true on success, false otherwise.
	 * Leaves the color unchanged if it failed.
	 * @param value Should be "r g b" or "r g b a" where each value is an integer in [0,255].
	 * @param defaultAlpha The alpha value that is used if the format of @p Value is "r g b".
	 */
	bool ParseString(const CStr8& value, int defaultAlpha = 255);

	bool operator==(const CColor& color) const;
	bool operator!=(const CColor& color) const
	{
		return !(*this == color);
	}

	// For passing to uniform as vec3/vec4.
	PS::span<const float> AsFloatArray() const
	{
		// Additional check to prevent a weird compiler has a different
		// alignement for an array and a class members.
		static_assert(
			sizeof(CColor) == sizeof(float) * 4u &&
			offsetof(CColor, r) == 0 &&
			offsetof(CColor, g) == sizeof(float) &&
			offsetof(CColor, b) == sizeof(float) * 2u &&
			offsetof(CColor, a) == sizeof(float) * 3u,
			"CColor should be properly layouted to use AsFloatArray");
		return PS::span(&r, 4);
	}

	// For passing to CRenderer:
	SColor4ub AsSColor4ub() const
	{
		return SColor4ub(
			static_cast<u8>(r * 255.f),
			static_cast<u8>(g * 255.f),
			static_cast<u8>(b * 255.f),
			static_cast<u8>(a * 255.f)
		);
	}

	float r, g, b, a;
};

#endif // INCLUDED_COLOR
