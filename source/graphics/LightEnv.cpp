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

#include "precompiled.h"

#include "graphics/LightEnv.h"

#include "maths/MathUtil.h"


CLightEnv::CLightEnv()
	: m_Elevation(DEGTORAD(45)),
	m_Rotation(DEGTORAD(315)),
	m_SunColor(1.5, 1.5, 1.5),
	m_TerrainAmbientColor(0x50/255.f, 0x60/255.f, 0x85/255.f),
	m_UnitsAmbientColor(0x80/255.f, 0x80/255.f, 0x80/255.f),
	m_FogColor(0xCC/255.f, 0xCC/255.f, 0xE5/255.f),
	m_FogFactor(0.000f),
	m_FogMax(0.5f),
	m_Brightness(0.0f), m_Contrast(1.0f), m_Saturation(0.99f), m_Bloom(0.1999f)
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

void CLightEnv::CalculateSunDirection()
{
	m_SunDir.Y = -sinf(m_Elevation);
	float scale = 1 + m_SunDir.Y;
	m_SunDir.X = scale * sinf(m_Rotation);
	m_SunDir.Z = scale * cosf(m_Rotation);
	m_SunDir.Normalize();
}
