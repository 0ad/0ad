//***********************************************************
//
// Name:		Matrix3D.H
// Last Update:	31/1/02
// Author:		Poya Manouchehri
//
// Description: A Matrix class used for holding and 
//				manipulating transformation info.
//
//***********************************************************

#ifndef MATRIX3D_H
#define MATRIX3D_H

#include <math.h>

#include "Vector3D.H"

class CMatrix3D
{
	public:
		float _11, _12, _13, _14;
		float _21, _22, _23, _24;
		float _31, _32, _33, _34;
		float _41, _42, _43, _44;

	public:
		CMatrix3D ();

		//Matrix multiplication
		CMatrix3D operator * (CMatrix3D &matrix);
		//Matrix multiplication/assignment
		CMatrix3D &operator *= (CMatrix3D &matrix);

		//Sets the identity matrix
		void SetIdentity ();
		//Sets the zero matrix
		void SetZero ();

		//The following clear the matrix and set the 
		//rotation of each of the 3 axes
		void SetXRotation (float angle);
		void SetYRotation (float angle);
		void SetZRotation (float angle);

		//The following apply a rotation to the matrix
		//about each of the axes;
		void RotateX (float angle);
		void RotateY (float angle);
		void RotateZ (float angle);

		//Sets the translation of the matrix
		void SetTranslation (float x, float y, float z);
		void SetTranslation (CVector3D &vector);

		//Applies a translation to the matrix
		void Translate (float x, float y, float z);
		void Translate (CVector3D &vector);

		CVector3D GetTranslation ();

		//Clears and sets the scaling of the matrix
		void SetScaling (float x_scale, float y_scale, float z_scale);
		//Scales the matrix
		void Scale (float x_scale, float y_scale, float z_scale);

		//Returns the transpose of the matrix. For orthonormal
		//matrices, this is the same is the inverse matrix
		CMatrix3D GetTranspose ();

		//Get a vector which points to the left of the matrix
		CVector3D GetLeft ();
		//Get a vector which points up from the matrix
		CVector3D GetUp ();
		//Get a vector which points to front of the matrix
		CVector3D GetIn ();

		//Set the matrix from two vectors (Up and In)
		void SetFromUpIn (CVector3D &up, CVector3D &in, float scale);

	public: //Vector manipulation methods
		//Transform a vector by this matrix
		CVector3D Transform (CVector3D &vector);
		//Only rotate (not translate) a vector by this matrix
		CVector3D Rotate (CVector3D &vector);
};

#endif