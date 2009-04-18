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

/**
 * =========================================================================
 * File        : Vector2D.h
 * Project     : 0 A.D.
 * Description : Provides an interface for a vector in R2 and
 *               allows vector and scalar operations on it
 * =========================================================================
 */

#ifndef INCLUDED_VECTOR2D
#define INCLUDED_VECTOR2D


#include <math.h>

///////////////////////////////////////////////////////////////////////////////
// CVector2D_Maths:
class CVector2D_Maths
{
public:
	CVector2D_Maths() {}
    CVector2D_Maths(float x,float y) { X=x; Y=y; }
	CVector2D_Maths(const CVector2D_Maths& p) { X=p.X; Y=p.Y; }

	operator float*() {
		return &X;
	}

	operator const float*() const {
		return &X;
	}

	CVector2D_Maths operator-() const {
	    return CVector2D_Maths(-X, -Y);
	}

    CVector2D_Maths operator+(const CVector2D_Maths& t) const {
    	return CVector2D_Maths(X+t.X, Y+t.Y);
	}

	CVector2D_Maths operator-(const CVector2D_Maths& t) const {
	    return CVector2D_Maths(X-t.X, Y-t.Y);
	}

	CVector2D_Maths operator*(float f) const {
	    return CVector2D_Maths(X*f, Y*f);
	}

	CVector2D_Maths operator/(float f) const {
	    float inv=1.0f/f;
		return CVector2D_Maths(X*inv, Y*inv);
	}

	CVector2D_Maths& operator+=(const CVector2D_Maths& t) {
		X+=t.X; Y+=t.Y;
	    return *this;
	}

	CVector2D_Maths& operator-=(const CVector2D_Maths& t) {
		X-=t.X; Y-=t.Y;
	    return *this;
	}

	CVector2D_Maths& operator*=(float f) {
	    X*=f; Y*=f;
	    return *this;
	}

	CVector2D_Maths& operator/=(float f) {
		float invf=1.0f/f;
	    X*=invf; Y*=invf;
    	return *this;
	}

    float Dot(const CVector2D_Maths& a) const {
		return X*a.X + Y*a.Y;
	}

	float LengthSquared() const {
	    return Dot(*this);
	}

    float Length() const {
		return (float) sqrt(LengthSquared());
	}

    void Normalize() {
		float mag=Length();
        X/=mag; Y/=mag;
	}

public:
	float X, Y;
};
//////////////////////////////////////////////////////////////////////////////////


#endif
