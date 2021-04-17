/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_SMOOTHEDVALUE
#define INCLUDED_SMOOTHEDVALUE

#include "maths/MathUtil.h"

/**
* A value with exponential decay towards the target value.
*/
class CSmoothedValue
{
public:
	CSmoothedValue(float value, float smoothness, float minDelta);

	float GetSmoothedValue() const
	{
		return m_Current;
	}

	void SetValueSmoothly(float value)
	{
		m_Target = value;
	}

	void AddSmoothly(float value)
	{
		m_Target += value;
	}

	void Add(float value)
	{
		m_Target += value;
		m_Current += value;
	}

	float GetValue() const
	{
		return m_Target;
	}

	void SetValue(float value)
	{
		m_Target = value;
		m_Current = value;
	}

	float GetSmoothness() const
	{
		return m_Smoothness;
	}

	void SetSmoothness(float smoothness)
	{
		m_Smoothness = smoothness;
	}

	void ClampSmoothly(float min, float max)
	{
		m_Target = Clamp<double>(m_Target, min, max);
	}

	float Update(float time);

	// Wrap so 'target' is in the range [min, max]
	void Wrap(float min, float max);

private:
	float m_Smoothness;

	double m_Target; // the value which m_Current is tending towards
	double m_Current;
	// (We use double because the extra precision is worthwhile here)

	float m_MinDelta; // cutoff where we stop moving (to avoid ugly shimmering effects)
};

#endif // INCLUDED_SMOOTHEDVALUE
