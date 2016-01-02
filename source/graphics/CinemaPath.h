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


#ifndef INCLUDED_CINEMAPATH
#define INCLUDED_CINEMAPATH

#include <list>
#include <map>
#include "ps/CStr.h"
#include "maths/NUSpline.h"

/*
	desc: contains various functions used for cinematic camera paths
	See also: CinemaHandler.cpp, Cinematic.h/.cpp
*/

class CVector3D;
class CVector4D;
class CCamera;

// For loading data
class CCinemaData
{
public:
	CCinemaData() : m_GrowthCount(0), m_Growth(0), m_Switch(0), 
					m_Mode(0), m_Style(0), m_Timescale(1) {}
	virtual ~CCinemaData() {}
	
	const CCinemaData* GetData() const { return this; }
	
	// Distortion variables
	mutable float m_GrowthCount;
	float m_Growth;
	float m_Switch;
	int m_Mode;
	int m_Style;
	float m_Timescale; // a negative timescale results in backwards play

};


// Once the data is part of the path, it shouldn't be changeable, so use private inheritance.
// This class encompasses the spline and the information which determines how the path will operate
// and also provides the functionality for doing so

class CCinemaPath : private CCinemaData, public TNSpline
{
public:
	CCinemaPath() { m_TimeElapsed = 0.0f; m_PreviousNodeTime = 0.0f; }
	CCinemaPath(const CCinemaData& data, const TNSpline& spline);
	~CCinemaPath() { DistStylePtr = NULL;  DistModePtr = NULL; }
	
	enum { EM_IN, EM_OUT, EM_INOUT, EM_OUTIN };
	enum { ES_DEFAULT, ES_GROWTH, ES_EXPO, ES_CIRCLE, ES_SINE };
	
	// Sets camera position to calculated point on spline
	void MoveToPointAt(float t, float nodet, const CVector3D&);
	
	// Distortion mode functions-change how ratio is passed to distortion style functions
	float EaseIn(float t) const;
	float EaseOut(float t) const;
	float EaseInOut(float t) const;
	float EaseOutIn(float t) const;

	// Distortion style functions
	float EaseDefault(float t) const;
	float EaseGrowth(float t) const;
	float EaseExpo(float t) const;
	float EaseCircle(float t) const;
	float EaseSine(float t) const;

	float (CCinemaPath::*DistStylePtr)(float ratio) const;
	float (CCinemaPath::*DistModePtr)(float ratio) const;

	const CCinemaData* GetData() const { return CCinemaData::GetData(); }

public:

	void DrawSpline(const CVector4D& RGBA, int smoothness, bool lines) const;

	inline CVector3D GetNodePosition(const int index) const { return Node[index].Position; }
	inline float GetNodeDuration(const int index) const { return Node[index].Distance; }
	inline float GetDuration() const { return MaxDistance; }

	inline float GetNodeFraction() const { return (m_TimeElapsed - m_PreviousNodeTime) / Node[m_CurrentNode].Distance; }
	inline float GetElapsedTime() const { return m_TimeElapsed; }

	inline void SetTimescale(float scale) { m_Timescale = scale; }

	float m_TimeElapsed;
	float m_PreviousNodeTime; // How much time has passed before the current node

	size_t m_CurrentNode;
	CVector3D m_PreviousRotation;

public: 

	/**
	 * Returns false if finished.
	 * 
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	bool Play(const float deltaRealTime);
	bool Validate();

	inline float GetTimescale() const { return m_Timescale; }	
};

#endif
