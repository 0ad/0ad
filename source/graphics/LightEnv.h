///////////////////////////////////////////////////////////////////////////////
//
// Name:		LightEnv.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
// Description: class describing current lighting environment - 
//	at the minute, this is only sunlight and ambient light 
//	parameters; will be extended to handle dynamic lights at some
//  later date
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __LIGHTENV_H
#define __LIGHTENV_H

#include "Color.h"
#include "Vector3D.h"

class CMapWriter;
class CMapReader;
class CEditorData;
class CMainFrame;
class CLightSettingsDlg;

///////////////////////////////////////////////////////////////////////////////
// CLightEnv: description of a lighting environment - contains all the 
// necessary parameters for representation of the lighting within a scenario
class CLightEnv
{
friend class CMapWriter;
friend class CMapReader;
friend class CEditorData;
friend class CMainFrame;
friend class CLightSettingsDlg;
// weird accessor order to preserve memory layout of the class
public:
	RGBColor m_SunColor;
private:
	float m_Elevation;
	float m_Rotation;
public:
	RGBColor m_TerrainAmbientColor;
	RGBColor m_UnitsAmbientColor;
    CVector3D m_SunDir;

	// get sun direction from a rotation and elevation; defined such that:
	//	0 rotation    = (0,0,1)
	// PI/2 rotation  = (-1,0,0)
	//	0 elevation	  = (0,0,0)
	// PI/2 elevation = (0,-1,0)

    float GetElevation() { return m_Elevation; }
    float GetRotation() { return m_Rotation; }

    void SetElevation(float f)
    {
        m_Elevation = f;
        CalculateSunDirection();
    }

    void SetRotation(float f)
    {
        m_Rotation = f;
        CalculateSunDirection();
    }

    void CalculateSunDirection()
    {
        m_SunDir.Y=-float(sin(m_Elevation));
        float scale=1+m_SunDir.Y;
        m_SunDir.X=scale*float(sin(m_Rotation));
        m_SunDir.Z=scale*float(cos(m_Rotation));
        m_SunDir.Normalize();
    }
};

#endif
