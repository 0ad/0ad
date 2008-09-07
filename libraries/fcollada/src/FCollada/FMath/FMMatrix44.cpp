/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMMatrix44.h"
#include <limits>

static float __identity[] = { 1, 0, 0, 0, 0, 1, 0 ,0 ,0, 0, 1, 0, 0, 0, 0, 1 };
FMMatrix44 FMMatrix44::Identity(__identity);

FMMatrix44::FMMatrix44(const float* _m)
{
	Set(_m);
}

FMMatrix44::FMMatrix44(const double* _m)
{
	Set(_m);
}

FMMatrix44& FMMatrix44::operator=(const FMMatrix44& copy)
{
	m[0][0] = copy.m[0][0]; m[0][1] = copy.m[0][1]; m[0][2] = copy.m[0][2]; m[0][3] = copy.m[0][3];
	m[1][0] = copy.m[1][0]; m[1][1] = copy.m[1][1]; m[1][2] = copy.m[1][2]; m[1][3] = copy.m[1][3];
	m[2][0] = copy.m[2][0]; m[2][1] = copy.m[2][1]; m[2][2] = copy.m[2][2]; m[2][3] = copy.m[2][3];
	m[3][0] = copy.m[3][0]; m[3][1] = copy.m[3][1]; m[3][2] = copy.m[3][2]; m[3][3] = copy.m[3][3];
	return *this;
}

void FMMatrix44::Set(const float* _m)
{
	m[0][0] = _m[0]; m[1][0] = _m[1]; m[2][0] = _m[2]; m[3][0] = _m[3];
	m[0][1] = _m[4]; m[1][1] = _m[5]; m[2][1] = _m[6]; m[3][1] = _m[7];
	m[0][2] = _m[8]; m[1][2] = _m[9]; m[2][2] = _m[10]; m[3][2] = _m[11];
	m[0][3] = _m[12]; m[1][3] = _m[13]; m[2][3] = _m[14]; m[3][3] = _m[15];
}

void FMMatrix44::Set(const double* _m)
{
	m[0][0] = (float)_m[0]; m[1][0] = (float)_m[1]; m[2][0] = (float)_m[2]; m[3][0] = (float)_m[3];
	m[0][1] = (float)_m[4]; m[1][1] = (float)_m[5]; m[2][1] = (float)_m[6]; m[3][1] = (float)_m[7];
	m[0][2] = (float)_m[8]; m[1][2] = (float)_m[9]; m[2][2] = (float)_m[10]; m[3][2] = (float)_m[11];
	m[0][3] = (float)_m[12]; m[1][3] = (float)_m[13]; m[2][3] = (float)_m[14]; m[3][3] = (float)_m[15];
}

// Returns the transpose of this matrix
FMMatrix44 FMMatrix44::Transposed() const
{
	FMMatrix44 mx;
	mx.m[0][0] = m[0][0]; mx.m[1][0] = m[0][1]; mx.m[2][0] = m[0][2]; mx.m[3][0] = m[0][3];
	mx.m[0][1] = m[1][0]; mx.m[1][1] = m[1][1]; mx.m[2][1] = m[1][2]; mx.m[3][1] = m[1][3];
	mx.m[0][2] = m[2][0]; mx.m[1][2] = m[2][1]; mx.m[2][2] = m[2][2]; mx.m[3][2] = m[2][3];
	mx.m[0][3] = m[3][0]; mx.m[1][3] = m[3][1]; mx.m[2][3] = m[3][2]; mx.m[3][3] = m[3][3];
	return mx;
}

FMVector3 FMMatrix44::TransformCoordinate(const FMVector3& coordinate) const
{
	return FMVector3(
		m[0][0] * coordinate.x + m[1][0] * coordinate.y + m[2][0] * coordinate.z + m[3][0],
		m[0][1] * coordinate.x + m[1][1] * coordinate.y + m[2][1] * coordinate.z + m[3][1],
		m[0][2] * coordinate.x + m[1][2] * coordinate.y + m[2][2] * coordinate.z + m[3][2]
	);
}

