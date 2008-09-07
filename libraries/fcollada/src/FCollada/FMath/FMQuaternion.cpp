/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMath.h"
#include "FMVector3.h"
#include "FMQuaternion.h"

//
// Helpers
//

static void AngleApproach(float pval, float& val)
{
	while (val - pval > FMath::Pi) val -= FMath::Pif * 2.0f;
	while (val - pval < -FMath::Pi) val += FMath::Pif * 2.0f;
}

static void PatchEuler(float* pval, float* val)
{
	// Approach these Eulers to the previous value.
	for (int i = 0; i < 3; ++i) AngleApproach(pval[i], val[i]);
	float distanceSq = 0.0f; for (int i = 0; i < 3; ++i) distanceSq += (val[i] - pval[i]) * (val[i] - pval[i]);

	// All quaternions can be expressed two ways. Check if the second way is better.
	float alternative[3] = { val[0] + FMath::Pif, FMath::Pif - val[1], val[2] + FMath::Pif };
	for (int i = 0; i < 3; ++i) AngleApproach(pval[i], alternative[i]);
	float alternateDistanceSq = 0.0f; for (int i = 0; i < 3; ++i) alternateDistanceSq += (alternative[i] - pval[i]) * (alternative[i] - pval[i]);

	if (alternateDistanceSq < distanceSq)
	{
		// Pick the alternative
		for (int i = 0; i < 3; ++i) val[i] = alternative[i];
	}
}

//
// FMQuaternion
//

const FMQuaternion FMQuaternion::Zero = FMQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
const FMQuaternion FMQuaternion::Identity = FMQuaternion(0.0f, 0.0f, 0.0f, 1.0f);

FMQuaternion::FMQuaternion(const float* values)
{
	if (values != NULL)
	{
		x = (*values++);
		y = (*values++);
		z = (*values++);
		w = (*values);
	}
}

FMQuaternion::FMQuaternion(const double* values)
{
	if (values != NULL)
	{
		x = (float) (*values++);
		y = (float) (*values++);
		z = (float) (*values++);
		w = (float) (*values);
	}
}

FMQuaternion::FMQuaternion(const FMVector3& axis, float angle)
{
	float s = sinf(angle / 2.0f);
	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = cosf(angle / 2.0f);
}

FMQuaternion FMQuaternion::operator*(const FMQuaternion& q) const
{
	FMQuaternion r;
	r.x = q.w * x + q.x * w + q.y * z - q.z * y;
	r.y = q.w * y + q.y * w + q.z * x - q.x * z;
	r.z = q.w * z + q.z * w + q.x * y - q.y * x;
	r.w = q.w * w - q.x * x - q.y * y - q.z * z;
	return r;
}

FMVector3 FMQuaternion::operator*(const FMVector3& v) const
{
	FMQuaternion out = (*this) * FMQuaternion(v.x, v.y, v.z, 0.0f) * (~(*this));
	return FMVector3(out.x, out.y, out.z);
}

FMQuaternion FMQuaternion::slerp(const FMQuaternion& other, float t) const
{
	float theta, st, sut, sout, interp1, interp2;
	float dot = x * other.x + y * other.y + z * other.z +
				w * other.w;

	if (IsEquivalent(dot, 1.0f)) return *this;

	// algorithm taken from Shoemake's paper
	theta = (float) acos(dot);
	theta = theta < 0.0 ? -theta : theta;

	st = (float) sin(theta);
	sut = (float) sin(t*theta);
	sout = (float) sin((1-t)*theta);
	interp1 = sout/st;
	interp2 = sut/st;

	FMQuaternion result;
	result.x = interp1*x + interp2*other.x;
	result.y = interp1*y + interp2*other.y;
	result.z = interp1*z + interp2*other.z;
	result.w = interp1*w + interp2*other.w;

	result.NormalizeIt();
	return result;
}

void FMQuaternion::ToAngleAxis(FMVector3& axis, float& angle) const
{
	angle = 2.0f * acosf(w);
	float s = sinf(angle / 2.0f);
	if (!IsEquivalent(s, 0.0f))
	{
		axis.x = x / s;
		axis.y = y / s;
		axis.z = z / s;
		axis.NormalizeIt();
	}
	else
	{
		// If s == 0, then angle == 0 and there is no rotation: assign any axis.
		axis = FMVector3::XAxis;
	}
}

