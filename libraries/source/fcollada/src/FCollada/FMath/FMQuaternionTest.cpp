/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMQuaternion.h"
#include "FUtils/FUTestBed.h"

//
// IMPORTANT: Many of the comparison numbers below were gotten using Maya.
//

TESTSUITE_START(FMQuaternion)

TESTSUITE_TEST(0, SimpleOperations)
	// Simple constructor.
	FMQuaternion q(0.1f, 0.2f, 0.5f, 0.6f);
	PassIf(IsEquivalent(0.1f, q.x));
	PassIf(IsEquivalent(0.2f, q.y));
	PassIf(IsEquivalent(0.5f, q.z));
	PassIf(IsEquivalent(0.6f, q.w));

	// Copy constructor.
	FMQuaternion q2 = q;
	PassIf(IsEquivalent(q2, q));
	PassIf(IsEquivalent(q2.x, 0.1f));
	PassIf(IsEquivalent(q2.y, 0.2f));
	PassIf(IsEquivalent(q2.z, 0.5f));
	PassIf(IsEquivalent(q2.w, 0.6f));

	// Basic arithmetic
	PassIf(IsEquivalent(q.Length(), 0.8124f));
	PassIf(IsEquivalent(q.LengthSquared(), 0.66f));
	q.NormalizeIt();
	PassIf(IsEquivalent(q.Length(), 1.0f));
	PassIf(IsEquivalent(q.LengthSquared(), 1.0f));

TESTSUITE_TEST(1, AngleAxis)
	// Angle-axis constructor.
	FMQuaternion aa(FMVector3::XAxis, 0.0f);
	PassIf(IsEquivalent(aa, FMQuaternion(0.0f, 0.0f, 0.0f, 1.0f)));
	FMQuaternion aa2(FMVector3(0.5f, -0.51f, 0.7f), FMath::DegToRad(30.0f));
	PassIf(IsEquivalent(aa2, FMQuaternion(0.1294f, -0.13199f, 0.181164f, 0.965926f)));
	FMQuaternion aa3(FMVector3(0.8f, 0.35f, 0.48734f), FMath::DegToRad(-45.0f));
	PassIf(IsEquivalent(aa3, FMQuaternion(-0.3061467f, -0.133939f, -0.1864969f, 0.9238795f)));

	// Angle-axis extraction.
	FMVector3 axis; float angle;
	aa.ToAngleAxis(axis, angle);
	PassIf(IsEquivalent(angle, 0.0f)); // Axis is irrelevant, since no rotation.
	aa2.ToAngleAxis(axis, angle);
	PassIf((IsEquivalent(axis, FMVector3(0.5f, -0.51f, 0.7f)) && IsEquivalent(angle, FMath::DegToRad(30.0f)))
		|| (IsEquivalent(axis, FMVector3(-0.5f, 0.51f, -0.7f)) && IsEquivalent(angle, FMath::DegToRad(-30.0f))));
	aa3.ToAngleAxis(axis, angle);
	PassIf((IsEquivalent(axis, FMVector3(0.8f, 0.35f, 0.48734f)) && IsEquivalent(angle, FMath::DegToRad(-45.0f)))
		|| (IsEquivalent(axis, FMVector3(-0.8f, -0.35f, -0.48734f)) && IsEquivalent(angle, FMath::DegToRad(45.0f))));

TESTSUITE_TEST(2, Matrix)
	// Matrix constructor.
	FMMatrix44 mx = FMMatrix44::AxisRotationMatrix(FMVector3(0.5f, -0.51f, 0.7f), FMath::DegToRad(30.0f));
	FMQuaternion aa = FMQuaternion::MatrixRotationQuaternion(mx);
	PassIf(IsEquivalent(aa, FMQuaternion(0.1294f, -0.13199f, 0.181164f, 0.965926f)));
	FMVector3 eulerAngles = FMVector3(FMath::Pif / 2.0f, FMath::Pif / 6.0f, 3.0f * FMath::Pif / 8.0f);
	FMMatrix44 mx2 = FMMatrix44::EulerRotationMatrix(eulerAngles);
	FMQuaternion aa2 = FMQuaternion::MatrixRotationQuaternion(mx2);
	FMQuaternion aaE = FMQuaternion::EulerRotationQuaternion(eulerAngles.x, eulerAngles.y, eulerAngles.z);
	PassIf(IsEquivalent(aa2, aaE));

	// Matrix extraction.
	FMMatrix44 mx3;
	mx3 = aa.ToMatrix();
	PassIf(IsEquivalent(mx3, mx));
	mx3 = aa2.ToMatrix();
	PassIf(IsEquivalent(mx3, mx2));
	FMVector3 eulerOut = aa2.ToEuler(&eulerAngles);
	PassIf(IsEquivalent(eulerOut, eulerAngles));

TESTSUITE_TEST(3, Transforms)
	FMQuaternion aa2(FMVector3(0.5f, -0.51f, 0.7f), FMath::DegToRad(30.0f));
	FMQuaternion aa3(FMVector3(0.8f, 0.35f, 0.48734f), FMath::DegToRad(-45.0f));
	FMQuaternion aa(FMVector3::XAxis, 0.0f);
	FMVector3 v(0.4f, 0.2f, 1.0f);
	FMVector3 v2(-0.5f, 0.0f, 0.6f);

	// Check identity transforms
	FMQuaternion out = aa * aa3;
	PassIf(IsEquivalent(out, aa3));
	out = aa2 * aa;
	PassIf(IsEquivalent(out, aa2));
	out = aa2 * aa3;
	PassIf(IsEquivalent(out, FMQuaternion(-0.22504311309918154f, -0.21998962990348064f, 0.044972560080678750f, 0.94812321394497479f)));
	out = aa3 * aa2;
	PassIf(IsEquivalent(out, FMQuaternion(-0.12728124831602353f, -0.28264878231341289f, -0.070509012633218196f, 0.94812321394497479f)));
	FMVector3 outV = aa * v;
	PassIf(IsEquivalent(outV, v));
	outV = aa * v2;
	PassIf(IsEquivalent(outV, v2));
	outV = aa2 * v;
	PassIf(IsEquivalent(outV, FMVector3(0.72484444514032287f, 0.22868005480411205f, 0.78886372007113525f)));
	outV = aa2 * v2;
	PassIf(IsEquivalent(outV, FMVector3(-0.26863363500894472f, 0.31336932927135464f, 0.66305027126465488f)));
	outV = aa3 * v;
	PassIf(IsEquivalent(outV, FMVector3(0.66698327793133894f, -0.19648504827654456f, 0.84647914252165402f)));
	outV = aa3 * v2;
	PassIf(IsEquivalent(outV, FMVector3(-0.23027227753990964f, -0.52274186502691655f, 0.53264964972765538f)));

TESTSUITE_TEST(4, Bugzilla455)
	double matrixData[16] = { -0.046491116, 0.00000000, -0.99891871, 0.00000000, 0.00000000, 1.0000000, 0.00000000, 0.00000000, 0.99891871, 0.00000000, -0.046491116, 0.00000000, -28652.848, 13.103017, -9445.6309, 1.0000000 };
	double quaternionData[4] = { 0.00000000, -0.72335714, 0.00000000, 0.69047403 };
	FMMatrix44 rotationMx(matrixData);
	FMQuaternion q = FMQuaternion::MatrixRotationQuaternion(rotationMx);
	PassIf(IsEquivalent(q, FMQuaternion(quaternionData)));

TESTSUITE_END
