/* Copyright (C) 2015 Wildfire Games.
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

#include <string>
#include <sstream>

#include "CinemaManager.h"

#include "CinemaPath.h"
#include "ps/CStr.h"
#include "maths/Vector4D.h"

CCinemaManager::CCinemaManager() : m_DrawCurrentSpline(false), m_Active(true), m_ValidCurrent(false)
{
	m_CurrentPath = m_Paths.end();
}

void CCinemaManager::AddPath(CCinemaPath path, const CStrW& name)
{
	ENSURE( m_Paths.find( name ) == m_Paths.end() );
	m_Paths[name] = path;
}

void CCinemaManager::QueuePath(const CStrW& name, bool queue )
{
	if (!m_PathQueue.empty() && queue == false)
	{
		return;
	}
	else
	{
		ENSURE(HasTrack(name));
		m_PathQueue.push_back(m_Paths[name]);
	}
}

void CCinemaManager::OverridePath(const CStrW& name)
{
	m_PathQueue.clear();
	ENSURE(HasTrack(name));
	m_PathQueue.push_back( m_Paths[name] );
}

void CCinemaManager::SetAllPaths( const std::map<CStrW, CCinemaPath>& paths)
{
	CStrW name;
	m_Paths = paths;
}
void CCinemaManager::SetCurrentPath(const CStrW& name, bool current, bool drawLines)
{
	if ( !HasTrack(name) )
		m_ValidCurrent = false;
	else
		m_ValidCurrent = true;

	m_CurrentPath = m_Paths.find(name);
	m_DrawCurrentSpline = current;
	m_DrawLines = drawLines;
	DrawSpline();
}

bool CCinemaManager::HasTrack(const CStrW& name) const
{ 
	return m_Paths.find(name) != m_Paths.end();
}

void CCinemaManager::DrawSpline() const
{
	if ( !(m_DrawCurrentSpline && m_ValidCurrent) )
		return;
	static const int smoothness = 200;

	m_CurrentPath->second.DrawSpline(CVector4D(0.f, 0.f, 1.f, 1.f), smoothness, m_DrawLines);
}

void CCinemaManager::MoveToPointAt(float time)
{
	ENSURE(m_CurrentPath != m_Paths.end());
	StopPlaying();

	m_CurrentPath->second.m_TimeElapsed = time;
	if ( !m_CurrentPath->second.Validate() )
		return;

	m_CurrentPath->second.MoveToPointAt(m_CurrentPath->second.m_TimeElapsed / 
				m_CurrentPath->second.GetDuration(), m_CurrentPath->second.GetNodeFraction(), 
				m_CurrentPath->second.m_PreviousRotation );
}

bool CCinemaManager::Update(const float deltaRealTime)
{
	if (!m_PathQueue.front().Play(deltaRealTime))
	{
		m_PathQueue.pop_front();
		return false;
	}
	return true;
}
