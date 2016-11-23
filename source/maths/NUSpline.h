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

/**
 * Contains classes for smooth splines
 * Borrowed from Game Programming Gems 4. (Slightly changed to better suit our purposes
 * and compatability. Any references to external material can be found there.
 */

#ifndef INCLUDED_NUSPLINE
#define INCLUDED_NUSPLINE

#define MAX_SPLINE_NODES 128

#include "FixedVector3D.h"
#include "Vector3D.h"

/**
 * Describes a node of the spline
 */
struct SplineData
{
	// Should be fixed, because used in the simulation
	CFixedVector3D Position;
	CVector3D Velocity;
	// TODO: make rotation as other spline
	CFixedVector3D Rotation;
	// Time interval to the previous node, should be 0 for the first node
	fixed Distance;
};


/**
 * Rounded Nonuniform Spline for describing spatial curves or paths with constant speed
 */
class RNSpline
{
public:

	RNSpline();
	virtual ~RNSpline();

	void AddNode(const CFixedVector3D& pos);
	void BuildSpline();
	CVector3D GetPosition(float time) const;
	CVector3D GetRotation(float time) const;
	const std::vector<SplineData>& GetAllNodes() const;

	fixed MaxDistance;
	int NodeCount;

protected:

	std::vector<SplineData> Node;
	CVector3D GetStartVelocity(int index);
	CVector3D GetEndVelocity(int index);
};


/**
 * Smooth Nonuniform Spline for describing paths with smooth acceleration and deceleration,
 * but without turning
 */
class SNSpline : public RNSpline
{
public:
	virtual ~SNSpline();

	void BuildSpline();
	void Smooth();
};


/**
 * Timed Nonuniform Spline for paths with different time intervals between nodes
 */
class TNSpline : public SNSpline
{
public:
	virtual ~TNSpline();

	void AddNode(const CFixedVector3D& pos, const CFixedVector3D& rotation, fixed timePeriod);
	void InsertNode(const int index, const CFixedVector3D& pos, const CFixedVector3D& rotation, fixed timePeriod);
	void RemoveNode(const int index);
	void UpdateNodePos(const int index, const CFixedVector3D& pos);
	void UpdateNodeTime(const int index, fixed time);

	void BuildSpline();
	void Smooth();
	void Constrain();
};

#endif // INCLUDED_NUSPLINE
