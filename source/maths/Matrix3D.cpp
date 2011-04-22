/* Copyright (C) 2010 Wildfire Games.
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

#include "precompiled.h"

#include "Matrix3D.h"
#include "Quaternion.h"
#include "Vector4D.h"

CMatrix3D::CMatrix3D ()
{
}

CMatrix3D::CMatrix3D(
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

CMatrix3D::CMatrix3D(float data[])
{
	for(int i=0; i<16; i++)
	{
		_data[i] = data[i];
	}
}

//Matrix multiplication
CMatrix3D CMatrix3D::operator*(const CMatrix3D& matrix) const
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

//Matrix multiplication/assignment
CMatrix3D& CMatrix3D::operator*=(const CMatrix3D& matrix)
{
	Concatenate(matrix);	
	return *this;
}

//Matrix scaling
CMatrix3D CMatrix3D::operator*(float f) const
{
	CMatrix3D tmp;
	for (int i=0;i<16;i++) {
		tmp._data[i]=_data[i]*f;
	}
	return tmp;
}

//Matrix scaling/assignment
CMatrix3D& CMatrix3D::operator*=(float f) 
{
	for (int i=0;i<16;i++) {
		_data[i]*=f;
	}
	return *this;
}

//Matrix addition
CMatrix3D CMatrix3D::operator+(const CMatrix3D& m) const
{
	CMatrix3D tmp;
	for (int i=0;i<16;i++) {
		tmp._data[i]=_data[i]+m._data[i];
	}
	return tmp;
}

//Matrix addition/assignment
CMatrix3D& CMatrix3D::operator+=(const CMatrix3D& m) 
{
	for (int i=0;i<16;i++) {
		_data[i]+=m._data[i];
	}
	return *this;
}

bool CMatrix3D::operator==(const CMatrix3D &matrix) const
{
	for (int i = 0; i < 16; ++i)
		if (matrix._data[i] != _data[i])
			return false;
	return true;
}

//Sets the identity matrix
void CMatrix3D::SetIdentity ()
{
	_11=1.0f; _12=0.0f; _13=0.0f; _14=0.0f;
	_21=0.0f; _22=1.0f; _23=0.0f; _24=0.0f;
	_31=0.0f; _32=0.0f; _33=1.0f; _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

//Sets the zero matrix
void CMatrix3D::SetZero ()
{
	_11=0.0f; _12=0.0f; _13=0.0f; _14=0.0f;
	_21=0.0f; _22=0.0f; _23=0.0f; _24=0.0f;
	_31=0.0f; _32=0.0f; _33=0.0f; _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=0.0f;
}

//The following clear the matrix and set the 
//rotation of each of the 3 axes

void CMatrix3D::SetXRotation (float angle)
{
	const float Cos = cosf (angle);
	const float Sin = sinf (angle);
	
	_11=1.0f; _12=0.0f; _13=0.0f; _14=0.0f;
	_21=0.0f; _22=Cos;  _23=-Sin; _24=0.0f;
	_31=0.0f; _32=Sin;  _33=Cos;  _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

void CMatrix3D::SetYRotation (float angle)
{
	const float Cos = cosf (angle);
	const float Sin = sinf (angle);

	_11=Cos;  _12=0.0f; _13=Sin;  _14=0.0f;
	_21=0.0f; _22=1.0f; _23=0.0f; _24=0.0f;
	_31=-Sin; _32=0.0f; _33=Cos;  _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

void CMatrix3D::SetZRotation (float angle)
{
	const float Cos = cosf (angle);
	const float Sin = sinf (angle);

	_11=Cos;  _12=-Sin; _13=0.0f; _14=0.0f;
	_21=Sin;  _22=Cos;  _23=0.0f; _24=0.0f;
	_31=0.0f; _32=0.0f; _33=1.0f; _34=0.0f;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

//The following apply a rotation to the matrix
//about each of the axes;

void CMatrix3D::RotateX (float angle)
{
	const float Cos = cosf (angle);
	const float Sin = sinf (angle);
	const float tmp_21 = _21;
	const float tmp_22 = _22;
	const float tmp_23 = _23;
	const float tmp_24 = _24;

	_21 = Cos * _21 - Sin * _31;
	_22 = Cos * _22 - Sin * _32;
	_23 = Cos * _23 - Sin * _33;
	_24 = Cos * _24 - Sin * _34;

	_31 = Sin * tmp_21 + Cos * _31;
	_32 = Sin * tmp_22 + Cos * _32;
	_33 = Sin * tmp_23 + Cos * _33;
	_34 = Sin * tmp_24 + Cos * _34;
}

void CMatrix3D::RotateY (float angle)
{
	const float Cos = cosf (angle);
	const float Sin = sinf (angle);
	const float tmp_11 = _11;
	const float tmp_12 = _12;
	const float tmp_13 = _13;
	const float tmp_14 = _14;

	_11 = Cos * _11 + Sin * _31;
	_12 = Cos * _12 + Sin * _32;
	_13 = Cos * _13 + Sin * _33;
	_14 = Cos * _14 + Sin * _34;

	_31 = -Sin * tmp_11 + Cos * _31;
	_32 = -Sin * tmp_12 + Cos * _32;
	_33 = -Sin * tmp_13 + Cos * _33;
	_34 = -Sin * tmp_14 + Cos * _34;
}

void CMatrix3D::RotateZ (float angle)
{
	const float Cos = cosf (angle);
	const float Sin = sinf (angle);
	const float tmp_11 = _11;
	const float tmp_12 = _12;
	const float tmp_13 = _13;
	const float tmp_14 = _14;

	_11 = Cos * _11 - Sin * _21;
	_12 = Cos * _12 - Sin * _22;
	_13 = Cos * _13 - Sin * _23;
	_14 = Cos * _14 - Sin * _24;

	_21 = Sin * tmp_11 + Cos * _21;
	_22 = Sin * tmp_12 + Cos * _22;
	_23 = Sin * tmp_13 + Cos * _23;
	_24 = Sin * tmp_14 + Cos * _24;
}

//Sets the translation of the matrix
void CMatrix3D::SetTranslation (float x, float y, float z)
{
	_11=1.0f; _12=0.0f; _13=0.0f; _14=x;
	_21=0.0f; _22=1.0f; _23=0.0f; _24=y;
	_31=0.0f; _32=0.0f; _33=1.0f; _34=z;
	_41=0.0f; _42=0.0f; _43=0.0f; _44=1.0f;
}

void CMatrix3D::SetTranslation(const CVector3D& vector)
{
	SetTranslation(vector.X, vector.Y, vector.Z);	
}

//Applies a translation to the matrix
void CMatrix3D::Translate(float x, float y, float z)
{
	_14 += x;
	_24 += y;
	_34 += z;
}

void CMatrix3D::Translate(const CVector3D &vector)
{
	_14 += vector.X;
	_24 += vector.Y;
	_34 += vector.Z;
}

void CMatrix3D::Concatenate(const CMatrix3D& m)
{
	(*this)=m*(*this);
}

CVector3D CMatrix3D::GetTranslation() const
{
	return CVector3D(_14, _24, _34);
}

//Clears and sets the scaling of the matrix
void CMatrix3D::SetScaling (float x_scale, float y_scale, float z_scale)
{
	_11=x_scale; _12=0.0f;	  _13=0.0f;	   _14=0.0f;
	_21=0.0f;	 _22=y_scale; _23=0.0f;	   _24=0.0f;
	_31=0.0f;	 _32=0.0f;	  _33=z_scale; _34=0.0f;
	_41=0.0f;	 _42=0.0f;	  _43=0.0f;    _44=1.0f;
}

//Scales the matrix
void CMatrix3D::Scale (float x_scale, float y_scale, float z_scale)
{
	_11 *= x_scale;
	_12 *= x_scale;
	_13 *= x_scale;
	_14 *= x_scale;

	_21 *= y_scale;
	_22 *= y_scale;
	_23 *= y_scale;
	_24 *= y_scale;

	_31 *= z_scale;
	_32 *= z_scale;
	_33 *= z_scale;
	_34 *= z_scale;
}

//Returns the transpose of the matrix. For orthonormal
//matrices, this is the same is the inverse matrix
CMatrix3D CMatrix3D::GetTranspose() const
{
	return CMatrix3D(
		_11, _21, _31, _41,
		_12, _22, _32, _42,
		_13, _23, _33, _43,
		_14, _24, _34, _44);
}


//Get a vector which points to the left of the matrix
CVector3D CMatrix3D::GetLeft() const
{
	return CVector3D(-_11, -_21, -_31);
}

//Get a vector which points up from the matrix
CVector3D CMatrix3D::GetUp() const
{
	return CVector3D(_12, _22, _32);
}

//Get a vector which points to front of the matrix
CVector3D CMatrix3D::GetIn() const
{
	return CVector3D(_13, _23, _33);
}

//Transform a vector by this matrix
CVector4D CMatrix3D::Transform(const CVector4D &vector) const
{
	CVector4D result;
	Transform(vector,result);
	return result;
}

void CMatrix3D::Transform(const CVector4D& vector,CVector4D& result) const
{
	result[0] = _11*vector[0] + _12*vector[1] + _13*vector[2] + _14*vector[3];
	result[1] = _21*vector[0] + _22*vector[1] + _23*vector[2] + _24*vector[3];
	result[2] = _31*vector[0] + _32*vector[1] + _33*vector[2] + _34*vector[3];
	result[3] = _41*vector[0] + _42*vector[1] + _43*vector[2] + _44*vector[3];
}

///////////////////////////////////////////////////////////////////////////////
// RotateTransposed: rotate a vector by the transpose of this matrix
CVector3D CMatrix3D::RotateTransposed(const CVector3D& vector) const
{
	CVector3D result;
	RotateTransposed(vector,result);
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// RotateTransposed: rotate a vector by the transpose of this matrix
void CMatrix3D::RotateTransposed(const CVector3D& vector,CVector3D& result) const
{
	result.X = _11*vector.X + _21*vector.Y + _31*vector.Z;
	result.Y = _12*vector.X + _22*vector.Y + _32*vector.Z;
	result.Z = _13*vector.X + _23*vector.Y + _33*vector.Z;
}


void CMatrix3D::GetInverse(CMatrix3D& dst) const
{
	float tmp[12];	// temp array for pairs 
	float src[16];	// array of transpose source matrix 
	float det;		// determinant 
	
	// transpose matrix 
	for (int i = 0; i < 4; ++i) {
		src[i] = _data[i*4];
		src[i + 4] = _data[i*4 + 1];
		src[i + 8] = _data[i*4 + 2];
		src[i + 12] = _data[i*4 + 3];
	}

	// calculate pairs for first 8 elements (cofactors) 
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	
	// calculate first 8 elements (cofactors)
	dst._data[0] = (tmp[0]-tmp[1])*src[5] + (tmp[3]-tmp[2])*src[6] + (tmp[4]-tmp[5])*src[7];
	dst._data[1] = (tmp[1]-tmp[0])*src[4] + (tmp[6]-tmp[7])*src[6] + (tmp[9]-tmp[8])*src[7];
	dst._data[2] = (tmp[2]-tmp[3])*src[4] + (tmp[7]-tmp[6])*src[5] + (tmp[10]-tmp[11])*src[7];
	dst._data[3] = (tmp[5]-tmp[4])*src[4] + (tmp[8]-tmp[9])*src[5] + (tmp[11]-tmp[10])*src[6];
	dst._data[4] = (tmp[1]-tmp[0])*src[1] + (tmp[2]-tmp[3])*src[2] + (tmp[5]-tmp[4])*src[3];
	dst._data[5] = (tmp[0]-tmp[1])*src[0] + (tmp[7]-tmp[6])*src[2] + (tmp[8]-tmp[9])*src[3];
	dst._data[6] = (tmp[3]-tmp[2])*src[0] + (tmp[6]-tmp[7])*src[1] + (tmp[11]-tmp[10])*src[3];
	dst._data[7] = (tmp[4]-tmp[5])*src[0] + (tmp[9]-tmp[8])*src[1] + (tmp[10]-tmp[11])*src[2];
	
	// calculate pairs for second 8 elements (cofactors) 
	tmp[0] = src[2]*src[7];
	tmp[1] = src[3]*src[6];
	tmp[2] = src[1]*src[7];
	tmp[3] = src[3]*src[5];
	tmp[4] = src[1]*src[6];
	tmp[5] = src[2]*src[5];
	tmp[6] = src[0]*src[7];
	tmp[7] = src[3]*src[4];
	tmp[8] = src[0]*src[6];
	tmp[9] = src[2]*src[4];
	tmp[10] = src[0]*src[5];
	tmp[11] = src[1]*src[4];

	// calculate second 8 elements (cofactors) 
	dst._data[8] = (tmp[0]-tmp[1])*src[13] + (tmp[3]-tmp[2])*src[14] + (tmp[4]-tmp[5])*src[15];
	dst._data[9] = (tmp[1]-tmp[0])*src[12] + (tmp[6]-tmp[7])*src[14] + (tmp[9]-tmp[8])*src[15];
	dst._data[10] = (tmp[2]-tmp[3])*src[12] + (tmp[7]-tmp[6])*src[13] + (tmp[10]-tmp[11])*src[15];
	dst._data[11] = (tmp[5]-tmp[4])*src[12] + (tmp[8]-tmp[9])*src[13] + (tmp[11]-tmp[10])*src[14];
	dst._data[12] = (tmp[2]-tmp[3])*src[10] + (tmp[5]-tmp[4])*src[11] + (tmp[1]-tmp[0])*src[9];
	dst._data[13] = (tmp[7]-tmp[6])*src[10] + (tmp[8]-tmp[9])*src[11] + (tmp[0]-tmp[1])*src[8];
	dst._data[14] = (tmp[6]-tmp[7])*src[9] + (tmp[11]-tmp[10])*src[11] + (tmp[3]-tmp[2])*src[8];
	dst._data[15] = (tmp[10]-tmp[11])*src[10] + (tmp[4]-tmp[5])*src[8] + (tmp[9]-tmp[8])*src[9];

	// calculate matrix inverse 
	det=src[0]*dst._data[0]+src[1]*dst._data[1]+src[2]*dst._data[2]+src[3]*dst._data[3];
	det = 1/det;
	for ( int j = 0; j < 16; j++) {
		dst._data[j] *= det;
	}
}

CMatrix3D CMatrix3D::GetInverse() const
{
	CMatrix3D r;
	GetInverse(r);
	return r;
}

void CMatrix3D::Rotate(const CQuaternion& quat)
{
	CMatrix3D rotationMatrix=quat.ToMatrix();
	Concatenate(rotationMatrix);
}

CQuaternion CMatrix3D::GetRotation() const
{
	float tr = _data2d[0][0] + _data2d[1][1] + _data2d[2][2];

	int next[] = { 1, 2, 0 };

	float quat[4];

	if (tr > 0.f)
	{
		float s = sqrtf(tr + 1.f);
		quat[3] = s * 0.5f;
		s = 0.5f / s;
		quat[0] = (_data2d[1][2] - _data2d[2][1]) * s;
		quat[1] = (_data2d[2][0] - _data2d[0][2]) * s;
		quat[2] = (_data2d[0][1] - _data2d[1][0]) * s;
	}
	else
	{
		int i = 0;
		if (_data2d[1][1] > _data2d[0][0]) i = 1;
		if (_data2d[2][2] > _data2d[i][i]) i = 2;
		int j = next[i];
		int k = next[j];

		float s = sqrtf((_data2d[i][i] - (_data2d[j][j] + _data2d[k][k])) + 1.f);
		quat[i] = s * 0.5f;

		if (s != 0.f) s = 0.5f / s;

		quat[3] = (_data2d[j][k] - _data2d[k][j]) * s;
		quat[j] = (_data2d[i][j] + _data2d[j][i]) * s;
		quat[k] = (_data2d[i][k] + _data2d[k][i]) * s;
	}

	return CQuaternion(quat[0], quat[1], quat[2], quat[3]);
}

void CMatrix3D::SetRotation(const CQuaternion& quat)
{
	quat.ToMatrix(*this);
}

float CMatrix3D::GetYRotation() const
{
	// Project the X axis vector onto the XZ plane
	CVector3D axis = -GetLeft();
	axis.Y = 0;

	// Normalise projected vector

	float len = axis.Length();
	if (len < 0.0001f)
		return 0.f;
	axis *= 1.0f/len;

	// Negate the return angle to match the SetYRotation convention
	return -atan2(axis.Z, axis.X);
}