FMVector3 FMQuaternion::ToEuler(FMVector3* previousAngles) const
{
	FMVector3 angles;

	// Convert the quaternion into Euler angles.
	float siny = 2.0f * (x * z + y * w);
	if (siny > 1.0f - FLT_TOLERANCE) // singularity at north pole
	{ 
		angles.y = (float) FMath::Pi / 2;

		angles.x = 2 * atan2(x,w);
		angles.z = 0;
	}
	else if (siny < -1.0f + FLT_TOLERANCE) // singularity at south pole
	{
		angles.y = (float) -FMath::Pi / 2;

		angles.x = -2 * atan2(x,w);
		angles.z = 0;
	}
	else
	{
		// [GLaforte] Derived on 18-07-2007.
		angles.y = asinf(siny);
		angles.x = atan2f((x*w-y*z), 1.0f - 2.0f * (x*x+y*y));
		angles.z = atan2f(2.0f * (z*w-x*y), 1.0f - 2.0f * (y*y+z*z));
	}

	// Patch to the closest Euler angles.
	if (previousAngles != NULL)
	{
		PatchEuler((float*) previousAngles, (float*) angles);
	}

	return angles;
}

FMMatrix44 FMQuaternion::ToMatrix() const
{
	FMMatrix44 tm = FMMatrix44::Identity;
	SetToMatrix(tm);
	return tm;
	//FMVector3 axis; float angle;
	//ToAngleAxis(axis, angle);
	//FMMatrix44 m;
	//SetToMatrix(m);
	//return FMMatrix44::AxisRotationMatrix(axis, angle);
}


void FMQuaternion::SetToMatrix(FMMatrix44& m) const
{
	float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz, den;

	if (*this == FMQuaternion::Identity) 
	{
		m = FMMatrix44::Identity;
		return;		
	}
	// For unit q, just set s = 2.0; or or set xs = q.x + q.x, etc 
	den =  (x*x + y*y + z*z + w*w);
	if (den==0.0) {  s = (float)1.0; }
	else s = (float)2.0/den;

	xs = x * s;   ys = y * s;  zs = z * s;
	wx = w * xs;  wy = w * ys; wz = w * zs;
	xx = x * xs;  xy = x * ys; xz = x * zs;
	yy = y * ys;  yz = y * zs; zz = z * zs;

	m[0][0] = (float)1.0 - (yy +zz);
	m[1][0] = xy - wz; 
	m[2][0] = xz + wy; 

	m[0][1] = xy + wz; 
	m[1][1] = (float)1.0 - (xx +zz);
	m[2][1] = yz - wx; 

	m[0][2] = xz - wy; 
	m[1][2] = yz + wx; 
	m[2][2] = (float)1.0 - (xx + yy);
}


FMQuaternion FMQuaternion::EulerRotationQuaternion(float x, float y, float z)
{
	FMQuaternion qx(FMVector3::XAxis, x);
	FMQuaternion qy(FMVector3::YAxis, y);
	FMQuaternion qz(FMVector3::ZAxis, z);
	return qz * qy * qx;
}

FMQuaternion FMQuaternion::MatrixRotationQuaternion(const FMMatrix44& mat)
{
	FMQuaternion q;

	float tr,s;
	
	tr = 1.0f + mat[0][0] + mat[1][1] + mat[2][2];
	if (tr > 0.00001f)
	{
		s = sqrtf(tr) * 2.0f;
		q.x = (mat[1][2] - mat[2][1]) / s;
		q.y = (mat[2][0] - mat[0][2]) / s;
		q.z = (mat[0][1] - mat[1][0]) / s;
		q.w = s * 0.25f;
	}
	else if (mat[0][0] > mat[1][1])
	{
		s = sqrtf(1.0f + mat[0][0] - mat[1][1] - mat[2][2]) * 2.0f;
		q.x = 0.25f * s;
		q.y = (mat[0][1] + mat[1][0]) / s;
		q.z = (mat[2][0] + mat[0][2]) / s;
		q.w = (mat[1][2] - mat[2][1]) / s;
	}
	else if (mat[1][1] > mat[2][2])
	{
		s = sqrtf(1.0f + mat[1][1] - mat[0][0] - mat[2][2]) * 2.0f;
		q.x = (mat[0][1] + mat[1][0]) / s;
		q.y = 0.25f * s;
		q.z = (mat[1][2] + mat[2][1]) / s;
		q.w = (mat[2][0] - mat[0][2]) / s;
	}
	else
	{
		s  = sqrtf(1.0f + mat[2][2] - mat[0][0] - mat[1][1]) * 2.0f;
		q.x = (mat[2][0] + mat[0][2]) / s;
		q.y = (mat[1][2] + mat[2][1]) / s;
		q.z = 0.25f * s;
		q.w = (mat[0][1] - mat[1][0]) / s;
	}
	return q;
}

