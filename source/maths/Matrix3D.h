/* Copyright (C) 2017 Wildfire Games.
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
 * A Matrix class used for holding and manipulating transformation
 * info.
 */

#ifndef INCLUDED_MATRIX3D
#define INCLUDED_MATRIX3D

#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

class CQuaternion;

/////////////////////////////////////////////////////////////////////////
// CMatrix3D: a 4x4 matrix class for common operations in 3D
class CMatrix3D
{
public:
	// the matrix data itself - accessible as either longhand names
	// or via a flat or 2d array
	// NOTE: _xy means row x, column y in the mathematical notation of this matrix, so don't be
	// fooled by the way they're listed below
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
	CMatrix3D ()
	{
	}

	CMatrix3D(
		float a11, float a12, float a13, float a14,
		float a21, float a22, float a23, float a24,
		float a31, float a32, float a33, float a34,
		float a41, float a42, float a43, float a44) :
		_11(a11), _12(a12), _13(a13), _14(a14),
		_21(a21), _22(a22), _23(a23), _24(a24),
		_31(a31), _32(a32), _33(a33), _34(a34),
		_41(a41), _42(a42), _43(a43), _44(a44)
	{
	}

	CMatrix3D(float data[]) :
		_11(data[0]), _21(data[1]), _31(data[2]), _41(data[3]),
		_12(data[4]), _22(data[5]), _32(data[6]), _42(data[7]),
		_13(data[8]), _23(data[9]), _33(data[10]), _43(data[11]),
		_14(data[12]), _24(data[13]), _34(data[14]), _44(data[15])
	{
	}

	// accessors to individual elements of matrix
	// NOTE: in this function definition, 'col' and 'row' represent the column and row into the
	// internal element matrix which is the transposed of the mathematical notation, so the first
	// and second arguments here are actually the row and column into the mathematical notation.
	float& operator()(int col, int row)
	{
		return _data[row*4+col];
	}
	const float& operator()(int col, int row) const
	{
		return _data[row*4+col];
	}

	float& operator[](int idx)
	{
		return _data[idx];
	}
	const float& operator[](int idx) const
	{
		return _data[idx];
	}

	// matrix multiplication
	CMatrix3D operator*(const CMatrix3D &matrix) const
	{
		return CMatrix3D(
			_11*matrix._11 + _12*matrix._21 + _13*matrix._31 + _14*matrix._41,
			_11*matrix._12 + _12*matrix._22 + _13*matrix._32 + _14*matrix._42,
			_11*matrix._13 + _12*matrix._23 + _13*matrix._33 + _14*matrix._43,
			_11*matrix._14 + _12*matrix._24 + _13*matrix._34 + _14*matrix._44,

			_21*matrix._11 + _22*matrix._21 + _23*matrix._31 + _24*matrix._41,
			_21*matrix._12 + _22*matrix._22 + _23*matrix._32 + _24*matrix._42,
			_21*matrix._13 + _22*matrix._23 + _23*matrix._33 + _24*matrix._43,
			_21*matrix._14 + _22*matrix._24 + _23*matrix._34 + _24*matrix._44,

			_31*matrix._11 + _32*matrix._21 + _33*matrix._31 + _34*matrix._41,
			_31*matrix._12 + _32*matrix._22 + _33*matrix._32 + _34*matrix._42,
			_31*matrix._13 + _32*matrix._23 + _33*matrix._33 + _34*matrix._43,
			_31*matrix._14 + _32*matrix._24 + _33*matrix._34 + _34*matrix._44,

			_41*matrix._11 + _42*matrix._21 + _43*matrix._31 + _44*matrix._41,
			_41*matrix._12 + _42*matrix._22 + _43*matrix._32 + _44*matrix._42,
			_41*matrix._13 + _42*matrix._23 + _43*matrix._33 + _44*matrix._43,
			_41*matrix._14 + _42*matrix._24 + _43*matrix._34 + _44*matrix._44
		);
	}

	// matrix multiplication/assignment
	CMatrix3D& operator*=(const CMatrix3D &matrix)
	{
		Concatenate(matrix);
		return *this;
	}

	// matrix scaling
	CMatrix3D operator*(float f) const
	{
		return CMatrix3D(
			_11*f, _12*f, _13*f, _14*f,
			_21*f, _22*f, _23*f, _24*f,
			_31*f, _32*f, _33*f, _34*f,
			_41*f, _42*f, _43*f, _44*f
		);
	}

	// matrix addition
	CMatrix3D operator+(const CMatrix3D &m) const
	{
		return CMatrix3D(
			_11+m._11, _12+m._12, _13+m._13, _14+m._14,
			_21+m._21, _22+m._22, _23+m._23, _24+m._24,
			_31+m._31, _32+m._32, _33+m._33, _34+m._34,
			_41+m._41, _42+m._42, _43+m._43, _44+m._44
		);
	}

	// matrix addition/assignment
	CMatrix3D& operator+=(const CMatrix3D &m)
	{
		_11 += m._11; _21 += m._21; _31 += m._31; _41 += m._41;
		_12 += m._12; _22 += m._22; _32 += m._32; _42 += m._42;
		_13 += m._13; _23 += m._23; _33 += m._33; _43 += m._43;
		_14 += m._14; _24 += m._24; _34 += m._34; _44 += m._44;
		return *this;
	}

