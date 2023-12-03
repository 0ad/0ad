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

#ifndef INCLUDED_QUATERNION
#define INCLUDED_QUATERNION

#include "Vector3D.h"

class CMatrix3D;

class CQuaternion
{
public:
	CVector3D	m_V;
	float		m_W;

public:
	CQuaternion();
	CQuaternion(float x, float y, float z, float w);

	CQuaternion operator + (const CQuaternion &quat) const;
	CQuaternion &operator += (const CQuaternion &quat);

	CQuaternion operator - (const CQuaternion &quat) const;
	CQuaternion &operator -= (const CQuaternion &quat);

	CQuaternion operator * (const CQuaternion &quat) const;
	CQuaternion &operator *= (const CQuaternion &quat);

	CQuaternion operator * (float factor) const;

	float Dot(const CQuaternion& quat) const;

	void FromEulerAngles (float x, float y, float z);
	CVector3D ToEulerAngles();

	// Convert the quaternion to matrix
	CMatrix3D ToMatrix() const;
	void ToMatrix(CMatrix3D& result) const;

	// Sphere interpolation
	void Slerp(const CQuaternion& from, const CQuaternion& to, float ratio);

	// Normalised linear interpolation
	void Nlerp(const CQuaternion& from, const CQuaternion& to, float ratio);

	// Create a quaternion from axis/angle representation of a rotation
	void FromAxisAngle(const CVector3D& axis, float angle);

	// Convert the quaternion to axis/angle representation of a rotation
	void ToAxisAngle(CVector3D& axis, float& angle);

	// Normalize this quaternion
	void Normalize();

	// Rotate a vector by this quaternion. Assumes the quaternion is normalised.
	CVector3D Rotate(const CVector3D& vec) const;

	// Calculate q^-1. Assumes the quaternion is normalised.
	CQuaternion GetInverse() const;
};

#endif
