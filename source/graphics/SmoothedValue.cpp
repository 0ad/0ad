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

#include <cmath>

#include "graphics/SmoothedValue.h"

CSmoothedValue::CSmoothedValue(float value, float smoothness, float minDelta)
	: m_Target(value),
	  m_Current(value),
	  m_Smoothness(smoothness),
	  m_MinDelta(minDelta)
{
}

float CSmoothedValue::Update(float time)
{
	if (fabs(m_Target - m_Current) < m_MinDelta)
		return 0.0f;

	double p = pow(static_cast<double>(m_Smoothness), 10.0 * static_cast<double>(time));
	// (add the factor of 10 so that smoothnesses don't have to be tiny numbers)

	double delta = (m_Target - m_Current) * (1.0 - p);
	m_Current += delta;
	return static_cast<float>(delta);
}

void CSmoothedValue::Wrap(float min, float max)
{
	double t = fmod(m_Target - min, static_cast<double>(max - min));
	if (t < 0)
		t += max - min;
	t += min;

	m_Current += t - m_Target;
	m_Target = t;
}