	// equality
	bool operator==(const CMatrix3D &m) const
	{
		return _11 == m._11 && _21 == m._21 && _31 == m._31 && _41 == m._41 &&
				 _12 == m._12 && _22 == m._22 && _32 == m._32 && _42 == m._42 &&
				 _13 == m._13 && _23 == m._23 && _33 == m._33 && _43 == m._43 &&
				 _14 == m._14 && _24 == m._24 && _34 == m._34 && _44 == m._44;
	}

	// inequality
	bool operator!=(const CMatrix3D& m) const
	{
		return !(*this == m);
	}

	// set this matrix to the identity matrix
	void SetIdentity();
	// set this matrix to the zero matrix
	void SetZero();
	// set this matrix to the orthogonal projection matrix (as with glOrtho)
	void SetOrtho(float left, float right, float bottom, float top, float near, float far);
	// set this matrix to the perspective projection matrix
	void SetPerspective(float fov, float aspect, float near, float far);

	// concatenate arbitrary matrix onto this matrix
	void Concatenate(const CMatrix3D& m)
	{
		(*this) = m * (*this);
	}

	// blend matrix using only 4x3 subset
	void Blend(const CMatrix3D& m, float f)
	{
		_11 = m._11*f; _21 = m._21*f; _31 = m._31*f;
		_12 = m._12*f; _22 = m._22*f; _32 = m._32*f;
		_13 = m._13*f; _23 = m._23*f; _33 = m._33*f;
		_14 = m._14*f; _24 = m._24*f; _34 = m._34*f;
	}

	// blend matrix using only 4x3 and add onto existing blend
	void AddBlend(const CMatrix3D& m, float f)
	{
		_11 += m._11*f; _21 += m._21*f; _31 += m._31*f;
		_12 += m._12*f; _22 += m._22*f; _32 += m._32*f;
		_13 += m._13*f; _23 += m._23*f; _33 += m._33*f;
		_14 += m._14*f; _24 += m._24*f; _34 += m._34*f;
	}

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

	// sets this matrix to the given translation matrix (any existing transformation will be overwritten)
	void SetTranslation(float x, float y, float z);
	void SetTranslation(const CVector3D& vector);

	// concatenate given translation onto this matrix. Assumes the current
	// matrix is an affine transformation (i.e. the bottom row is [0,0,0,1])
	// as an optimisation.
	void Translate(float x, float y, float z);
	void Translate(const CVector3D& vector);

	// apply translation after this matrix (M = M * T)
	void PostTranslate(float x, float y, float z);

	// set this matrix to the given scaling matrix
	void SetScaling(float x_scale, float y_scale, float z_scale);

	// concatenate given scaling matrix onto this matrix
	void Scale(float x_scale, float y_scale, float z_scale);

	// calculate the inverse of this matrix, store in dst
	void GetInverse(CMatrix3D& dst) const;

	// return the inverse of this matrix
	CMatrix3D GetInverse() const;

	// calculate the transpose of this matrix, store in dst
	CMatrix3D GetTranspose() const;

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
	// return the angle of rotation around the Y axis in range [-pi,pi]
	// (based on projecting the X axis onto the XZ plane)
	float GetYRotation() const;

	// transform a 3D vector by this matrix
	CVector3D Transform(const CVector3D &vector) const
	{
		CVector3D result;
		Transform(vector, result);
		return result;
	}

	void Transform(const CVector3D& vector, CVector3D& result) const
	{
		result.X = _11*vector.X + _12*vector.Y + _13*vector.Z + _14;
		result.Y = _21*vector.X + _22*vector.Y + _23*vector.Z + _24;
		result.Z = _31*vector.X + _32*vector.Y + _33*vector.Z + _34;
	}

	// transform a 4D vector by this matrix
	CVector4D Transform(const CVector4D &vector) const
	{
		CVector4D result;
		Transform(vector, result);
		return result;
	}

	void Transform(const CVector4D& vector, CVector4D& result) const
	{
		result.X = _11*vector.X + _12*vector.Y + _13*vector.Z + _14*vector.W;
		result.Y = _21*vector.X + _22*vector.Y + _23*vector.Z + _24*vector.W;
		result.Z = _31*vector.X + _32*vector.Y + _33*vector.Z + _34*vector.W;
		result.W = _41*vector.X + _42*vector.Y + _43*vector.Z + _44*vector.W;
	}

	// rotate a vector by this matrix
	CVector3D Rotate(const CVector3D& vector) const
	{
		CVector3D result;
		Rotate(vector, result);
		return result;
	}

	void Rotate(const CVector3D& vector, CVector3D& result) const
	{
		result.X = _11*vector.X + _12*vector.Y + _13*vector.Z;
		result.Y = _21*vector.X + _22*vector.Y + _23*vector.Z;
		result.Z = _31*vector.X + _32*vector.Y + _33*vector.Z;
	}

	// rotate a vector by the transpose of this matrix
	void RotateTransposed(const CVector3D& vector,CVector3D& result) const;
	CVector3D RotateTransposed(const CVector3D& vector) const;
};

#endif
