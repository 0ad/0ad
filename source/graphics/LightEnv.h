/* Copyright (C) 2011 Wildfire Games.
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

/*
 * CLightEnv, a class describing the current lights
 */

#ifndef INCLUDED_LIGHTENV
#define INCLUDED_LIGHTENV

#include "graphics/Color.h"
#include "maths/MathUtil.h"
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
private:
	/**
	 * m_Elevation: Height of sun above the horizon, in radians.
	 * For example, an elevation of M_PI/2 means the sun is straight up.
	 */
	float m_Elevation;

	/**
	 * m_Rotation: Direction of sun on the compass, in radians.
	 * For example, a rotation of zero means the sun is in the direction (0,0,-1)
	 * and a rotation of M_PI/2 means the sun is in the direction (1,0,0) (not taking
	 * elevation into account).
	 */
	float m_Rotation;

	/**
	 * m_TerrainShadowTransparency: Fraction of diffuse light that reaches shadowed terrain.
	 * A value of 0.0 means shadowed polygons get only ambient light, while a value of 1.0
	 * means shadows don't have any effect at all.
	 * TODO: probably delete this, since it's never used and always set to 0.0.
	 */
	float m_TerrainShadowTransparency;

	CVector3D m_SunDir;

	/**
	 * A string that shaders use to determine what lighting model to implement.
	 * Current recognised values are "old" and "standard".
	 */
	std::string m_LightingModel;

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
	const std::string& GetLightingModel() const { return m_LightingModel; }

	void SetElevation(float f);
	void SetRotation(float f);

	void SetTerrainShadowTransparency(float f);

	void SetLightingModel(const std::string& model) { m_LightingModel = model; }

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

	/**
	 * Compute the diffuse sun lighting.
	 * If @p includeSunColor is set, the return value includes the sun color.
	 * (If sun overbrightness is enabled, this might result in clamping).
	 * Otherwise it returns a factor that the sun color should be multiplied by.
	 */
	SColor4ub EvaluateDiffuse(const CVector3D& normal, bool includeSunColor) const
	{
		float dot = -normal.Dot(m_SunDir);

		if (dot <= 0)
			return SColor4ub(0, 0, 0, 255);

		if (includeSunColor)
		{
			return ConvertRGBColorTo4ub(m_SunColor * dot);
		}
		else
		{
			int c = clamp((int)(dot * 255), 0, 255);
			return SColor4ub(c, c, c, 255);
		}
	}

	// Comparison operators
	bool operator==(const CLightEnv& o) const
	{
		return m_Elevation == o.m_Elevation &&
			m_Rotation == o.m_Rotation &&
			m_TerrainShadowTransparency == o.m_TerrainShadowTransparency &&
			m_LightingModel == o.m_LightingModel &&
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