FMVector4 FMMatrix44::TransformCoordinate(const FMVector4& coordinate) const
{
	return FMVector4(
		m[0][0] * coordinate.x + m[1][0] * coordinate.y + m[2][0] * coordinate.z + m[3][0] * coordinate.w,
		m[0][1] * coordinate.x + m[1][1] * coordinate.y + m[2][1] * coordinate.z + m[3][1] * coordinate.w,
		m[0][2] * coordinate.x + m[1][2] * coordinate.y + m[2][2] * coordinate.z + m[3][2] * coordinate.w,
		m[0][3] * coordinate.x + m[1][3] * coordinate.y + m[2][3] * coordinate.z + m[3][3] * coordinate.w
	);
}

FMVector3 FMMatrix44::TransformVector(const FMVector3& v) const
{
	return FMVector3(
		m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
		m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
		m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z
	);
}
/*
void FMMatrix44::SetTranslation(const FMVector3& translation)
{
	m[3][0] = translation.x;
	m[3][1] = translation.y;
	m[3][2] = translation.z;
}
*/
static float det3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3);

void FMMatrix44::Decompose(FMVector3& scale, FMVector3& rotation, FMVector3& translation, float& inverted) const
{
	// translation * rotation [x*y*z order] * scale = original matrix
	// if inverted is true, then negate scale and the above formula will be correct
	scale.x = sqrtf(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
	scale.y = sqrtf(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
	scale.z = sqrtf(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

	FMVector3 savedScale(scale);
	if (IsEquivalent(scale.x, 0.0f)) { scale.x = FLT_TOLERANCE; }
	if (IsEquivalent(scale.y, 0.0f)) { scale.y = FLT_TOLERANCE; }
	if (IsEquivalent(scale.z, 0.0f)) { scale.z = FLT_TOLERANCE; }

	inverted = FMath::Sign(det3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]));

	if (inverted < 0.0f)
	{
		// intentionally do not want to do this on the savedScale
		scale.x = -scale.x;
		scale.y = -scale.y;
		scale.z = -scale.z;
	}

	// Calculate the rotation in Y first, using m[0][2], checking for out-of-bounds values
	float c;
	if (m[2][0] / scale.z >= 1.0f - FLT_TOLERANCE)
	{
		rotation.y = ((float) FMath::Pi) / 2.0f;
		c = 0.0f;
	}
	else if (m[2][0] / scale.z <= -1.0f + FLT_TOLERANCE)
	{
		rotation.y = ((float) FMath::Pi) / -2.0f;
		c = 0.0f;
	}
	else
	{
		rotation.y = asinf(m[2][0] / scale.z);
		c = cosf(rotation.y);
	}

	// Using the cosine of the Y rotation will give us the rotation in X and Z.
	// Check for the infamous Gimbal Lock.
	if (fabsf(c) > 0.01f) 
	{
		float rx = m[2][2] / scale.z / c;
		float ry = -m[2][1] / scale.z / c;
		rotation.x = atan2f(ry, rx);
		rx = m[0][0] / scale.x / c;
		ry = -m[1][0] / scale.y / c;
		rotation.z = atan2f(ry, rx);
	}
	else
	{
		rotation.z = 0;
		float rx = m[1][1] / scale.y;
		float ry = m[1][2] / scale.y;
		rotation.x = atan2f(ry, rx);
	}

	translation = GetTranslation();
	scale = savedScale;
}

void FMMatrix44::Recompose(const FMVector3& scale, const FMVector3& rotation, const FMVector3& translation, float inverted)
{
	(*this) = FMMatrix44::TranslationMatrix(translation) * FMMatrix44::AxisRotationMatrix(FMVector3::ZAxis, rotation.z)
		* FMMatrix44::AxisRotationMatrix(FMVector3::YAxis, rotation.y) * FMMatrix44::AxisRotationMatrix(FMVector3::XAxis, rotation.x)
		* FMMatrix44::ScaleMatrix(inverted * scale);
}

// Code taken and adapted from nVidia's nv_algebra: det2x2, det3x3, invert, multiply
// -----
// Calculate the determinant of a 2x2 matrix
static float det2x2(float a1, float a2, float b1, float b2)
{
    return a1 * b2 - b1 * a2;
}

// Calculate the determinent of a 3x3 matrix
static float det3x3(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3)
{
    return a1 * det2x2(b2, b3, c2, c3) - b1 * det2x2(a2, a3, c2, c3) + c1 * det2x2(a2, a3, b2, b3);
}

// Returns the inverse of this matrix
FMMatrix44 FMMatrix44::Inverted() const
{
	FMMatrix44 b;

	b.m[0][0] =  det3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
	b.m[0][1] = -det3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], m[2][3], m[3][1], m[3][2], m[3][3]);
	b.m[0][2] =  det3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[3][1], m[3][2], m[3][3]);
	b.m[0][3] = -det3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], m[2][3]);

	b.m[1][0] = -det3x3(m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
	b.m[1][1] =  det3x3(m[0][0], m[0][2], m[0][3], m[2][0], m[2][2], m[2][3], m[3][0], m[3][2], m[3][3]);
	b.m[1][2] = -det3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[3][0], m[3][2], m[3][3]);
	b.m[1][3] =  det3x3(m[0][0], m[0][2], m[0][3], m[1][0], m[1][2], m[1][3], m[2][0], m[2][2], m[2][3]);

	b.m[2][0] =  det3x3(m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
	b.m[2][1] = -det3x3(m[0][0], m[0][1], m[0][3], m[2][0], m[2][1], m[2][3], m[3][0], m[3][1], m[3][3]);
	b.m[2][2] =  det3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[3][0], m[3][1], m[3][3]);
	b.m[2][3] = -det3x3(m[0][0], m[0][1], m[0][3], m[1][0], m[1][1], m[1][3], m[2][0], m[2][1], m[2][3]);

	b.m[3][0] = -det3x3(m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
	b.m[3][1] =  det3x3(m[0][0], m[0][1], m[0][2], m[2][0], m[2][1], m[2][2], m[3][0], m[3][1], m[3][2]);
	b.m[3][2] = -det3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[3][0], m[3][1], m[3][2]);
	b.m[3][3] =  det3x3(m[0][0], m[0][1], m[0][2], m[1][0], m[1][1], m[1][2], m[2][0], m[2][1], m[2][2]);

	double det = (m[0][0] * b.m[0][0]) + (m[1][0] * b.m[0][1]) + (m[2][0] * b.m[0][2]) + (m[3][0] * b.m[0][3]);

	double epsilon = std::numeric_limits<double>::epsilon();
	if (det + epsilon >= 0.0f && det - epsilon <= 0.0f) det = FMath::Sign(det) * 0.0001f;
	float oodet = (float) (1.0 / det);

	b.m[0][0] *= oodet;
	b.m[0][1] *= oodet;
	b.m[0][2] *= oodet;
	b.m[0][3] *= oodet;

	b.m[1][0] *= oodet;
	b.m[1][1] *= oodet;
	b.m[1][2] *= oodet;
	b.m[1][3] *= oodet;

	b.m[2][0] *= oodet;
	b.m[2][1] *= oodet;
	b.m[2][2] *= oodet;
	b.m[2][3] *= oodet;

	b.m[3][0] *= oodet;
	b.m[3][1] *= oodet;
	b.m[3][2] *= oodet;
	b.m[3][3] *= oodet;

	return b;
}

