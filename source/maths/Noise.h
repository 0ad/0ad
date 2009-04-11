/**
 * =========================================================================
 * File        : Noise.h
 * Project     : 0 A.D.
 * Description : 2D and 3D seamless Perlin noise
 * =========================================================================
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
	/// Frequency in X and Y
	int freq;

	/// freq*freq random gradient vectors in the unit cube
	CVector2D_Maths** grads;
public:
	Noise2D(int freq);
	~Noise2D();
	
	/// Evaluate the noise function at a given point
	float operator() (float x, float y);
};

class Noise3D 
{
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
