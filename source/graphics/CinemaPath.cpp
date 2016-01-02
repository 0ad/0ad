/* Copyright (C) 2016 Wildfire Games.
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

#include "CinemaPath.h"

#include <sstream>
#include <string>

#include "Camera.h"
#include "GameView.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CStr.h"
#include "ps/Game.h"


CCinemaPath::CCinemaPath(const CCinemaData& data, const TNSpline& spline)
	: CCinemaData(data), TNSpline(spline), m_TimeElapsed(0.f)
{ 
	m_TimeElapsed = 0;
	BuildSpline();

	// Set distortion mode and style
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
}

void CCinemaPath::DrawSpline(const CVector4D& RGBA, int smoothness, bool lines) const
{
	if (NodeCount < 2 || DistModePtr == NULL)
		return;
	if (NodeCount == 2 && lines)
		smoothness = 2;

	float start = MaxDistance / smoothness;
	float time = 0;

#if CONFIG2_GLES
	#warning TODO: do something about CCinemaPath on GLES
#else

	glColor4f(RGBA.X, RGBA.Y, RGBA.Z, RGBA.W);
	if (lines)
	{
		glLineWidth(1.8f);
		glEnable(GL_LINE_SMOOTH);
		glBegin(GL_LINE_STRIP);

		for (int i = 0; i <= smoothness; ++i)
		{
			// Find distorted time
			time = start*i / MaxDistance;
			CVector3D tmp = GetPosition(time);
			glVertex3f(tmp.X, tmp.Y, tmp.Z);
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

		for (int i = 0; i <= smoothness; ++i)
		{
			// Find distorted time
			time = (this->*DistModePtr)(start*i / MaxDistance);
			CVector3D tmp = GetPosition(time);
			glVertex3f(tmp.X, tmp.Y, tmp.Z);
		}
		glColor3f(1.0f, 1.0f, 0.0f); // yellow

		for (size_t i = 0; i < Node.size(); ++i)
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

// Distortion mode functions
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

// Distortion style functions
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
	if (t == 0)
		return t;
	return powf(m_Growth, 10*(t-1.0f));
}

float CCinemaPath::EaseCircle(float t) const
{
	t = -(sqrt(1.0f - t*t) - 1.0f);
	if (m_GrowthCount > 1.0f)
	{
		--m_GrowthCount;
		return (this->*DistStylePtr)(t);
	}
	return t;
}

float CCinemaPath::EaseSine(float t) const
{
	t = 1.0f - cos(t * (float)M_PI/2);
	if (m_GrowthCount > 1.0f)
	{
		--m_GrowthCount;
		return (this->*DistStylePtr)(t);
	}
	return t;
}

bool CCinemaPath::Validate()
{
	if (m_TimeElapsed > GetDuration() || m_TimeElapsed < 0.0f)
		return false;

	// Find current node and past "node time"
	float previousTime = 0.0f, cumulation = 0.0f;

	// Ignore the last node, since it is a blank (node time values are shifted down one from interface)
	for (size_t i = 0; i < Node.size() - 1; ++i)
	{
		cumulation += Node[i].Distance;
		if (m_TimeElapsed <= cumulation)
		{
			m_PreviousNodeTime = previousTime;
			m_PreviousRotation = Node[i].Rotation;
			m_CurrentNode = i; // We're moving toward this next node, so use its rotation
			return true;
		}
		previousTime += Node[i].Distance;
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

	MoveToPointAt(m_TimeElapsed / GetDuration(), GetNodeFraction(), m_PreviousRotation);
	return true;
}