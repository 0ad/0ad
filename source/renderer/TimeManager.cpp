/* Copyright (C) 2012 Wildfire Games.
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

#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "renderer/TimeManager.h"


CTimeManager::CTimeManager()
{
	m_frameDelta = 0.0;
	m_globalTime = 0.0;
}

double CTimeManager::GetFrameDelta()
{
	return m_frameDelta;
}

double CTimeManager::GetGlobalTime()
{
	return m_globalTime;
}

void CTimeManager::Update(double delta)
{
	m_frameDelta = delta;
	m_globalTime += delta;
}
