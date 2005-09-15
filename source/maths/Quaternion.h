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
	
	//quaternion addition
	CQuaternion operator + (const CQuaternion &quat) const;
	//quaternion addition/assignment
	CQuaternion &operator += (const CQuaternion &quat);

	//quaternion multiplication
	CQuaternion operator * (const CQuaternion &quat) const;
	//quaternion multiplication/assignment
	CQuaternion &operator *= (const CQuaternion &quat);
	
	void FromEulerAngles (float x, float y, float z);
	
	//convert the quaternion to matrix
	CMatrix3D ToMatrix() const;
	void ToMatrix(CMatrix3D& result) const;

	//sphere interpolation
	void Slerp(const CQuaternion& from,const CQuaternion& to, float ratio);

	// create a quaternion from axis/angle representation of a rotation
	void FromAxisAngle(const CVector3D& axis,float angle);

	// normalize this quaternion
	void Normalize();

	// rotate a vector by this quaternion
	CVector3D Rotate(const CVector3D& vec) const;

	// calculate q^-1
	CQuaternion GetInverse() const;
};

#endif
