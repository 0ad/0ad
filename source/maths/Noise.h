/* Copyright (C) 2013 Wildfire Games.
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
 * 2D and 3D seamless Perlin noise
 */

// Based on http://www.cs.cmu.edu/~mzucker/code/perlin-noise-math-faq.html
// and http://mrl.nyu.edu/~perlin/paper445.pdf.
// Not optimized for speed yet.

#ifndef INCLUDED_NOISE
#define INCLUDED_NOISE

#include "Vector2D.h"
#include "Vector3D.h"
#include "MathUtil.h"

class Noise2D
{
	NONCOPYABLE(Noise2D);

	/// Frequency in X and Y
	int freq;

	/// freq*freq random gradient vectors in the unit cube
	CVector2D** grads;
public:
	Noise2D(int freq);
	~Noise2D();

	/// Evaluate the noise function at a given point
	float operator() (float x, float y);
};

class Noise3D
{
	NONCOPYABLE(Noise3D);

	/// Frequency in X and Y
	int freq;

	/// Frequency in Z (vertical frequency)
	int vfreq;

	/// freq*freq*vfreq random gradient vectors in the unit cube
	CVector3D*** grads;
public:
	Noise3D(int freq, int vfreq);
	~Noise3D();

	/// Evaluate the noise function at a given point
	float operator() (float x, float y, float z);
};

#endif
