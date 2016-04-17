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

#include "maths/NUSpline.h"
#include "ps/CStr.h"

class CVector3D;
class CVector4D;
class CCamera;

// For loading data
class CCinemaData
{
public:
	CCinemaData() : m_LookAtTarget(false), m_GrowthCount(0), m_Growth(1), m_Switch(1), m_Timescale(fixed::FromInt(1)) {}
	virtual ~CCinemaData() {}
	
	const CCinemaData* GetData() const { return this; }

	CStrW m_Name;
	CStrW m_Orientation;
	CStrW m_Mode;
	CStrW m_Style;

	bool m_LookAtTarget;

	fixed m_Timescale; // a negative timescale results in backwards play

	// Distortion variables
	mutable float m_GrowthCount;
	float m_Growth;
	float m_Switch;
};


// Once the data is part of the path, it shouldn't be changeable, so use private inheritance.
// This class encompasses the spline and the information which determines how the path will operate
// and also provides the functionality for doing so

class CCinemaPath : private CCinemaData, public TNSpline
{
public:
	CCinemaPath() : m_TimeElapsed(0), m_PreviousNodeTime(0) {}
	CCinemaPath(const CCinemaData& data, const TNSpline& spline, const TNSpline& targetSpline);

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

	const CCinemaData* GetData() const;

public:

	void Draw() const;
	void DrawSpline(const RNSpline& spline, const CVector4D& RGBA, int smoothness, bool lines) const;
	void DrawNodes(const RNSpline& spline, const CVector4D& RGBA) const;

	CVector3D GetNodePosition(const int index) const;
	fixed GetNodeDuration(const int index) const;
	fixed GetDuration() const;

	float GetNodeFraction() const;
	float GetElapsedTime() const;

	CStrW GetName() const;

	void SetTimescale(fixed scale);

	float m_TimeElapsed;
	float m_PreviousNodeTime; // How much time has passed before the current node

	size_t m_CurrentNode;
	CVector3D m_PreviousRotation;

public: 

	/**
	 * Returns false if finished.
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	bool Play(const float deltaRealTime);

	/**
	 * Validate the path
	 * @return true if the path is valid
	 */
	bool Validate();

	/**
	 * Returns true if path doesn't contain nodes
	 */
	bool Empty() const;

	/**
	 * Resets the path state
	 */
	void Reset();

	fixed GetTimescale() const;

	const TNSpline& GetTargetSpline() const;

private:

	TNSpline m_TargetSpline;
};

#endif // INCLUDED_CINEMAPATH
