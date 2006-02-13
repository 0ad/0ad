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
friend class CXMLReader;
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

	/**
	 * EvaluateTerrain: Calculate brightness of a point of the terrain with the given normal
	 * vector.
	 * The resulting color contains both ambient and diffuse light.
	 *
	 * @param normal normal vector (must have length 1)
	 * @param color resulting color
	 */
	void EvaluateTerrain(const CVector3D& normal, RGBColor& color) const
	{
		float dot = -normal.Dot(m_SunDir);

		color = m_TerrainAmbientColor;
		if (dot > 0)
			color += m_SunColor * dot;
	}

	/**
	 * EvaluateUnit: Calculate brightness of a point of a unit with the given normal
	 * vector.
	 * The resulting color contains both ambient and diffuse light.
	 *
	 * @param normal normal vector (must have length 1)
	 * @param color resulting color
	 */
	void EvaluateUnit(const CVector3D& normal, RGBColor& color) const
	{
		float dot = -normal.Dot(m_SunDir);

		color = m_UnitsAmbientColor;
		if (dot > 0)
			color += m_SunColor * dot;
	}

	/**
	 * EvaluateDirect: Like EvaluateTerrain and EvaluateUnit, but return only the direct
	 * sunlight term without ambient.
	 *
	 * @param normal normal vector (must have length 1)
	 * @param color resulting color
	 */
	void EvaluateDirect(const CVector3D& normal, RGBColor& color) const
	{
		float dot = -normal.Dot(m_SunDir);

		if (dot > 0)
			color = m_SunColor * dot;
		else
			color = CVector3D(0,0,0);
	}

private:
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
