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
#include "lib/ogl.h"
#include "CinemaTrack.h"
#include "ps/Game.h"
#include "GameView.h"
#include "maths/MathUtil.h"
#include "Camera.h"
#include "ps/CStr.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "maths/Quaternion.h"

CCinemaPath::CCinemaPath(const CCinemaData& data, const TNSpline& spline)
	: CCinemaData(data), TNSpline(spline), m_TimeElapsed(0.f)
{ 
	m_TimeElapsed = 0;
	BuildSpline();

	//Set distortion mode and style
	switch(data.m_Mode)
	{
	case CCinemaPath::EM_IN:
		DistModePtr = &CCinemaPath::EaseIn;
		break;
	case CCinemaPath::EM_OUT:
		DistModePtr = &CCinemaPath::EaseOut;
		break;
	case CCinemaPath::EM_INOUT:
		DistModePtr = &CCinemaPath::EaseInOut;
		break;
	case CCinemaPath::EM_OUTIN:
		DistModePtr = &CCinemaPath::EaseOutIn;
		break;
	default:
		debug_printf("Cinematic mode not found for %d\n", data.m_Mode);
		break;
	}

	switch (data.m_Style)
	{
	case CCinemaPath::ES_DEFAULT:
		DistStylePtr = &CCinemaPath::EaseDefault;
		break;
	case CCinemaPath::ES_GROWTH:
		DistStylePtr = &CCinemaPath::EaseGrowth;
		break;
	case CCinemaPath::ES_EXPO:
		DistStylePtr = &CCinemaPath::EaseExpo;
		break;
	case CCinemaPath::ES_CIRCLE:
		DistStylePtr = &CCinemaPath::EaseCircle;
		break;
	case CCinemaPath::ES_SINE:
		DistStylePtr = &CCinemaPath::EaseSine;
		break;
	default:
		debug_printf("Cinematic mode not found for %d\n", data.m_Style);
		break;
	}
	//UpdateDuration();

}

void CCinemaPath::DrawSpline(const CVector4D& RGBA, int smoothness, bool lines) const
{
	if (NodeCount < 2 || DistModePtr == NULL)
		return;
	if ( NodeCount == 2 && lines )
		smoothness = 2;

	float start = MaxDistance / smoothness;
	float time=0;
	
#if CONFIG2_GLES
#warning TODO: do something about CCinemaPath on GLES
#else

	glColor4f( RGBA.X, RGBA.Y, RGBA.Z, RGBA.W );
	if ( lines )
	{	
		glLineWidth(1.8f);
		glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINE_STRIP);

		for (int i=0; i<=smoothness; ++i)
		{
			//Find distorted time
			time = start*i / MaxDistance;
			CVector3D tmp = GetPosition(time);
			glVertex3f( tmp.X, tmp.Y, tmp.Z );
		}
		glEnd();
		glDisable(GL_LINE_SMOOTH);
		glLineWidth(1.0f);
	}
	else
	{
		smoothness /= 2;
		start = MaxDistance / smoothness;
		glEnable(GL_POINT_SMOOTH);
		glPointSize(3.0f);
		glBegin(GL_POINTS);

		for (int i=0; i<=smoothness; ++i)
		{
			//Find distorted time
			time = (this->*DistModePtr)(start*i / MaxDistance);
			CVector3D tmp = GetPosition(time);
			glVertex3f( tmp.X, tmp.Y, tmp.Z );
		}
		glColor3f(1.0f, 1.0f, 0.0f);	//yellow

		for ( size_t i=0; i<Node.size(); ++i )
			glVertex3f(Node[i].Position.X, Node[i].Position.Y, Node[i].Position.Z);

		glEnd();
		glPointSize(1.0f);
		glDisable(GL_POINT_SMOOTH);
	}

#endif
}

void CCinemaPath::MoveToPointAt(float t, float nodet, const CVector3D& startRotation)
{
	CCamera *Cam = g_Game->GetView()->GetCamera();
	t = (this->*DistModePtr)(t);
	
	CVector3D nodeRotation = Node[m_CurrentNode + 1].Rotation;
	CQuaternion start, end;
	start.FromEulerAngles(DEGTORAD(startRotation.X), DEGTORAD(startRotation.Y), DEGTORAD(startRotation.Z));
	end.FromEulerAngles(DEGTORAD(nodeRotation.X), DEGTORAD(nodeRotation.Y), DEGTORAD(nodeRotation.Z));
	start.Slerp(start, end, nodet);
	CVector3D pos = GetPosition(t);
	CQuaternion quat;
	
	Cam->m_Orientation.SetIdentity();
	Cam->m_Orientation.Rotate(start);
	Cam->m_Orientation.Translate(pos);
	Cam->UpdateFrustum();
}

//Distortion mode functions
float CCinemaPath::EaseIn(float t) const
{
	return (this->*DistStylePtr)(t);
}
float CCinemaPath::EaseOut(float t) const
{
	return 1.0f - EaseIn(1.0f-t);
}
float CCinemaPath::EaseInOut(float t) const
{
	if (t < m_Switch)
		return EaseIn(1.0f/m_Switch * t) * m_Switch;
	return EaseOut(1.0f/m_Switch * (t-m_Switch)) * m_Switch + m_Switch;
}
float CCinemaPath::EaseOutIn(float t) const
{
	if (t < m_Switch)
		return EaseOut(1.0f/m_Switch * t) * m_Switch;
	return EaseIn(1.0f/m_Switch * (t-m_Switch)) * m_Switch + m_Switch;
}


//Distortion style functions
float CCinemaPath::EaseDefault(float t) const
{
	return t;
}
float CCinemaPath::EaseGrowth(float t) const
{
	return pow(t, m_Growth);
}

float CCinemaPath::EaseExpo(float t) const
{
	if(t == 0)
		return t;
    return powf(m_Growth, 10*(t-1.0f));
}
float CCinemaPath::EaseCircle(float t) const
{
	 t = -(sqrt(1.0f - t*t) - 1.0f);
     if(m_GrowthCount > 1.0f)
	 {
		 m_GrowthCount--;
		 return (this->*DistStylePtr)(t);
	 }
	 return t;
}

float CCinemaPath::EaseSine(float t) const
{
     t = 1.0f - cos(t * (float)M_PI/2);
     if(m_GrowthCount > 1.0f)
	 {
		 m_GrowthCount--;
		 return (this->*DistStylePtr)(t);
	 }
	 return t;
}

bool CCinemaPath::Validate()
{
	if ( m_TimeElapsed <= GetDuration() && m_TimeElapsed >= 0.0f )
	{
		//Find current node and past "node time"
		float previousTime = 0.0f, cumulation = 0.0f;
		//Ignore the last node, since it is a blank (node time values are shifted down one from interface)
		for ( size_t i = 0; i < Node.size() - 1; ++i )
		{
			cumulation += Node[i].Distance;
			if ( m_TimeElapsed <= cumulation )
			{
				m_PreviousNodeTime = previousTime;
				m_PreviousRotation = Node[i].Rotation;
				m_CurrentNode = i;	//We're moving toward this next node, so use its rotation
				return true;
			}
			else
				previousTime += Node[i].Distance;
		}
	}
	return false;
}

bool CCinemaPath::Play(const float deltaRealTime)
{
	m_TimeElapsed += m_Timescale * deltaRealTime;

	if (!Validate())
	{
		m_TimeElapsed = 0.0f;
		return false;
	}
	
	MoveToPointAt( m_TimeElapsed / GetDuration(), GetNodeFraction(), m_PreviousRotation );
	return true;
}


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
