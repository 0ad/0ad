//----------------------------------------------------------------
//
// Name:		LightEnv.h
// Last Update: 25/11/03
// Author:		Rich Cross
// Contact:		rich@0ad.wildfiregames.com
//
// Description: class describing current lighting environment - 
//	at the minute, this is only sunlight and ambient light 
//	parameters; will be extended to handle dynamic lights at some
//  later date
//----------------------------------------------------------------


#ifndef __LIGHTENV_H
#define __LIGHTENV_H

#include "Color.h"
#include "Vector3D.h"

class CLightEnv 
{
public:
	RGBColor m_SunColor;
	float m_Elevation;
	float m_Rotation;
	RGBColor m_TerrainAmbientColor;
	RGBColor m_UnitsAmbientColor;

	// get sun direction from a rotation and elevation; defined such that:
	//	0 rotation    = (0,0,1)
	// PI/2 rotation  = (-1,0,0)
	//	0 elevation	  = (0,0,0)
	// PI/2 elevation = (0,-1,0)
	void GetSunDirection(CVector3D& lightdir) const {
		lightdir.Y=-float(sin(m_Elevation));
		float scale=1+lightdir.Y;
		lightdir.X=scale*float(sin(m_Rotation));
		lightdir.Z=scale*float(cos(m_Rotation));
		lightdir.Normalize();
	}
};

#endif
