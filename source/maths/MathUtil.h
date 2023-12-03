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

#ifndef INCLUDED_MATHUTIL
#define INCLUDED_MATHUTIL

#define DEGTORAD(a)					((a) * ((float)M_PI/180.0f))
#define RADTODEG(a)					((a) * (180.0f/(float)M_PI))
#define SQR(x)						((x) * (x))

template<typename T>
inline T Interpolate(const T& a, const T& b, float t)
{
	return a + (b - a) * t;
}

template<typename T>
inline T Clamp(T value, T min, T max)
{
	if (value <= min)
		return min;
	else if (value >= max)
		return max;
	return value;
}

template<typename T>
inline T SmoothStep(T edge0, T edge1, T value)
{
	value = Clamp<T>((value - edge0) / (edge1 - edge0), 0, 1); 
	return value * value * (3 - 2 * value);
}

template<typename T>
inline T Sign(const T value)
{
    if (value > T(0))
    	return T(1);
    if (value < T(0))
    	return T(-1);
    return T(0);
}

#endif // INCLUDED_MATHUTIL
