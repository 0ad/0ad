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

#ifndef INCLUDED_MATHUTIL
#define INCLUDED_MATHUTIL

#define DEGTORAD(a)					((a) * ((float)M_PI/180.0f))
#define RADTODEG(a)					((a) * (180.0f/(float)M_PI))
#define SQR(x)						((x) * (x))

template <typename T>
inline T Interpolate(const T& a, const T& b, float t)
{
	return a + (b - a) * t;
}

template <typename T>
inline T Clamp(T value, T min, T max)
{
	if (value <= min)
		return min;
	else if (value >= max)
		return max;
	return value;
}

inline float sgn(float a)
{
    if (a > 0.0f)
    	return 1.0f;
    if (a < 0.0f)
    	return -1.0f;
    return 0.0f;
}

#endif // INCLUDED_MATHUTIL
