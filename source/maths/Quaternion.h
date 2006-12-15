/************************************************************
 *
 * File Name: Quaternion.H
 *
 * Description: 
 *
 ************************************************************/

#ifndef QUATERNION_H
#define QUATERNION_H

#include "Matrix3D.h"
#include "Vector3D.h"

class CQuaternion
{
public:
	CVector3D	m_V;
	float		m_W;

public:
	CQuaternion();
	CQuaternion(float x, float y, float z, float w);
	
	// Quaternion addition
	CQuaternion operator + (const CQuaternion &quat) const;
	// Quaternion addition/assignment
	CQuaternion &operator += (const CQuaternion &quat);

	// Quaternion multiplication
	CQuaternion operator * (const CQuaternion &quat) const;
	// Quaternion multiplication/assignment
	CQuaternion &operator *= (const CQuaternion &quat);
	
	void FromEulerAngles (float x, float y, float z);
	CVector3D ToEulerAngles();
	
	// Convert the quaternion to matrix
	CMatrix3D ToMatrix() const;
	void ToMatrix(CMatrix3D& result) const;

	// Sphere interpolation
	void Slerp(const CQuaternion& from, const CQuaternion& to, float ratio);

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
