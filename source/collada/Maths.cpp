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

#include "Maths.h"

#include "FCollada.h"


void DumpMatrix(const FMMatrix44& m)
{
	Log(LOG_INFO, "\n[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]",
		m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
		m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
		m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
		m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]
	);
}

FMMatrix44 DecomposeToScaleMatrix(const FMMatrix44& m)
{
	FMVector3 scale, rotation, translation;
	float inverted;
	m.Decompose(scale, rotation, translation, inverted);
	return FMMatrix44::ScaleMatrix(scale);
}

/*
FMMatrix44 operator+ (const FMMatrix44& a, const FMMatrix44& b)
{
	FMMatrix44 r;
	for (int x = 0; x < 4; ++x)
		for (int y = 0; y < 4; ++y)
			r[x][y] = a[x][y] + b[x][y];
	return r;
}

FMMatrix44 operator/ (const FMMatrix44& a, const float b)
{
	FMMatrix44 r;
	for (int x = 0; x < 4; ++x)
		for (int y = 0; y < 4; ++y)
			r[x][y] = a[x][y] / b;
	return r;
}

FMMatrix44 QuatToMatrix(float x, float y, float z, float w)
{
	FMMatrix44 r;

	r[0][0] = 1.0f - (y*y*2 + z*z*2);
	r[1][0] = x*y*2 - w*z*2;
	r[2][0] = x*z*2 + w*y*2;
	r[3][0] = 0;

	r[0][1] = x*y*2 + w*z*2;
	r[1][1] = 1.0f - (x*x*2 + z*z*2);
	r[2][1] = y*z*2 - w*x*2;
	r[3][1] = 0;

	r[0][2] = x*z*2 - w*y*2;
	r[1][2] = y*z*2 + w*x*2;
	r[2][2] = 1.0f - (x*x*2 + y*y*2);
	r[3][2] = 0;

	r[0][3] = 0;
	r[1][3] = 0;
	r[2][3] = 0;
	r[3][3] = 1;

	return r;
}
*/
