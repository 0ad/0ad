//***********************************************************
//
// Name:		CVector2D.h
// Author:		Matei Zaharia
//
// Description: Provides an interface for a vector in R4 and
//				allows vector and scalar operations on it
//
//***********************************************************

#ifndef VECTOR2D_H
#define VECTOR2D_H


#include <math.h>

///////////////////////////////////////////////////////////////////////////////
// CVector2D:
class CVector2D 
{
public:
	CVector2D() {}
    CVector2D(float x,float y) { X=x; Y=y; }
	CVector2D(const CVector2D& p) { X=p.X; Y=p.Y; }

	operator float*() {
		return &X;
	}

	operator const float*() const {
		return &X;
	}

	CVector2D operator-() const {
	    return CVector2D(-X, -Y);
	}

    CVector2D operator+(const CVector2D& t) const {
    	return CVector2D(X+t.X, Y+t.Y);
	}

	CVector2D operator-(const CVector2D& t) const {
	    return CVector2D(X-t.X, Y-t.Y);
	}

	CVector2D operator*(float f) const {
	    return CVector2D(X*f, Y*f);
	}

	CVector2D operator/(float f) const {
	    float inv=1.0f/f;
		return CVector2D(X*inv, Y*inv);
	}

	CVector2D& operator+=(const CVector2D& t) {
		X+=t.X; Y+=t.Y;
	    return *this;
	}

	CVector2D& operator-=(const CVector2D& t) {
		X-=t.X; Y-=t.Y;
	    return *this;
	}

	CVector2D& operator*=(float f) {
	    X*=f; Y*=f;
	    return *this;
	}

	CVector2D& operator/=(float f) {
		float invf=1.0f/f;
	    X*=invf; Y*=invf;
    	return *this;
	}

    float Dot(const CVector2D& a) const {
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
