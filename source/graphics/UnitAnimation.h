/* Copyright (C) 2009 Wildfire Games.
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

#ifndef INCLUDED_UNITANIMATION
#define INCLUDED_UNITANIMATION

#include "ps/CStr.h"

class CUnit;

class CUnitAnimation
{
	NONCOPYABLE(CUnitAnimation);
public:
	CUnitAnimation(CUnit& unit);

	// (All times are measured in seconds)

	void SetAnimationState(const CStr& name, bool once, float speed, bool keepSelection);
	void SetAnimationSync(float timeUntilActionPos);
	void Update(float time);

private:
	CUnit& m_Unit;
	CStr m_State;
	bool m_Looping;
	float m_Speed;
	float m_OriginalSpeed;
	float m_TimeToNextSync;
};

#endif // INCLUDED_UNITANIMATION
