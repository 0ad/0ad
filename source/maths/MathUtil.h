/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_MATHUTIL
#define INCLUDED_MATHUTIL


// C99 math constants (missing on MSVC):
#ifndef INFINITY
#define INFINITY					(std::numeric_limits<float>::infinity())
#endif
#ifndef NAN
#define NAN							(std::numeric_limits<float>::quiet_NaN())
#endif

// POSIX math constants (missing on MSVC):
#ifndef M_PI
#define M_PI						3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2						1.57079632679489661923
#endif

#ifndef PI
#define PI							3.14159265358979323846f
#endif

#define DEGTORAD(a)					((a) * (PI/180.0f))
#define RADTODEG(a)					((a) * (180.0f/PI))
#define SQR(x)						((x) * (x))

template <typename T>
T Interpolate(const T& a, const T& b, float l)
{
	return a + (b - a) * l;
}

template <typename T>
inline T clamp(T value, T min, T max)
{
	if (value <= min) return min;
	else if (value >= max) return max;
	else return value;
}

inline float sgn(float a)
{
    if (a > 0.0f) return 1.0f;
    if (a < 0.0f) return -1.0f;
    return 0.0f;
}

#endif