float FMMatrix44::Determinant() const
{
	float cofactor0 = det3x3(m[1][1], m[1][2], m[1][3], m[2][1], m[2][2], 
							 m[2][3], m[3][1], m[3][2], m[3][3]);
	float cofactor1 = -det3x3(m[0][1], m[0][2], m[0][3], m[2][1], m[2][2], 
							  m[2][3], m[3][1], m[3][2], m[3][3]);
	float cofactor2 = det3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], 
							 m[1][3], m[3][1], m[3][2], m[3][3]);
	float cofactor3 = -det3x3(m[0][1], m[0][2], m[0][3], m[1][1], m[1][2], 
							  m[1][3], m[2][1], m[2][2], m[2][3]);
	return (m[0][0] * cofactor0) + (m[1][0] * cofactor1) + 
           (m[2][0] * cofactor2) + (m[3][0] * cofactor3);
}

FMMatrix44 operator*(const FMMatrix44& m1, const FMMatrix44& m2)
{
    FMMatrix44 mx;
    mx.m[0][0] = m1.m[0][0] * m2.m[0][0] + m1.m[1][0] * m2.m[0][1] + m1.m[2][0] * m2.m[0][2] + m1.m[3][0] * m2.m[0][3];
    mx.m[0][1] = m1.m[0][1] * m2.m[0][0] + m1.m[1][1] * m2.m[0][1] + m1.m[2][1] * m2.m[0][2] + m1.m[3][1] * m2.m[0][3];
    mx.m[0][2] = m1.m[0][2] * m2.m[0][0] + m1.m[1][2] * m2.m[0][1] + m1.m[2][2] * m2.m[0][2] + m1.m[3][2] * m2.m[0][3];
    mx.m[0][3] = m1.m[0][3] * m2.m[0][0] + m1.m[1][3] * m2.m[0][1] + m1.m[2][3] * m2.m[0][2] + m1.m[3][3] * m2.m[0][3];
    mx.m[1][0] = m1.m[0][0] * m2.m[1][0] + m1.m[1][0] * m2.m[1][1] + m1.m[2][0] * m2.m[1][2] + m1.m[3][0] * m2.m[1][3];
    mx.m[1][1] = m1.m[0][1] * m2.m[1][0] + m1.m[1][1] * m2.m[1][1] + m1.m[2][1] * m2.m[1][2] + m1.m[3][1] * m2.m[1][3];
    mx.m[1][2] = m1.m[0][2] * m2.m[1][0] + m1.m[1][2] * m2.m[1][1] + m1.m[2][2] * m2.m[1][2] + m1.m[3][2] * m2.m[1][3];
    mx.m[1][3] = m1.m[0][3] * m2.m[1][0] + m1.m[1][3] * m2.m[1][1] + m1.m[2][3] * m2.m[1][2] + m1.m[3][3] * m2.m[1][3];
    mx.m[2][0] = m1.m[0][0] * m2.m[2][0] + m1.m[1][0] * m2.m[2][1] + m1.m[2][0] * m2.m[2][2] + m1.m[3][0] * m2.m[2][3];
    mx.m[2][1] = m1.m[0][1] * m2.m[2][0] + m1.m[1][1] * m2.m[2][1] + m1.m[2][1] * m2.m[2][2] + m1.m[3][1] * m2.m[2][3];
    mx.m[2][2] = m1.m[0][2] * m2.m[2][0] + m1.m[1][2] * m2.m[2][1] + m1.m[2][2] * m2.m[2][2] + m1.m[3][2] * m2.m[2][3];
    mx.m[2][3] = m1.m[0][3] * m2.m[2][0] + m1.m[1][3] * m2.m[2][1] + m1.m[2][3] * m2.m[2][2] + m1.m[3][3] * m2.m[2][3];
    mx.m[3][0] = m1.m[0][0] * m2.m[3][0] + m1.m[1][0] * m2.m[3][1] + m1.m[2][0] * m2.m[3][2] + m1.m[3][0] * m2.m[3][3];
    mx.m[3][1] = m1.m[0][1] * m2.m[3][0] + m1.m[1][1] * m2.m[3][1] + m1.m[2][1] * m2.m[3][2] + m1.m[3][1] * m2.m[3][3];
    mx.m[3][2] = m1.m[0][2] * m2.m[3][0] + m1.m[1][2] * m2.m[3][1] + m1.m[2][2] * m2.m[3][2] + m1.m[3][2] * m2.m[3][3];
    mx.m[3][3] = m1.m[0][3] * m2.m[3][0] + m1.m[1][3] * m2.m[3][1] + m1.m[2][3] * m2.m[3][2] + m1.m[3][3] * m2.m[3][3];
    return mx;
}

