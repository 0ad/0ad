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
#include "Vector3D.h"
#include "Vector4D.h"

class CMatrix3D
{
	public:
		union {
			struct {
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
			float _data[16];
		};
		
	public:
		CMatrix3D ();
		CMatrix3D (float a11,float a12,float a13,float a14,float a21,float a22,float a23,float a24,
			float a31,float a32,float a33,float a34,float a41,float a42,float a43,float a44);

		// accessors to individual elements of matrix
		float& operator()(int row,int col) {
			return _data[row*4+col];
		}
		const float& operator()(int row,int col) const {
			return _data[row*4+col];
		}

		//Matrix multiplication
		CMatrix3D operator * (const CMatrix3D &matrix) const;
		//Matrix multiplication/assignment
		CMatrix3D &operator *= (const CMatrix3D &matrix);

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
		void Translate (const CVector3D &vector);

		CVector3D GetTranslation ();

		// calculate the inverse of this matrix, store in dst
		void Invert(CMatrix3D& dst) const;

		//Clears and sets the scaling of the matrix
		void SetScaling (float x_scale, float y_scale, float z_scale);
		//Scales the matrix
		void Scale (float x_scale, float y_scale, float z_scale);

		//Returns the transpose of the matrix. For orthonormal
		//matrices, this is the same is the inverse matrix
		void GetTranspose(CMatrix3D& result) const;

		//Get a vector which points to the left of the matrix
		CVector3D GetLeft () const;
		//Get a vector which points up from the matrix
		CVector3D GetUp () const;
		//Get a vector which points to front of the matrix
		CVector3D GetIn () const;

		//Set the matrix from two vectors (Up and In)
		void SetFromUpIn (CVector3D &up, CVector3D &in, float scale);

	public: //Vector manipulation methods
		//Transform a 3D vector by this matrix
		CVector3D Transform (CVector3D &vector);
		//Transform a 4D vector by this matrix
		CVector4D Transform (const CVector4D &vector) const;
		//Only rotate (not translate) a vector by this matrix
		CVector3D Rotate (CVector3D &vector);
};

#endif
