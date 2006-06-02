/**
 * =========================================================================
 * File        : LightEnv.h
 * Project     : Pyrogenesis
 * Description : CLightEnv, a class describing the current lights
 *
 * @author Rich Cross <rich@wildfiregames.com>
 * @author Nicolai Hähnle <nicolai@wildfiregames.com>
 * =========================================================================
 */

#ifndef __LIGHTENV_H
#define __LIGHTENV_H

#include "Color.h"
#include "maths/Vector3D.h"

class CMapWriter;
class CMapReader;

/**
 * Class CLightEnv: description of a lighting environment - contains all the
 * necessary parameters for representation of the lighting within a scenario
 */
class CLightEnv
{
friend class CMapWriter;
friend class CMapReader;
friend class CXMLReader;
/* Trying to compile ScEd? ;)
friend class CEditorData;
friend class CMainFrame;
friend class CLightSettingsDlg;
*/
private:
	/**
	 * m_Elevation: Height of sun above the horizon, in radians.
	 * For example, an elevation of PI/2 means the sun is straight up.
	 */
	float m_Elevation;

	/**
	 * m_Rotation: Direction of sun on the compass, in radians.
	 * For example, a rotation of zero means the sun is in the direction (0,0,-1)
	 * and a rotation of PI/2 means the sun is in the direction (1,0,0) (not taking
	 * elevation into account).
	 */
	float m_Rotation;

	/**
	 * m_TerrainShadowTransparency: Fraction of diffuse light that reaches shadowed terrain.
	 * A value of 0.0 means shadowed polygons get only ambient light, while a value of 1.0
	 * means shadows don't have any effect at all.
	 */
	float m_TerrainShadowTransparency;

	CVector3D m_SunDir;

public:
	RGBColor m_SunColor;
	RGBColor m_TerrainAmbientColor;
	RGBColor m_UnitsAmbientColor;

public:
	CLightEnv();

	float GetElevation() const { return m_Elevation; }
	float GetRotation() const { return m_Rotation; }
	const CVector3D& GetSunDir() const { return m_SunDir; }
	float GetTerrainShadowTransparency() const { return m_TerrainShadowTransparency; }

	void SetElevation(float f);
	void SetRotation(float f);

	void SetTerrainShadowTransparency(float f);

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

	// Comparison operators
	bool operator==(const CLightEnv& o) const
	{
		return m_Elevation == o.m_Elevation &&
			m_Rotation == o.m_Rotation &&
			m_TerrainShadowTransparency == o.m_TerrainShadowTransparency &&
			m_SunColor == o.m_SunColor &&
			m_TerrainAmbientColor == o.m_TerrainAmbientColor &&
			m_UnitsAmbientColor == o.m_UnitsAmbientColor;
	}

	bool operator!=(const CLightEnv& o) const
	{
		return !(*this == o);
	}

private:
	void CalculateSunDirection();
};

#endif
