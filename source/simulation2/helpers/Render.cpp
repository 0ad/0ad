/* Copyright (C) 2010 Wildfire Games.
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

#include "Render.h"

#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/BoundingBoxOriented.h"
#include "maths/MathUtil.h"
#include "maths/Vector2D.h"
#include "ps/Profile.h"
#include "maths/Quaternion.h"

void SimRender::ConstructLineOnGround(const CSimContext& context, const std::vector<float>& xz,
		SOverlayLine& overlay, bool floating, float heightOffset)
{
	PROFILE("ConstructLineOnGround");

	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	if (xz.size() < 2)
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterMan(context, SYSTEM_ENTITY);
		if (!cmpWaterMan.null())
			water = cmpWaterMan->GetExactWaterLevel(xz[0], xz[1]);
	}

	overlay.m_Coords.reserve(xz.size()/2 * 3);

	for (size_t i = 0; i < xz.size(); i += 2)
	{
		float px = xz[i];
		float pz = xz[i+1];
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + heightOffset;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

void SimRender::ConstructCircleOnGround(const CSimContext& context, float x, float z, float radius,
		SOverlayLine& overlay, bool floating, float heightOffset)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterMan(context, SYSTEM_ENTITY);
		if (!cmpWaterMan.null())
			water = cmpWaterMan->GetExactWaterLevel(x, z);
	}

	// Adapt the circle resolution to look reasonable for small and largeish radiuses
	size_t numPoints = clamp((size_t)(radius*4.0f), (size_t)12, (size_t)48);

	overlay.m_Coords.reserve((numPoints + 1) * 3);

	for (size_t i = 0; i <= numPoints; ++i) // use '<=' so it's a closed loop
	{
		float a = (float)i * 2 * (float)M_PI / (float)numPoints;
		float px = x + radius * sinf(a);
		float pz = z + radius * cosf(a);
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + heightOffset;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

// This method splits up a straight line into a number of line segments each having a length ~= CELL_SIZE
static void SplitLine(std::vector<std::pair<float, float> >& coords, float x1, float y1, float x2, float y2)
{
	float length = sqrtf(SQR(x1 - x2) + SQR(y1 - y2));
	size_t pieces = ((int)length) / CELL_SIZE;
	if (pieces > 0)
	{
		float xPieceLength = (x1 - x2) / (float)pieces;
		float yPieceLength = (y1 - y2) / (float)pieces;
		for (size_t i = 1; i <= (pieces - 1); ++i)
		{
			coords.push_back(std::make_pair(x1 - (xPieceLength * (float)i), y1 - (yPieceLength * (float)i)));
		}
	}
	coords.push_back(std::make_pair(x2, y2));
}

void SimRender::ConstructSquareOnGround(const CSimContext& context, float x, float z, float w, float h, float a,
		SOverlayLine& overlay, bool floating, float heightOffset)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (cmpTerrain.null())
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterMan(context, SYSTEM_ENTITY);
		if (!cmpWaterMan.null())
			water = cmpWaterMan->GetExactWaterLevel(x, z);
	}

	float c = cosf(a);
	float s = sinf(a);

	std::vector<std::pair<float, float> > coords;

	// Add the first vertex, since SplitLine will be adding only the second end-point of the each line to
	// the coordinates list. We don't have to worry about the other lines, since the end-point of one line
	// will be the starting point of the next
	coords.push_back(std::make_pair(x - w/2*c + h/2*s, z + w/2*s + h/2*c));

	SplitLine(coords, x - w/2*c + h/2*s, z + w/2*s + h/2*c, x - w/2*c - h/2*s, z + w/2*s - h/2*c);
	SplitLine(coords, x - w/2*c - h/2*s, z + w/2*s - h/2*c, x + w/2*c - h/2*s, z - w/2*s - h/2*c);
	SplitLine(coords, x + w/2*c - h/2*s, z - w/2*s - h/2*c, x + w/2*c + h/2*s, z - w/2*s + h/2*c);
	SplitLine(coords, x + w/2*c + h/2*s, z - w/2*s + h/2*c, x - w/2*c + h/2*s, z + w/2*s + h/2*c);

	overlay.m_Coords.reserve(coords.size() * 3);

	for (size_t i = 0; i < coords.size(); ++i)
	{
		float px = coords[i].first;
		float pz = coords[i].second;
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + heightOffset;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}
}

void SimRender::ConstructBoxOutline(const CBoundingBoxAligned& bound, SOverlayLine& overlayLine)
{
	overlayLine.m_Coords.clear(); 

	if (bound.IsEmpty())
		return;

	const CVector3D& pMin = bound[0];
	const CVector3D& pMax = bound[1];
	
	// floor square 
	overlayLine.PushCoords(pMin.X, pMin.Y, pMin.Z);
	overlayLine.PushCoords(pMax.X, pMin.Y, pMin.Z);
	overlayLine.PushCoords(pMax.X, pMin.Y, pMax.Z);
	overlayLine.PushCoords(pMin.X, pMin.Y, pMax.Z);
	overlayLine.PushCoords(pMin.X, pMin.Y, pMin.Z);
	// roof square 
	overlayLine.PushCoords(pMin.X, pMax.Y, pMin.Z);
	overlayLine.PushCoords(pMax.X, pMax.Y, pMin.Z);
	overlayLine.PushCoords(pMax.X, pMax.Y, pMax.Z);
	overlayLine.PushCoords(pMin.X, pMax.Y, pMax.Z);
	overlayLine.PushCoords(pMin.X, pMax.Y, pMin.Z);
}

void SimRender::ConstructBoxOutline(const CBoundingBoxOriented& box, SOverlayLine& overlayLine)
{
	overlayLine.m_Coords.clear();

	if (box.IsEmpty())
		return;

	CVector3D corners[8];
	box.GetCorner(-1, -1, -1, corners[0]);
	box.GetCorner( 1, -1, -1, corners[1]);
	box.GetCorner( 1, -1,  1, corners[2]);
	box.GetCorner(-1, -1,  1, corners[3]);
	box.GetCorner(-1,  1, -1, corners[4]);
	box.GetCorner( 1,  1, -1, corners[5]);
	box.GetCorner( 1,  1,  1, corners[6]);
	box.GetCorner(-1,  1,  1, corners[7]);

	overlayLine.PushCoords(corners[0]);
	overlayLine.PushCoords(corners[1]);
	overlayLine.PushCoords(corners[2]);
	overlayLine.PushCoords(corners[3]);
	overlayLine.PushCoords(corners[0]);

	overlayLine.PushCoords(corners[4]);
	overlayLine.PushCoords(corners[5]);
	overlayLine.PushCoords(corners[6]);
	overlayLine.PushCoords(corners[7]);
	overlayLine.PushCoords(corners[4]);
}

void SimRender::ConstructGimbal(const CVector3D& center, float radius, SOverlayLine& out, size_t numSteps)
{
	ENSURE(numSteps > 0 && numSteps % 4 == 0); // must be a positive multiple of 4
	
	out.m_Coords.clear();

	size_t fullCircleSteps = numSteps;
	const float angleIncrement = 2.f*M_PI/fullCircleSteps;

	const CVector3D X_UNIT(1, 0, 0);
	const CVector3D Y_UNIT(0, 1, 0);
	const CVector3D Z_UNIT(0, 0, 1);
	CVector3D rotationVector(0, 0, radius); // directional vector based in the center that we will be rotating to get the gimbal points

	// first draw a quarter of XZ gimbal; then complete the XY gimbal; then continue the XZ gimbal and finally add the YZ gimbal
	// (that way we can keep a single continuous line)
	
	// -- XZ GIMBAL (PART 1/2) -----------------------------------------------

	CQuaternion xzRotation;
	xzRotation.FromAxisAngle(Y_UNIT, angleIncrement);

	for (size_t i = 0; i < fullCircleSteps/4; ++i) // complete only a quarter of the way
	{
		out.PushCoords(center + rotationVector);
		rotationVector = xzRotation.Rotate(rotationVector);
	}

	// -- XY GIMBAL ----------------------------------------------------------

	// now complete the XY gimbal while the XZ gimbal is interrupted
	CQuaternion xyRotation;
	xyRotation.FromAxisAngle(Z_UNIT, angleIncrement);

	for (size_t i = 0; i < fullCircleSteps; ++i) // note the <; the last point of the XY gimbal isn't added, because the XZ gimbal will add it
	{
		out.PushCoords(center + rotationVector);
		rotationVector = xyRotation.Rotate(rotationVector);
	}

	// -- XZ GIMBAL (PART 2/2) -----------------------------------------------

	// resume the XZ gimbal to completion
	for (size_t i = fullCircleSteps/4; i < fullCircleSteps; ++i) // exclude the last point of the circle so the YZ gimbal can add it
	{
		out.PushCoords(center + rotationVector);
		rotationVector = xzRotation.Rotate(rotationVector);
	}

	// -- YZ GIMBAL ----------------------------------------------------------

	CQuaternion yzRotation;
	yzRotation.FromAxisAngle(X_UNIT, angleIncrement);

	for (size_t i = 0; i <= fullCircleSteps; ++i)
	{
		out.PushCoords(center + rotationVector);
		rotationVector = yzRotation.Rotate(rotationVector);
	}
}

void SimRender::SmoothPointsAverage(std::vector<CVector2D>& points, bool closed)
{
	PROFILE("SmoothPointsAverage");

	size_t n = points.size();
	if (n < 2)
		return; // avoid out-of-bounds array accesses, and leave the points unchanged

	std::vector<CVector2D> newPoints;
	newPoints.resize(points.size());

	// Handle the end points appropriately
	if (closed)
	{
		newPoints[0] = (points[n-1] + points[0] + points[1]) / 3.f;
		newPoints[n-1] = (points[n-2] + points[n-1] + points[0]) / 3.f;
	}
	else
	{
		newPoints[0] = points[0];
		newPoints[n-1] = points[n-1];
	}

	// Average all the intermediate points
	for (size_t i = 1; i < n-1; ++i)
		newPoints[i] = (points[i-1] + points[i] + points[i+1]) / 3.f;

	points.swap(newPoints);
}

static CVector2D EvaluateSpline(float t, CVector2D a0, CVector2D a1, CVector2D a2, CVector2D a3, float offset)
{
	// Compute position on spline
	CVector2D p = a0*(t*t*t) + a1*(t*t) + a2*t + a3;

	// Compute unit-vector direction of spline
	CVector2D dp = (a0*(3*t*t) + a1*(2*t) + a2).Normalized();

	// Offset position perpendicularly
	return p + CVector2D(dp.Y*-offset, dp.X*offset);
}

void SimRender::InterpolatePointsRNS(std::vector<CVector2D>& points, bool closed, float offset)
{
	PROFILE("InterpolatePointsRNS");

	std::vector<CVector2D> newPoints;

	// (This does some redundant computations for adjacent vertices,
	// but it's fairly fast (<1ms typically) so we don't worry about it yet)

	// TODO: Instead of doing a fixed number of line segments between each
	// control point, it should probably be somewhat adaptive to get a nicer
	// curve with fewer points

	size_t n = points.size();
	if (n < 1)
		return; // can't do anything unless we have two points

	size_t imax = closed ? n : n-1; // TODO: we probably need to do a bit more to handle non-closed paths

	newPoints.reserve(imax*4);

	for (size_t i = 0; i < imax; ++i)
	{
		// Get the relevant points for this spline segment
		CVector2D p0 = points[(i-1+n)%n];
		CVector2D p1 = points[i];
		CVector2D p2 = points[(i+1)%n];
		CVector2D p3 = points[(i+2)%n];

		// Do the RNS computation (based on GPG4 "Nonuniform Splines")
		float l1 = (p2 - p1).Length(); // length of spline segment (i)..(i+1)
		CVector2D s0 = (p1 - p0).Normalized(); // unit vector of spline segment (i-1)..(i)
		CVector2D s1 = (p2 - p1).Normalized(); // unit vector of spline segment (i)..(i+1)
		CVector2D s2 = (p3 - p2).Normalized(); // unit vector of spline segment (i+1)..(i+2)
		CVector2D v1 = (s0 + s1).Normalized() * l1; // spline velocity at i
		CVector2D v2 = (s1 + s2).Normalized() * l1; // spline velocity at i+1

		// Compute standard cubic spline parameters
		CVector2D a0 = p1*2 + p2*-2 + v1 + v2;
		CVector2D a1 = p1*-3 + p2*3 + v1*-2 + v2*-1;
		CVector2D a2 = v1;
		CVector2D a3 = p1;

		// Interpolate at various points
		newPoints.push_back(EvaluateSpline(0.f, a0, a1, a2, a3, offset));
		newPoints.push_back(EvaluateSpline(1.f/4.f, a0, a1, a2, a3, offset));
		newPoints.push_back(EvaluateSpline(2.f/4.f, a0, a1, a2, a3, offset));
		newPoints.push_back(EvaluateSpline(3.f/4.f, a0, a1, a2, a3, offset));
	}

	points.swap(newPoints);
}
