#ifndef __MATRIX3D_H
#define __MATRIX3D_H

#include <math.h>
#include "Vector3D.h"
#include "Vector4D.h"

class CQuaternion;

/////////////////////////////////////////////////////////////////////////
// CMatrix3D: a 4x4 matrix class for common operations in 3D
class CMatrix3D
{
public:
	// the matrix data itself - accessible as either longhand names
	// or via a flat or 2d array
	union {
		struct {
			float _11, _21, _31, _41;
			float _12, _22, _32, _42;
			float _13, _23, _33, _43;
			float _14, _24, _34, _44;
		};
		float _data[16];
		float _data2d[4][4];
			// (Be aware that m(0,2) == _data2d[2][0] == _13, etc. This is to be considered a feature.)
	};

public:
	// constructors
	CMatrix3D();
	CMatrix3D(float a11,float a12,float a13,float a14,float a21,float a22,float a23,float a24,
		float a31,float a32,float a33,float a34,float a41,float a42,float a43,float a44);

	// accessors to individual elements of matrix
	float& operator()(int col,int row) {
		return _data[row*4+col];
	}
	const float& operator()(int col,int row) const {
		return _data[row*4+col];
	}

	// matrix multiplication
	CMatrix3D operator*(const CMatrix3D &matrix) const;
	// matrix multiplication/assignment
	CMatrix3D& operator*=(const CMatrix3D &matrix);
	// matrix scaling
	CMatrix3D operator*(float f) const;
	// matrix scaling/assignment
	CMatrix3D& operator*=(float f);
	// matrix addition
	CMatrix3D operator+(const CMatrix3D &matrix) const;
	// matrix addition/assignment
	CMatrix3D& operator+=(const CMatrix3D &matrix);

	// set this matrix to the identity matrix
	void SetIdentity();
	// set this matrix to the zero matrix
	void SetZero();

	// concatenate arbitrary matrix onto this matrix
	void Concatenate(const CMatrix3D& m);

	// set this matrix to a rotation matrix for a rotation about X axis of given angle
	void SetXRotation(float angle);
	// set this matrix to a rotation matrix for a rotation about Y axis of given angle
	void SetYRotation(float angle);
	// set this matrix to a rotation matrix for a rotation about Z axis of given angle
	void SetZRotation(float angle);
	// set this matrix to a rotation described by given quaternion
	void SetRotation(const CQuaternion& quat);

	// concatenate a rotation about the X axis onto this matrix
	void RotateX(float angle);
	// concatenate a rotation about the Y axis onto this matrix
	void RotateY(float angle);
	// concatenate a rotation about the Z axis onto this matrix
	void RotateZ(float angle);
	// concatenate a rotation described by given quaternion
	void Rotate(const CQuaternion& quat);

	// set this matrix to given translation
	void SetTranslation(float x, float y, float z);
	void SetTranslation(const CVector3D& vector);

	// concatenate given translation onto this matrix
	void Translate(float x, float y, float z);
	void Translate(const CVector3D& vector);

	// set this matrix to the given scaling matrix
	void SetScaling(float x_scale, float y_scale, float z_scale);

	// concatenate given scaling matrix onto this matrix
	void Scale(float x_scale, float y_scale, float z_scale);

	// calculate the inverse of this matrix, store in dst
	void GetInverse(CMatrix3D& dst) const;

	// calculate the transpose of this matrix, store in dst
	void GetTranspose(CMatrix3D& dst) const;

	// return the translation component of this matrix
	CVector3D GetTranslation() const;
	// return left vector, derived from rotation
	CVector3D GetLeft() const;
	// return up vector, derived from rotation
	CVector3D GetUp() const;
	// return forward vector, derived from rotation
	CVector3D GetIn() const;
	// return a quaternion representing the matrix's rotation
	CQuaternion GetRotation() const;

	// transform a 3D vector by this matrix
	void Transform(const CVector3D &vector,CVector3D& result) const;
	CVector3D Transform(const CVector3D &vector) const;
	// transform a 4D vector by this matrix
	void Transform(const CVector4D &vector,CVector4D& result) const;
	CVector4D Transform(const CVector4D &vector) const;
	// rotate a vector by this matrix
	void Rotate(const CVector3D& vector,CVector3D& result) const;
	CVector3D Rotate(const CVector3D& vector) const;
	// rotate a vector by the transpose of this matrix
	void RotateTransposed(const CVector3D& vector,CVector3D& result) const;
	CVector3D RotateTransposed(const CVector3D& vector) const;
};

#endif
