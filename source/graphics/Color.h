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

/**
 * =========================================================================
 * File        : Color.h
 * Project     : 0 A.D.
 * Description : Convert float RGB(A) colors to unsigned byte
 * =========================================================================
 */

#ifndef INCLUDED_COLOR
#define INCLUDED_COLOR

#include "SColor.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

// simple defines for 3 and 4 component floating point colors - just map to
// corresponding vector types
typedef CVector3D RGBColor;
typedef CVector4D RGBAColor;

// exposed as function pointer because it is set at init-time to
// one of several implementations depending on CPU caps.
extern SColor4ub (*ConvertRGBColorTo4ub)(const RGBColor& src);

// call once ia32_Init has run; detects CPU caps and activates the best
// possible codepath.
extern void ColorActivateFastImpl();

#endif
