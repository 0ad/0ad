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

class CQuaternion
{
	public:
		CVector3D	m_V;
		float		m_W;

	public:
		CQuaternion();
		
		//quaternion addition
		CQuaternion operator + (CQuaternion &quat);
		//quaternion addition/assignment
		CQuaternion &operator += (CQuaternion &quat);

		//quaternion multiplication
		CQuaternion operator * (CQuaternion &quat);
		//quaternion multiplication/assignment
		CQuaternion &operator *= (CQuaternion &quat);
		
		void FromEularAngles (float x, float y, float z);
		
		//convert the quaternion to matrix
		CMatrix3D ToMatrix ();

		//sphere interpolation
		void Slerp(CQuaternion &from, CQuaternion &to, float ratio);
};

#endif
