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

//Desc:  Contains classes for smooth splines
//Borrowed from Game Programming Gems4.  (Slightly changed to better suit our purposes 
//(and compatability).  Any references to external material can be found there

#ifndef INCLUDED_NUSPLINE
#define INCLUDED_NUSPLINE

#define MAX_SPLINE_NODES 40
#include <stdlib.h>
#include "Vector3D.h"

struct SplineData
{
	CVector3D Position;
	CVector3D Velocity;
	CVector3D Rotation;
	float Distance/*, DistanceOffset*/;	//DistanceOffset is to keep track of how far into the spline this node is
};

class RNSpline
{
public:

	RNSpline() { NodeCount = 0; }
	virtual ~RNSpline() {}

	void AddNode(const CVector3D& pos);
	void BuildSpline();
	CVector3D GetPosition(float time) const;
	CVector3D GetRotation(float time) const;
	const std::vector<SplineData>& GetAllNodes() const { return Node; }

	float MaxDistance;
	int NodeCount;

protected:

	std::vector<SplineData> Node;
	CVector3D GetStartVelocity(int index);
	CVector3D GetEndVelocity(int index);
};

class SNSpline : public RNSpline
{
public:
	virtual ~SNSpline() {}
	void BuildSpline(){ RNSpline::BuildSpline(); Smooth(); Smooth(); Smooth(); }
	void Smooth();
};

class TNSpline : public SNSpline
{
public:
	virtual ~TNSpline() {}

	void AddNode(const CVector3D& pos, const CVector3D& rotation, float timePeriod);
	void PushNode() { Node.push_back( SplineData() ); }
	void InsertNode(const int index, const CVector3D& pos, const CVector3D& rotation, float timePeriod);
	void RemoveNode(const int index);
	void UpdateNodeTime(const int index, float time);
	void UpdateNodePos(const int index, const CVector3D& pos);

	void BuildSpline(){ RNSpline::BuildSpline();  Smooth(); Smooth(); Smooth(); }
	void Smooth(){ for (int x = 0; x < 3; ++x) { SNSpline::Smooth(); Constrain(); } }
	void Constrain();
};

#endif // INCLUDED_NUSPLINE
