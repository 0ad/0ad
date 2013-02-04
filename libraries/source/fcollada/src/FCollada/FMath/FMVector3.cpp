/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMVector2.h"
#include "FMVector3.h"
#include "FMVector4.h"

// Vector constants
const FMVector2 FMVector2::Zero(0.0f, 0.0f);
const FMVector2 FMVector2::Origin = FMVector2::Zero;
const FMVector2 FMVector2::XAxis(1.0f, 0.0f);
const FMVector2 FMVector2::YAxis(0.0f, 1.0f);
const FMVector3 FMVector3::Zero(0.0f, 0.0f, 0.0f);
const FMVector3 FMVector3::XAxis(1.0f, 0.0f, 0.0f);
const FMVector3 FMVector3::YAxis(0.0f, 1.0f, 0.0f);
const FMVector3 FMVector3::ZAxis(0.0f, 0.0f, 1.0f);
const FMVector3 FMVector3::Origin = FMVector3::Zero;
const FMVector3 FMVector3::One(1.0f, 1.0f, 1.0f);
const FMVector4 FMVector4::Zero(0.0f, 0.0f, 0.0f, 0.0f);
const FMVector4 FMVector4::One(1.0f, 1.0f, 1.0f, 1.0f);
const FMVector4 FMVector4::AlphaOne(0.0f, 0.0f, 0.0f, 1.0f);

FMVector3::FMVector3(const FMVector4& vect4)
:	x(vect4.x), y(vect4.y), z(vect4.z)
{
}

// Read in the vector from a source
FMVector3::FMVector3(const float* source, uint32 startIndex)
{
	x = source[startIndex];
	y = source[startIndex + 1];
	z = source[startIndex + 2];
}

// Read in the vector from a source
FMVector3::FMVector3(const double* source, uint32 startIndex)
{
	x = (float)source[startIndex];
	y = (float)source[startIndex + 1];
	z = (float)source[startIndex + 2];
}

// Read in the vector from a color value.
FMVector4::FMVector4(const FMColor& c)
{
	x = (float)c.r / 255.0f;
	y = (float)c.g / 255.0f;
	z = (float)c.b / 255.0f;
	w = (float)c.a / 255.0f;
}

FMVector4 FMVector4::FromHSVColor(float hue, float saturation, float value)
{
	// [GLaforte - 15-04-2007]
	// Algorithm inspired from http://www.cs.rit.edu/~ncs/color/t_convert.html
	// Written by Nan C. Schaller, Rochester Institute of Technology, Computer Science Department
	if (!IsEquivalent(saturation, 0.0f)) 
	{
		hue *= 6.0f;						// sector 0 to 5
		float sector = floor(hue);
		float f = hue - sector;				// factorial part of h
		float p = value * (1.0f - saturation);
		float q = value * (1.0f - saturation * f);
		float t = value * (1.0f - saturation * (1.0f - f));
		switch ((uint32) sector)
		{
			case 0: return FMVector4(value, t, p, 1.0f);
			case 1: return FMVector4(q, value, p, 1.0f);
			case 2: return FMVector4(p, value, t, 1.0f);
			case 3: return FMVector4(p, q, value, 1.0f);
			case 4: return FMVector4(t, p, value, 1.0f);
			case 5:
			default: return FMVector4(value, p, q, 1.0f);
		}
	}
	else return FMVector4(value, value, value, 1.0f); // Achromatic (grey)
}

FMVector3 FMVector4::ToHSVColor()
{
	// [GLaforte - 15-04-2007]
	// Algorithm inspired from http://www.cs.rit.edu/~ncs/color/t_convert.html
	// Written by Nan C. Schaller, Rochester Institute of Technology, Computer Science Department

	FMVector3 hsv;
	float smallest = min(min(x, y), z);
	float largest = max(max(x, y), z);
	hsv.z = largest;				// v
	float delta = largest - smallest;
	if (!IsEquivalent(largest, 0.0f))
	{
		hsv.y = delta / largest;	// s
		if (IsEquivalent(x, largest))			hsv.x = (y-z) / delta;			// between yellow & magenta
		else if (IsEquivalent(y, largest))		hsv.x = 2.0f + (z-x) / delta;	// between cyan & yellow
		else									hsv.x = 4.0f + (x-y) / delta;	// between magenta & cyan
		hsv.x /= 6.0f;				// convert to [0,1]
		if (hsv.x < 0.0f) hsv.x += 1.0f;
	}
	else hsv.x = hsv.y = 0; // black
	return hsv;
}
