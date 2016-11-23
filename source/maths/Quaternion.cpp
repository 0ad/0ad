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

#include "precompiled.h"

#include "Quaternion.h"
#include "MathUtil.h"

const float EPSILON=0.0001f;


CQuaternion::CQuaternion() :
	m_W(1)
{
}

CQuaternion::CQuaternion(float x, float y, float z, float w) :
	m_V(x, y, z), m_W(w)
{
}

CQuaternion CQuaternion::operator + (const CQuaternion &quat) const
{
	CQuaternion Temp;
	Temp.m_W = m_W + quat.m_W;
	Temp.m_V = m_V + quat.m_V;
	return Temp;
}

CQuaternion &CQuaternion::operator += (const CQuaternion &quat)
{
	*this = *this + quat;
	return *this;
}

CQuaternion CQuaternion::operator - (const CQuaternion &quat) const
{
	CQuaternion Temp;
	Temp.m_W = m_W - quat.m_W;
	Temp.m_V = m_V - quat.m_V;
	return Temp;
}

CQuaternion &CQuaternion::operator -= (const CQuaternion &quat)
{
	*this = *this - quat;
	return *this;
}

CQuaternion CQuaternion::operator * (const CQuaternion &quat) const
{
	CQuaternion Temp;
	Temp.m_W = (m_W * quat.m_W) - (m_V.Dot(quat.m_V));
	Temp.m_V = (m_V.Cross(quat.m_V)) + (quat.m_V * m_W) + (m_V * quat.m_W);
	return Temp;
}

CQuaternion &CQuaternion::operator *= (const CQuaternion &quat)
{
	*this = *this * quat;
	return *this;
}

CQuaternion CQuaternion::operator * (float factor) const
{
	CQuaternion Temp;
	Temp.m_W = m_W * factor;
	Temp.m_V = m_V * factor;
	return Temp;
}


float CQuaternion::Dot(const CQuaternion& quat) const
{
	return
		m_V.X * quat.m_V.X +
		m_V.Y * quat.m_V.Y +
		m_V.Z * quat.m_V.Z +
		m_W   * quat.m_W;
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

	QRoll.m_V = CVector3D(sr, 0, 0);
	QRoll.m_W = cr;

	QPitch.m_V = CVector3D(0, sp, 0);
	QPitch.m_W = cp;

	QYaw.m_V = CVector3D(0, 0, sy);
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
		attitude = (float)M_PI/2;
		bank = 0;
	}
	else if (test < (-.5f+EPSILON)*unit)
	{ // singularity at south pole
		heading = -2 * atan2(m_V.X, m_W);
		attitude = -(float)M_PI/2;
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
	float wx, wy, wz, xx, xy, xz, yy, yz, zz;

	// calculate coefficients
	xx = m_V.X * m_V.X * 2.f;
	xy = m_V.X * m_V.Y * 2.f;
	xz = m_V.X * m_V.Z * 2.f;

	yy = m_V.Y * m_V.Y * 2.f;
	yz = m_V.Y * m_V.Z * 2.f;

	zz = m_V.Z * m_V.Z * 2.f;

	wx = m_W * m_V.X * 2.f;
	wy = m_W * m_V.Y * 2.f;
	wz = m_W * m_V.Z * 2.f;

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

void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float ratio)
{
	float to1[4];
	float omega, cosom, sinom, scale0, scale1;

	// calc cosine
	cosom = from.Dot(to);


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

void CQuaternion::Nlerp(const CQuaternion& from, const CQuaternion& to, float ratio)
{
	float c = from.Dot(to);
	if (c < 0.f)
		*this = from - (to + from) * ratio;
	else
		*this = from + (to - from) * ratio;
	Normalize();
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
	// (x,y,z,w)^-1 = (-x/l^2, -y/l^2, -z/l^2, w/l^2) where l^2=x^2+y^2+z^2+w^2
	// Since we're only using quaternions for rotation, they should always have unit
	// length, so assume l=1
	return CQuaternion(-m_V.X, -m_V.Y, -m_V.Z, m_W);
}
