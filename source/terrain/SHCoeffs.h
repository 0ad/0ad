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

#ifndef __SHCOEFFS_H
#define __SHCOEFFS_H

#include "Color.h"

class CSHCoeffs
{
public:
	CSHCoeffs();

	void AddAmbientLight(const RGBColor& color);
	void AddDirectionalLight(const CVector3D& lightDir,const RGBColor& lightColor);

	void Evaluate(const CVector3D& normal,RGBColor& color);

	const RGBColor* GetCoefficients() const { return _data; }

private:
	RGBColor _data[9];
};


#endif
