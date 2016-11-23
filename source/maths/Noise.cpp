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
 * 2D and 3D seamless Perlin noise
 */

// Based on http://www.cs.cmu.edu/~mzucker/code/perlin-noise-math-faq.html
// and http://mrl.nyu.edu/~perlin/paper445.pdf.
// Not optimized for speed yet.

#include "precompiled.h"
#include "Noise.h"
#include <cmath>
#include <boost/random/mersenne_twister.hpp>

namespace
{
	/// Random number generator (Boost Mersenne Twister)
	boost::mt19937 rng;

	/// Utility function for random numbers
	float randFloat() {
		return ((float)rng()) / 4294967296.0f;
	}

	/// Utility function used in both noises as an ease curve
	float easeCurve(float t)
	{
		return t*t*t*(t*(t*6-15)+10);
	}
}

Noise2D::Noise2D(int f)
{
	freq = f;
	grads = new CVector2D*[freq];
	for(int i=0; i<freq; i++)
	{
		grads[i] = new CVector2D[freq];
		for(int j=0; j<freq; j++)
		{
			float a = randFloat() * 2 * (float)M_PI;
			grads[i][j] = CVector2D(cos(a), sin(a));
		}
	}
}

Noise2D::~ Noise2D()
{
	for(int i=0; i<freq; i++)
	{
		delete[] grads[i];
	}
	delete[] grads;
}

float Noise2D::operator()(float x, float y)
{
	x *= freq;
	y *= freq;

	int ix = (int)floor(x);
	int iy = (int)floor(y);

	float fx = x - ix;
	float fy = y - iy;

	ix %= freq; if(ix<0) ix += freq;
	iy %= freq; if(iy<0) iy += freq;

	int ix1 = (ix+1) % freq;
	int iy1 = (iy+1) % freq;

	float s = grads[ix][iy].Dot(CVector2D(fx, fy));
	float t = grads[ix1][iy].Dot(CVector2D(fx-1, fy));
	float u = grads[ix][iy1].Dot(CVector2D(fx, fy-1));
	float v = grads[ix1][iy1].Dot(CVector2D(fx-1, fy-1));

	float ex = easeCurve(fx);
	float ey = easeCurve(fy);
	float a = s + ex*(t-s);
	float b = u + ex*(v-u);
	return (a + ey*(b-a)) * .5 + .5;
}

Noise3D::Noise3D(int f, int v) : freq(f), vfreq(v)
{
	grads = new CVector3D**[freq];
	for(int i=0; i<freq; i++)
	{
		grads[i] = new CVector3D*[freq];
		for(int j=0; j<freq; j++)
		{
			grads[i][j] = new CVector3D[vfreq];
			for(int k=0; k<vfreq; k++)
			{
				CVector3D vec;
				do {
					vec = CVector3D(2*randFloat()-1, 2*randFloat()-1, 2*randFloat()-1);
				}
				while(vec.LengthSquared() > 1 || vec.LengthSquared() < 0.1);
				vec.Normalize();
				grads[i][j][k] = CVector3D(vec.X, vec.Y, vec.Z);
			}
		}
	}
}

Noise3D::~ Noise3D()
{
	for(int i=0; i<freq; i++)
	{
		for(int j=0; j<freq; j++)
		{
			delete[] grads[i][j];
		}
		delete[] grads[i];
	}
	delete[] grads;
}

float Noise3D::operator()(float x, float y, float z)
{
	x *= freq;
	y *= freq;
	z *= vfreq;

	int ix = (int)floor(x);
	int iy = (int)floor(y);
	int iz = (int)floor(z);

	float fx = x - ix;
	float fy = y - iy;
	float fz = z - iz;

	ix %= freq; if(ix<0) ix += freq;
	iy %= freq; if(iy<0) iy += freq;
	iz %= vfreq; if(iz<0) iz += vfreq;

	int ix1 = (ix+1) % freq;
	int iy1 = (iy+1) % freq;
	int iz1 = (iz+1) % vfreq;

	float s0 = grads[ix][iy][iz].Dot(CVector3D(fx, fy, fz));
	float t0 = grads[ix1][iy][iz].Dot(CVector3D(fx-1, fy, fz));
	float u0 = grads[ix][iy1][iz].Dot(CVector3D(fx, fy-1, fz));
	float v0 = grads[ix1][iy1][iz].Dot(CVector3D(fx-1, fy-1, fz));

	float s1 = grads[ix][iy][iz1].Dot(CVector3D(fx, fy, fz-1));
	float t1 = grads[ix1][iy][iz1].Dot(CVector3D(fx-1, fy, fz-1));
	float u1 = grads[ix][iy1][iz1].Dot(CVector3D(fx, fy-1, fz-1));
	float v1 = grads[ix1][iy1][iz1].Dot(CVector3D(fx-1, fy-1, fz-1));

	float ex = easeCurve(fx);
	float ey = easeCurve(fy);
	float ez = easeCurve(fz);

	float a0 = s0 + ex*(t0-s0);
	float b0 = u0 + ex*(v0-u0);
	float c0 = a0 + ey*(b0-a0);

	float a1 = s1 + ex*(t1-s1);
	float b1 = u1 + ex*(v1-u1);
	float c1 = a1 + ey*(b1-a1);

	return (c0 + ez*(c1-c0)) * .5 + .5;
}


