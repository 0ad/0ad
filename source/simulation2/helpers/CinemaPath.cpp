/* Copyright (C) 2017 Wildfire Games.
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

#include "graphics/Camera.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"

CCinemaPath::CCinemaPath(const CCinemaData& data, const TNSpline& spline, const TNSpline& targetSpline)
	: CCinemaData(data), TNSpline(spline), m_TargetSpline(targetSpline), m_TimeElapsed(0.f)
{
	// Calculate curves by nodes
	BuildSpline();
	m_TargetSpline.BuildSpline();

	if (m_Orientation == L"target")
	{
		m_LookAtTarget = true;
		ENSURE(!m_TargetSpline.GetAllNodes().empty());
	}

	// Set distortion mode and style
	if (data.m_Mode == L"ease_in")
		DistModePtr = &CCinemaPath::EaseIn;
	else if (data.m_Mode == L"ease_out")
		DistModePtr = &CCinemaPath::EaseOut;
	else if (data.m_Mode == L"ease_inout")
		DistModePtr = &CCinemaPath::EaseInOut;
	else if (data.m_Mode == L"ease_outin")
		DistModePtr = &CCinemaPath::EaseOutIn;
	else
	{
		LOGWARNING("Cinematic mode not found for '%s'", data.m_Mode.ToUTF8().c_str());
		DistModePtr = &CCinemaPath::EaseInOut;
	}

	if (data.m_Style == L"default")
		DistStylePtr = &CCinemaPath::EaseDefault;
	else if (data.m_Style == L"growth")
		DistStylePtr = &CCinemaPath::EaseGrowth;
	else if (data.m_Style == L"expo")
		DistStylePtr = &CCinemaPath::EaseExpo;
	else if (data.m_Style == L"circle")
		DistStylePtr = &CCinemaPath::EaseCircle;
	else if (data.m_Style == L"sine")
		DistStylePtr = &CCinemaPath::EaseSine;
	else
	{
		LOGWARNING("Cinematic style not found for '%s'", data.m_Style.ToUTF8().c_str());
		DistStylePtr = &CCinemaPath::EaseDefault;
	}
}

CVector3D CCinemaPath::GetNodePosition(const int index) const
{
	return Node[index].Position;
}

fixed CCinemaPath::GetNodeDuration(const int index) const
{
	return Node[index].Distance;
}

fixed CCinemaPath::GetDuration() const
{
	return MaxDistance;
}

float CCinemaPath::GetNodeFraction() const
{
	return (m_TimeElapsed - m_PreviousNodeTime) / Node[m_CurrentNode].Distance.ToFloat();
}

float CCinemaPath::GetElapsedTime() const
{
	return m_TimeElapsed;
}

const CStrW& CCinemaPath::GetName() const
{
	return m_Name;
}

void CCinemaPath::SetTimescale(fixed scale)
{
	m_Timescale = scale;
}

void CCinemaPath::MoveToPointAt(float t, float nodet, const CVector3D& startRotation, CCamera* camera) const
{
	t = (this->*DistModePtr)(t);

	CVector3D pos = GetPosition(t);

	if (m_LookAtTarget)
	{
		if (m_TimeElapsed <= m_TargetSpline.MaxDistance.ToFloat())
			camera->LookAt(pos, m_TargetSpline.GetPosition(m_TimeElapsed / m_TargetSpline.MaxDistance.ToFloat()), CVector3D(0, 1, 0));
		else
			camera->LookAt(pos, m_TargetSpline.GetAllNodes().back().Position, CVector3D(0, 1, 0));
	}
	else
	{
		CVector3D nodeRotation = Node[m_CurrentNode + 1].Rotation;
		CQuaternion start, end;
		start.FromEulerAngles(DEGTORAD(startRotation.X), DEGTORAD(startRotation.Y), DEGTORAD(startRotation.Z));
		end.FromEulerAngles(DEGTORAD(nodeRotation.X), DEGTORAD(nodeRotation.Y), DEGTORAD(nodeRotation.Z));
		start.Slerp(start, end, nodet);

		camera->m_Orientation.SetIdentity();
		camera->m_Orientation.Rotate(start);
		camera->m_Orientation.Translate(pos);
	}
	camera->UpdateFrustum();
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

const CCinemaData* CCinemaPath::GetData() const
{
	return CCinemaData::GetData();
}

bool CCinemaPath::Validate()
{
	if (m_TimeElapsed > GetDuration().ToFloat() || m_TimeElapsed < 0.0f)
		return false;

	// Find current node and past "node time"
	float previousTime = 0.0f, cumulation = 0.0f;

	// Ignore the last node, since it is a blank (node time values are shifted down one from interface)
	for (size_t i = 0; i < Node.size() - 1; ++i)
	{
		cumulation += Node[i].Distance.ToFloat();
		if (m_TimeElapsed <= cumulation)
		{
			m_PreviousNodeTime = previousTime;
			m_PreviousRotation = Node[i].Rotation;
			m_CurrentNode = i; // We're moving toward this next node, so use its rotation
			return true;
		}
		previousTime += Node[i].Distance.ToFloat();
	}
	debug_warn("validation of cinema path is wrong\n");
	return false;
}

bool CCinemaPath::Play(const float deltaRealTime, CCamera* camera)
{
	m_TimeElapsed += m_Timescale.ToFloat() * deltaRealTime;
	if (!Validate())
		return false;

	MoveToPointAt(m_TimeElapsed / GetDuration().ToFloat(), GetNodeFraction(), m_PreviousRotation, camera);
	return true;
}

bool CCinemaPath::Empty() const
{
	return Node.empty();
}

void CCinemaPath::Reset()
{
	m_TimeElapsed = 0.0f;
}

fixed CCinemaPath::GetTimescale() const
{
	return m_Timescale;
}

const TNSpline& CCinemaPath::GetTargetSpline() const
{
	return m_TargetSpline;
}
