#include "precompiled.h"

#include "Maths.h"

#include "FCollada.h"


void DumpMatrix(const FMMatrix44& m)
{
	Log(LOG_INFO, "\n[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]\n[%f %f %f %f]",
		m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
		m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
		m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
		m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]
	);
}

FMMatrix44 DecomposeToScaleMatrix(const FMMatrix44& m)
{
	FMVector3 scale, rotation, translation;
	float inverted;
	m.Decompose(scale, rotation, translation, inverted);
	return FMMatrix44::ScaleMatrix(scale);
}

/*
FMMatrix44 operator+ (const FMMatrix44& a, const FMMatrix44& b)
{
	FMMatrix44 r;
	for (int x = 0; x < 4; ++x)
		for (int y = 0; y < 4; ++y)
			r[x][y] = a[x][y] + b[x][y];
	return r;
}

FMMatrix44 operator/ (const FMMatrix44& a, const float b)
{
	FMMatrix44 r;
	for (int x = 0; x < 4; ++x)
		for (int y = 0; y < 4; ++y)
			r[x][y] = a[x][y] / b;
	return r;
}

FMMatrix44 QuatToMatrix(float x, float y, float z, float w)
{
	FMMatrix44 r;

	r[0][0] = 1.0f - (y*y*2 + z*z*2);
	r[1][0] = x*y*2 - w*z*2;
	r[2][0] = x*z*2 + w*y*2;
	r[3][0] = 0;

	r[0][1] = x*y*2 + w*z*2;
	r[1][1] = 1.0f - (x*x*2 + z*z*2);
	r[2][1] = y*z*2 - w*x*2;
	r[3][1] = 0;

	r[0][2] = x*z*2 - w*y*2;
	r[1][2] = y*z*2 + w*x*2;
	r[2][2] = 1.0f - (x*x*2 + y*y*2);
	r[3][2] = 0;

	r[0][3] = 0;
	r[1][3] = 0;
	r[2][3] = 0;
	r[3][3] = 1;

	return r;
}
*/
