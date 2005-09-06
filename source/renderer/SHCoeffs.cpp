//----------------------------------------------------------------
//
// Name:		SHCoeffs.h
// Last Update: 25/11/03
// Author:		Rich Cross
// Contact:		rich@0ad.wildfiregames.com
//
// Description: implementation of 9 component spherical harmonic
//	lighting
//----------------------------------------------------------------

#include "precompiled.h"

#include "SHCoeffs.h"

CSHCoeffs::CSHCoeffs()
{
	Clear();
}

void CSHCoeffs::Clear()
{
	for (int i=0;i<9;i++) {
		_data[i].Clear();
	}
}

void CSHCoeffs::AddAmbientLight(const RGBColor& color)
{
	_data[0]+=color;
}

void CSHCoeffs::AddDirectionalLight(const CVector3D& lightDir,const RGBColor& lightColor)
{
	CVector3D dirToLight(-lightDir.X,-lightDir.Y,-lightDir.Z);

	const float normalisation = PI*16/17;
	const float c1 = SQR(0.282095f) * normalisation * 1.0f;
	const float c2 = SQR(0.488603f) * normalisation * (2.0f/3.0f);
	const float c3 = SQR(1.092548f) * normalisation * (1.0f/4.0f);
	const float c4 = SQR(0.315392f) * normalisation * (1.0f/4.0f);
	const float c5 = SQR(0.546274f) * normalisation * (1.0f/4.0f);

	_data[0]+=lightColor*c1;
	_data[1]+=lightColor*c2*dirToLight.X;
	_data[2]+=lightColor*c2*dirToLight.Y;
	_data[3]+=lightColor*c2*dirToLight.Z;
	_data[4]+=lightColor*c3*dirToLight.X*dirToLight.Z;
	_data[5]+=lightColor*c3*dirToLight.Z*dirToLight.Y;
	_data[6]+=lightColor*c3*dirToLight.Y*dirToLight.X;
	_data[7]+=lightColor*c4*(3.0f*SQR(dirToLight.Z)-1.0f);
	_data[8]+=lightColor*c5*(SQR(dirToLight.X)-SQR(dirToLight.Y));
}

void CSHCoeffs::Evaluate(const CVector3D& normal,RGBColor& color,RGBColor mod) const
{
#if 1
	float c4=normal.X*normal.Z;
	float c5=normal.Z*normal.Y;
	float c6=normal.Y*normal.X;
	float c7=(3*SQR(normal.Z)-1.0f);
	float c8=(SQR(normal.X)-SQR(normal.Y));

#define DO(C) \
		color.C = mod.C * (\
		  _data[0].C \
		+ _data[1].C*normal.X \
		+ _data[2].C*normal.Y \
		+ _data[3].C*normal.Z \
		+ _data[4].C*c4 \
		+ _data[5].C*c5 \
		+ _data[6].C*c6 \
		+ _data[7].C*c7 \
		+ _data[8].C*c8)
	DO(X);
	DO(Y);
	DO(Z);
#undef DO

#else
	// debug aid: output quantised normal 
	color=RGBColor((normal.X+1)*0.5,(normal.Y+1)*0.5,(normal.Z+1)*0.5);
#endif
}
