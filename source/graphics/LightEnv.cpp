/**
 * =========================================================================
 * File        : LightEnv.cpp
 * Project     : Pyrogenesis
 * Description : CLightEnv implementation
 * =========================================================================
 */

#include "precompiled.h"

#include "maths/MathUtil.h"

#include "graphics/LightEnv.h"


CLightEnv::CLightEnv()
	: m_Elevation(DEGTORAD(45)),
	m_Rotation(DEGTORAD(315)),
	m_TerrainShadowTransparency(0.0),
	m_SunColor(1.5, 1.5, 1.5),
	m_TerrainAmbientColor(0x50/255.f, 0x60/255.f, 0x85/255.f),
	m_UnitsAmbientColor(0x80/255.f, 0x80/255.f, 0x80/255.f)
{
	CalculateSunDirection();
}

void CLightEnv::SetElevation(float f)
{
	m_Elevation = f;
	CalculateSunDirection();
}

void CLightEnv::SetRotation(float f)
{
	m_Rotation = f;
	CalculateSunDirection();
}

void CLightEnv::SetTerrainShadowTransparency(float f)
{
	m_TerrainShadowTransparency = f;
}

void CLightEnv::CalculateSunDirection()
{
	m_SunDir.Y=-float(sin(m_Elevation));
	float scale=1+m_SunDir.Y;
	m_SunDir.X=scale*float(sin(m_Rotation));
	m_SunDir.Z=scale*float(cos(m_Rotation));
	m_SunDir.Normalize();
}
