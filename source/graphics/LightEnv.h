/* Copyright (C) 2019 Wildfire Games.
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
public:
	RGBColor m_SunColor;
	RGBColor m_TerrainAmbientColor;
	RGBColor m_UnitsAmbientColor;
	RGBColor m_FogColor;

	float m_FogFactor;
	float m_FogMax;

	float m_Brightness, m_Contrast, m_Saturation, m_Bloom;

	CLightEnv();

	float GetElevation() const { return m_Elevation; }
	float GetRotation() const { return m_Rotation; }
	const CVector3D& GetSunDir() const { return m_SunDir; }

	void SetElevation(float f);
	void SetRotation(float f);

	/**
	 * Calculate brightness of a point of a unit with the given normal vector,
	 * for rendering with CPU lighting.
	 * The resulting color contains both ambient and diffuse light.
	 * To cope with sun overbrightness, the color is scaled by 0.5.
	 *
	 * @param normal normal vector (must have length 1)
	 */
	RGBColor EvaluateUnitScaled(const CVector3D& normal) const
	{
		float dot = -normal.Dot(m_SunDir);

		RGBColor color = m_UnitsAmbientColor;
		if (dot > 0)
			color += m_SunColor * dot;

		return color * 0.5f;
	}

	/**
	 * Compute the diffuse sun lighting color on terrain, for rendering with CPU lighting.
	 * To cope with sun overbrightness, the color is scaled by 0.5.
	 *
	 * @param normal normal vector (must have length 1)
	 */
	SColor4ub EvaluateTerrainDiffuseScaled(const CVector3D& normal) const
	{
		float dot = -normal.Dot(m_SunDir);
		return ConvertRGBColorTo4ub(m_SunColor * dot * 0.5f);
	}

	/**
	 * Compute the diffuse sun lighting factor on terrain, for rendering with shader lighting.
	 *
	 * @param normal normal vector (must have length 1)
	 */
	SColor4ub EvaluateTerrainDiffuseFactor(const CVector3D& normal) const
	{
		float dot = -normal.Dot(m_SunDir);
		u8 c = static_cast<u8>(clamp(dot * 255.f, 0.f, 255.f));
		return SColor4ub(c, c, c, 255);
	}

	// Comparison operators
	bool operator==(const CLightEnv& o) const
	{
		return m_Elevation == o.m_Elevation &&
			m_Rotation == o.m_Rotation &&
			m_SunColor == o.m_SunColor &&
			m_TerrainAmbientColor == o.m_TerrainAmbientColor &&
			m_UnitsAmbientColor == o.m_UnitsAmbientColor &&
			m_FogColor == o.m_FogColor &&
			m_FogFactor == o.m_FogFactor &&
			m_FogMax == o.m_FogMax &&
			m_Brightness == o.m_Brightness &&
			m_Contrast == o.m_Contrast &&
			m_Saturation == o.m_Saturation &&
			m_Bloom == o.m_Bloom;
	}

	bool operator!=(const CLightEnv& o) const
	{
		return !(*this == o);
	}

private:
	friend class CMapWriter;
	friend class CMapReader;
	friend class CXMLReader;

	/**
	* Height of sun above the horizon, in radians.
	* For example, an elevation of M_PI/2 means the sun is straight up.
	*/
	float m_Elevation;

	/**
	* Direction of sun on the compass, in radians.
	* For example, a rotation of zero means the sun is in the direction (0,0,-1)
	* and a rotation of M_PI/2 means the sun is in the direction (1,0,0) (not taking
	* elevation into account).
	*/
	float m_Rotation;

	/**
	* Vector corresponding to m_Elevation and m_Rotation.
	* Updated by CalculateSunDirection.
	*/
	CVector3D m_SunDir;

	void CalculateSunDirection();
};

#endif // INCLUDED_LIGHTENV
