///////////////////////////////////////////////////////////////////////////////
//
// Name:		Noise.h
// Author:		Matei Zaharia
// Contact:		matei@wildfiregames.com
//
// Description: 2D and 3D seamless Perlin noise classes. Not optimized for speed yet.
//
// Based on http://www.cs.cmu.edu/~mzucker/code/perlin-noise-math-faq.html 
// and http://mrl.nyu.edu/~perlin/paper445.pdf.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef NOISE_H
#define NOISE_H

#include "Vector2D.h"
#include "Vector3D.h"
#include "MathUtil.h"

class Noise2D 
{
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
