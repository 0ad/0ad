/************************************************************
 *
 * File Name: Quaternion.Cpp
 *
 * Description: 
 *
 ************************************************************/

#include "Quaternion.h"

const float EPSILON=0.0001f;


CQuaternion::CQuaternion()
{
	m_V.Clear ();
	m_W = 0;
}

//quaternion addition
CQuaternion CQuaternion::operator + (CQuaternion &quat)
{
	CQuaternion Temp;

	Temp.m_W = m_W + quat.m_W;
	Temp.m_V = m_V + quat.m_V;

	return Temp;
}

//quaternion addition/assignment
CQuaternion &CQuaternion::operator += (CQuaternion &quat)
{
	m_W += quat.m_W;
	m_V += quat.m_V;

	return (*this);
}

//quaternion multiplication
CQuaternion CQuaternion::operator * (CQuaternion &quat)
{
	CQuaternion Temp;

	Temp.m_W = (m_W * quat.m_W) - (m_V.Dot(quat.m_V));
	Temp.m_V = (m_V.Cross(quat.m_V)) + (quat.m_V * m_W) + (m_V * quat.m_W);

	return Temp;
}

//quaternion multiplication/assignment
CQuaternion &CQuaternion::operator *= (CQuaternion &quat)
{
	(*this) = (*this) * quat;

	return (*this);
}


void CQuaternion::FromEularAngles (float x, float y, float z)
{
	float cr, cp, cy;
	float sr, sp, sy;

	CQuaternion QRoll, QPitch, QYaw;

	cr = cosf(x * 0.5f);
	cp = cosf(y * 0.5f);
	cy = cosf(z * 0.5f);

	sr = sinf(x * 0.5f);
	sp = sinf(y * 0.5f);
	sy = sinf(z * 0.5f);

	QRoll.m_V.Set (sr,0,0);
	QRoll.m_W = cr;

	QPitch.m_V.Set (0,sp,0);
	QPitch.m_W = cp;

	QYaw.m_V.Set (0,0,sy);
	QYaw.m_W = cy;

	(*this) = QYaw * QPitch * QRoll;
}

CMatrix3D CQuaternion::ToMatrix () const
{
	CMatrix3D result;
	ToMatrix(result);
	return result;
}

void CQuaternion::ToMatrix(CMatrix3D& result) const
{
	float x2, y2, z2;
	float wx, wy, wz, xx, xy, xz, yy, yz, zz;

	// calculate coefficients
	x2 = m_V.X + m_V.X;
	y2 = m_V.Y + m_V.Y; 
	z2 = m_V.Z + m_V.Z;

	xx = m_V.X * x2;
	xy = m_V.X * y2;
	xz = m_V.X * z2;
	
	yy = m_V.Y * y2;
	yz = m_V.Y * z2;
	
	zz = m_V.Z * z2;

	wx = m_W * x2;
	wy = m_W * y2;
	wz = m_W * z2;

	result._11 = 1.0f - (yy + zz);
	result._12 = xy - wz;
	result._13 = xz + wy;
	result._14 = 0;

	result._21 = xy + wz;
	result._22 = 1.0f - (xx + zz);
	result._23 = yz - wx;
	result._24 = 0;

	result._31 = xz - wy;
	result._32 = yz + wx;
	result._33 = 1.0f - (xx + yy);
	result._34 = 0;

	result._41 = 0;
	result._42 = 0;
	result._43 = 0;
	result._44 = 1;
}

void CQuaternion::Slerp(const CQuaternion& from,const CQuaternion& to, float ratio)
{
	float to1[4];
	float omega, cosom, sinom, scale0, scale1;
	
	// calc cosine
	cosom = from.m_V.X * to.m_V.X  +
			from.m_V.Y * to.m_V.Y  +
			from.m_V.Z * to.m_V.Z  +
			from.m_W * to.m_W;


	// adjust signs (if necessary)
	if (cosom < 0.0)
	{
		cosom = -cosom;
		to1[0] = -to.m_V.X;
		to1[1] = -to.m_V.Y;
		to1[2] = -to.m_V.Z;
		to1[3] = -to.m_W;
	}
	else
	{
		to1[0] = to.m_V.X;
		to1[1] = to.m_V.Y;
		to1[2] = to.m_V.Z;
		to1[3] = to.m_W;
	}

	// calculate coefficients
	if ((1.0f - cosom) > EPSILON)
	{
		// standard case (slerp)
		omega = acosf(cosom);
		sinom = sinf(omega);
		scale0 = sinf((1.0f - ratio) * omega) / sinom;
		scale1 = sinf(ratio * omega) / sinom;
	}
	else
	{        
		// "from" and "to" quaternions are very close 
	    //  ... so we can do a linear interpolation
		scale0 = 1.0f - ratio;
		scale1 = ratio;
	}

	// calculate final values
	m_V.X = scale0 * from.m_V.X + scale1 * to1[0];
	m_V.Y = scale0 * from.m_V.Y + scale1 * to1[1];
	m_V.Z = scale0 * from.m_V.Z + scale1 * to1[2];
	m_W	  = scale0 * from.m_W + scale1 * to1[3];
}