FMVector4 operator*(const FMMatrix44& m, const FMVector4& v)
{
	float x = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w;
	float y = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w;
	float z = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w;
	float w = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w;
	return FMVector4(x, y, z, w);
}

FMMatrix44 operator*(float a, const FMMatrix44& m1)
{
	FMMatrix44 mx;
    mx.m[0][0] = a * m1.m[0][0];
    mx.m[0][1] = a * m1.m[0][1];
    mx.m[0][2] = a * m1.m[0][2];
    mx.m[0][3] = a * m1.m[0][3];
    mx.m[1][0] = a * m1.m[1][0];
    mx.m[1][1] = a * m1.m[1][1];
    mx.m[1][2] = a * m1.m[1][2];
    mx.m[1][3] = a * m1.m[1][3];
    mx.m[2][0] = a * m1.m[2][0];
    mx.m[2][1] = a * m1.m[2][1];
    mx.m[2][2] = a * m1.m[2][2];
    mx.m[2][3] = a * m1.m[2][3];
    mx.m[3][0] = a * m1.m[3][0];
    mx.m[3][1] = a * m1.m[3][1];
    mx.m[3][2] = a * m1.m[3][2];
    mx.m[3][3] = a * m1.m[3][3];
    return mx;
}

FMMatrix44 FMMatrix44::TranslationMatrix(const FMVector3& translation)
{
	FMMatrix44 matrix;
	matrix[0][0] = 1.0f; matrix[0][1] = 0.0f; matrix[0][2] = 0.0f; matrix[0][3] = 0.0f;
	matrix[1][0] = 0.0f; matrix[1][1] = 1.0f; matrix[1][2] = 0.0f; matrix[1][3] = 0.0f;
	matrix[2][0] = 0.0f; matrix[2][1] = 0.0f; matrix[2][2] = 1.0f; matrix[2][3] = 0.0f;
	matrix[3][0] = translation.x; matrix[3][1] = translation.y; matrix[3][2] = translation.z; matrix[3][3] = 1.0f;
	return matrix;
}

