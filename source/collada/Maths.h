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

#ifndef INCLUDED_MATHS
#define INCLUDED_MATHS

class FMMatrix44;

extern void DumpMatrix(const FMMatrix44& m);
extern FMMatrix44 DecomposeToScaleMatrix(const FMMatrix44& m);

// (None of these are used any more)
// extern FMMatrix44 operator+ (const FMMatrix44& a, const FMMatrix44& b);
// extern FMMatrix44 operator/ (const FMMatrix44& a, const float b);
// extern FMMatrix44 QuatToMatrix(float x, float y, float z, float w);

#endif // INCLUDED_MATHS
