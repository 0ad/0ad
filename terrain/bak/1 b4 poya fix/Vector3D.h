//***********************************************************
//
// Name:		Vector3D.H
// Last Update:	28/1/02
// Author:		Poya Manouchehri
//
// Description: Provides an interface for a vector in R3 and
//				allows vector and scalar operations on it
//
//***********************************************************

#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <math.h>
#include "MathUtil.H"
#include "Types.H"

class CVector3D
{
	public:
		float X, Y, Z;

	public:
		CVector3D ();
		CVector3D (float x, float y, float z);

		int operator == (CVector3D &vector);
		int operator != (CVector3D &vector);
		int operator ! ();
		
		//vector addition
		CVector3D operator + (CVector3D &vector);
		//vector addition/assignment
		CVector3D &operator += (CVector3D &vector);

		//vector subtraction
		CVector3D operator - (CVector3D &vector);
		//vector subtraction/assignment
		CVector3D &operator -= (CVector3D &vector);
		
		//scalar multiplication
		CVector3D operator * (float value);
		//scalar multiplication/assignment
		CVector3D operator *= (float value);

	public:
		void Set (float x, float y, float z);
		void Clear ();

		//Dot product
		float Dot (CVector3D &vector);
		//Cross product
		CVector3D Cross (CVector3D &vector);

		//Returns length of the vector
		float GetLength ();
		void Normalize ();

		//Returns a color which describes the vector
		SColor4ub ConvertToColor (float alpha_factor);
};


#endif