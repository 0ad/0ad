/************************************************************
 *
 * File Name: Quaternion.Cpp
 *
 * Description: 
 *
 ************************************************************/

#include "precompiled.h"

#include "Quaternion.h"
#include "MathUtil.h"

const float EPSILON=0.0001f;


CQuaternion::CQuaternion()
{
	m_V.Clear();
	m_W = 1;
}

CQuaternion::CQuaternion(float x, float y, float z, float w)
: m_V(x, y, z), m_W(w)
{
}

//quaternion addition
CQuaternion CQuaternion::operator + (const CQuaternion &quat) const
{
	CQuaternion Temp;

	Temp.m_W = m_W + quat.m_W;
	Temp.m_V = m_V + quat.m_V;

	return Temp;
}

//quaternion addition/assignment
CQuaternion &CQuaternion::operator += (const CQuaternion &quat)
{
	m_W += quat.m_W;
	m_V += quat.m_V;

	return (*this);
}

//quaternion multiplication
CQuaternion CQuaternion::operator * (const CQuaternion &quat) const
{
	CQuaternion Temp;

	Temp.m_W = (m_W * quat.m_W) - (m_V.Dot(quat.m_V));
	Temp.m_V = (m_V.Cross(quat.m_V)) + (quat.m_V * m_W) + (m_V * quat.m_W);

	return Temp;
}

//quaternion multiplication/assignment
CQuaternion &CQuaternion::operator *= (const CQuaternion &quat)
{
	(*this) = (*this) * quat;

	return (*this);
}


void CQuaternion::FromEulerAngles (float x, float y, float z)
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
CVector3D CQuaternion::ToEulerAngles()
{
    float heading, attitude, bank;
	float sqw = m_W * m_W;
    float sqx = m_V.X*m_V.X;
    float sqy = m_V.Y*m_V.Y;
    float sqz = m_V.Z*m_V.Z;
	float unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor
	float test = m_V.X*m_V.Y + m_V.Z*m_W;
	if (test > (.5f-EPSILON)*unit) 
	{ // singularity at north pole
		heading = 2 * atan2( m_V.X, m_W);
		attitude = PI/2;
		bank = 0;
	}
	else if (test < (-.5f+EPSILON)*unit) 
	{ // singularity at south pole
		heading = -2 * atan2(m_V.X, m_W);
		attitude = -PI/2;
		bank = 0;
	}
	else 
	{
		heading = atan2(2.f * (m_V.X*m_V.Y + m_V.Z*m_W),(sqx - sqy - sqz + sqw));
	    bank = atan2(2.f * (m_V.Y*m_V.Z + m_V.X*m_W),(-sqx - sqy + sqz + sqw));  
		attitude = asin(-2.f * (m_V.X*m_V.Z - m_V.Y*m_W));
	}
	return CVector3D(bank, attitude, heading);
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

///////////////////////////////////////////////////////////////////////////////////////////////
// FromAxisAngle: create a quaternion from axis/angle representation of a rotation
void CQuaternion::FromAxisAngle(const CVector3D& axis, float angle)
{
	float sinHalfTheta=(float) sin(angle/2);
	float cosHalfTheta=(float) cos(angle/2);

	m_V.X=axis.X*sinHalfTheta;
	m_V.Y=axis.Y*sinHalfTheta;
	m_V.Z=axis.Z*sinHalfTheta;
	m_W=cosHalfTheta;
}

///////////////////////////////////////////////////////////////////////////////////////////////
// ToAxisAngle: convert the quaternion to axis/angle representation of a rotation
void CQuaternion::ToAxisAngle(CVector3D& axis, float& angle)
{
	CQuaternion q = *this;
	q.Normalize();
	angle = acosf(q.m_W) * 2.f;
	float sin_a = sqrtf(1.f - q.m_W * q.m_W);
	if (fabsf(sin_a) < 0.0005f) sin_a = 1.f;
	axis.X = q.m_V.X / sin_a;
	axis.Y = q.m_V.Y / sin_a;
	axis.Z = q.m_V.Z / sin_a;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// Normalize: normalize this quaternion
void CQuaternion::Normalize()
{
	float lensqrd=SQR(m_V.X)+SQR(m_V.Y)+SQR(m_V.Z)+SQR(m_W);
	if (lensqrd>0) {
		float invlen=1.0f/sqrtf(lensqrd);
		m_V*=invlen;
		m_W*=invlen;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

CVector3D CQuaternion::Rotate(const CVector3D& vec) const
{
	// v' = q * v * q^-1
	// (where v is the quat. with w=0, xyz=vec)
	
	return (*this * CQuaternion(vec.X, vec.Y, vec.Z, 0.f) * GetInverse()).m_V;
}

CQuaternion CQuaternion::GetInverse() const
{
	float lensqrd = SQR(m_V.X) + SQR(m_V.Y) + SQR(m_V.Z) + SQR(m_W);
	return CQuaternion(-m_V.X/lensqrd, -m_V.Y/lensqrd, -m_V.Z/lensqrd, m_W/lensqrd);
}