FMMatrix44 FMMatrix44::AxisRotationMatrix(const FMVector3& axis, float angle)
{
	// Formulae inspired from http://www.mines.edu/~gmurray/ArbitraryAxisRotation/ArbitraryAxisRotation.html
	FMMatrix44 matrix;
	FMVector3 a = (IsEquivalent(axis.LengthSquared(), 1.0f)) ? axis : axis.Normalize();
	float xSq = a.x * a.x;
	float ySq = a.y * a.y;
	float zSq = a.z * a.z;
	float cT = cosf(angle);
	float sT = sinf(angle);

	matrix[0][0] = xSq + (ySq + zSq) * cT;
	matrix[0][1] = a.x * a.y * (1.0f - cT) + a.z * sT;
	matrix[0][2] = a.x * a.z * (1.0f - cT) - a.y * sT;
	matrix[0][3] = 0;
	matrix[1][0] = a.x * a.y * (1.0f - cT) - a.z * sT;
	matrix[1][1] = ySq + (xSq + zSq) * cT;
	matrix[1][2] = a.y * a.z * (1.0f - cT) + a.x * sT;
	matrix[1][3] = 0;
	matrix[2][0] = a.x * a.z * (1.0f - cT) + a.y * sT;
	matrix[2][1] = a.y * a.z * (1.0f - cT) - a.x * sT;
	matrix[2][2] = zSq + (xSq + ySq) * cT;
	matrix[2][3] = 0;
	matrix[3][2] = matrix[3][1] = matrix[3][0] = 0;
	matrix[3][3] = 1;
	return matrix;
}

FMMatrix44 FMMatrix44::XAxisRotationMatrix(float angle)
{
	FMMatrix44 ret = FMMatrix44::Identity;
	ret[1][1] = ret[2][2] = cos(angle);
	ret[2][1] = -(ret[1][2] = sin(angle));
	return ret;
}

FMMatrix44 FMMatrix44::YAxisRotationMatrix(float angle)
{
	FMMatrix44 ret = FMMatrix44::Identity;
	ret[0][0] = ret[2][2] = cos(angle);
	ret[0][2] = -(ret[2][0] = sin(angle));
	return ret;
}

FMMatrix44 FMMatrix44::ZAxisRotationMatrix(float angle)
{
	FMMatrix44 ret = FMMatrix44::Identity;
	ret[0][0] = ret[1][1] = cos(angle);
	ret[1][0] = -(ret[0][1] = sin(angle));
	return ret;
}

FMMatrix44 FMMatrix44::EulerRotationMatrix(const FMVector3& rotation)
{
	FMMatrix44 transform;
	if (!IsEquivalent(rotation.x, 0.0f)) transform = XAxisRotationMatrix(rotation.x);
	else transform = FMMatrix44::Identity;
	if (!IsEquivalent(rotation.y, 0.0f)) transform *= YAxisRotationMatrix(rotation.y);
	if (!IsEquivalent(rotation.z, 0.0f)) transform *= ZAxisRotationMatrix(rotation.z);
	return transform;
}

FMMatrix44 FMMatrix44::ScaleMatrix(const FMVector3& scale)
{
	FMMatrix44 mx(Identity);
	mx[0][0] = scale.x; mx[1][1] = scale.y; mx[2][2] = scale.z;
	return mx;
}

FMMatrix44 FMMatrix44::LookAtMatrix(const FMVector3& eye, const FMVector3& target, const FMVector3& up)
{
	FMMatrix44 mx;

	FMVector3 forward = (target - eye).Normalize();
	FMVector3 sideways, upward;
	if (!IsEquivalent(forward, up) && !IsEquivalent(forward, -up))
	{
		sideways = (forward ^ up).Normalize();
	}
	else if (!IsEquivalent(up, FMVector3::XAxis))
	{
		sideways = FMVector3::XAxis;
	}
	else
	{
		sideways = FMVector3::ZAxis;
	}
	upward = (sideways ^ forward);

	mx[0][0] = sideways.x;  mx[0][1] = sideways.y;  mx[0][2] = sideways.z; 
	mx[1][0] = upward.x;    mx[1][1] = upward.y;    mx[1][2] = upward.z; 
	mx[2][0] = -forward.x;	mx[2][1] = -forward.y;	mx[2][2] = -forward.z; 
	mx[3][0] = eye.x;		mx[3][1] = eye.y;		mx[3][2] = eye.z;

	mx[0][3] = mx[1][3] = mx[2][3] = 0.0f;
	mx[3][3] = 1.0f;

	return mx;
}

bool IsEquivalent(const FMMatrix44& m1, const FMMatrix44& m2)
{
	return IsEquivalent(m1.m[0][0], m2.m[0][0]) && IsEquivalent(m1.m[1][0], m2.m[1][0])
		&& IsEquivalent(m1.m[2][0], m2.m[2][0]) && IsEquivalent(m1.m[3][0], m2.m[3][0])
		&& IsEquivalent(m1.m[0][1], m2.m[0][1]) && IsEquivalent(m1.m[1][1], m2.m[1][1])
		&& IsEquivalent(m1.m[2][1], m2.m[2][1]) && IsEquivalent(m1.m[3][1], m2.m[3][1])
		&& IsEquivalent(m1.m[0][2], m2.m[0][2]) && IsEquivalent(m1.m[1][2], m2.m[1][2])
		&& IsEquivalent(m1.m[2][2], m2.m[2][2]) && IsEquivalent(m1.m[3][2], m2.m[3][2])
		&& IsEquivalent(m1.m[0][3], m2.m[0][3]) && IsEquivalent(m1.m[1][3], m2.m[1][3])
		&& IsEquivalent(m1.m[2][3], m2.m[2][3]) && IsEquivalent(m1.m[3][3], m2.m[3][3]);
}
