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

#include "Render.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/BoundingBoxOriented.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "maths/Vector2D.h"
#include "ps/Profile.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/helpers/Geometry.h"

void SimRender::ConstructLineOnGround(const CSimContext& context, const std::vector<float>& xz,
		SOverlayLine& overlay, bool floating, float heightOffset)
{
	PROFILE("ConstructLineOnGround");

	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return;

	if (xz.size() < 2)
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterManager(context, SYSTEM_ENTITY);
		if (cmpWaterManager)
			water = cmpWaterManager->GetExactWaterLevel(xz[0], xz[1]);
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

static void ConstructCircleOrClosedArc(
	const CSimContext& context, float x, float z, float radius,
	bool isCircle,
	float start, float end,
	SOverlayLine& overlay, bool floating, float heightOffset)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterManager(context, SYSTEM_ENTITY);
		if (cmpWaterManager)
			water = cmpWaterManager->GetExactWaterLevel(x, z);
	}

	// Adapt the circle resolution to look reasonable for small and largeish radiuses
	size_t numPoints = clamp((size_t)(radius*(end-start)), (size_t)12, (size_t)48);

	if (!isCircle)
		overlay.m_Coords.reserve((numPoints + 1 + 2) * 3);
	else
		overlay.m_Coords.reserve((numPoints + 1) * 3);

	float cy;
	if (!isCircle)
	{
		// Start at the center point
		cy = std::max(water, cmpTerrain->GetExactGroundLevel(x, z)) + heightOffset;
		overlay.m_Coords.push_back(x);
		overlay.m_Coords.push_back(cy);
		overlay.m_Coords.push_back(z);
	}

	for (size_t i = 0; i <= numPoints; ++i) // use '<=' so it's a closed loop
	{
		float a = start + (float)i * (end - start) / (float)numPoints;
		float px = x + radius * cosf(a);
		float pz = z + radius * sinf(a);
		float py = std::max(water, cmpTerrain->GetExactGroundLevel(px, pz)) + heightOffset;
		overlay.m_Coords.push_back(px);
		overlay.m_Coords.push_back(py);
		overlay.m_Coords.push_back(pz);
	}

	if (!isCircle)
	{
		// Return to the center point
		overlay.m_Coords.push_back(x);
		overlay.m_Coords.push_back(cy);
		overlay.m_Coords.push_back(z);
	}
}

void SimRender::ConstructCircleOnGround(
	const CSimContext& context, float x, float z, float radius,
	SOverlayLine& overlay, bool floating, float heightOffset)
{
	ConstructCircleOrClosedArc(context, x, z, radius, true, 0.0f, 2.0f*(float)M_PI, overlay, floating, heightOffset);
}

void SimRender::ConstructClosedArcOnGround(
	const CSimContext& context, float x, float z, float radius,
	float start, float end,
	SOverlayLine& overlay, bool floating, float heightOffset)
{
	ConstructCircleOrClosedArc(context, x, z, radius, false, start, end, overlay, floating, heightOffset);
}

// This method splits up a straight line into a number of line segments each having a length ~= TERRAIN_TILE_SIZE
static void SplitLine(std::vector<std::pair<float, float> >& coords, float x1, float y1, float x2, float y2)
{
	float length = sqrtf(SQR(x1 - x2) + SQR(y1 - y2));
	size_t pieces = ((int)length) / TERRAIN_TILE_SIZE;
	if (pieces > 0)
	{
		float xPieceLength = (x1 - x2) / (float)pieces;
		float yPieceLength = (y1 - y2) / (float)pieces;
		for (size_t i = 1; i <= (pieces - 1); ++i)
		{
			coords.emplace_back(x1 - (xPieceLength * (float)i), y1 - (yPieceLength * (float)i));
		}
	}
	coords.emplace_back(x2, y2);
}

void SimRender::ConstructSquareOnGround(const CSimContext& context, float x, float z, float w, float h, float a,
		SOverlayLine& overlay, bool floating, float heightOffset)
{
	overlay.m_Coords.clear();

	CmpPtr<ICmpTerrain> cmpTerrain(context, SYSTEM_ENTITY);
	if (!cmpTerrain)
		return;

	float water = 0.f;
	if (floating)
	{
		CmpPtr<ICmpWaterManager> cmpWaterManager(context, SYSTEM_ENTITY);
		if (cmpWaterManager)
			water = cmpWaterManager->GetExactWaterLevel(x, z);
	}

	float c = cosf(a);
	float s = sinf(a);

	std::vector<std::pair<float, float> > coords;

	// Add the first vertex, since SplitLine will be adding only the second end-point of the each line to
	// the coordinates list. We don't have to worry about the other lines, since the end-point of one line
	// will be the starting point of the next
	coords.emplace_back(x - w/2*c + h/2*s, z + w/2*s + h/2*c);

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

void SimRender::ConstructAxesMarker(const CMatrix3D& coordSystem, SOverlayLine& outX, SOverlayLine& outY, SOverlayLine& outZ)
{
	outX.m_Coords.clear();
	outY.m_Coords.clear();
	outZ.m_Coords.clear();

	outX.m_Color = CColor(1, 0, 0, .5f); // X axis; red
	outY.m_Color = CColor(0, 1, 0, .5f); // Y axis; green
	outZ.m_Color = CColor(0, 0, 1, .5f); // Z axis; blue

	outX.m_Thickness = 2;
	outY.m_Thickness = 2;
	outZ.m_Thickness = 2;

	CVector3D origin = coordSystem.GetTranslation();
	outX.PushCoords(origin);
	outY.PushCoords(origin);
	outZ.PushCoords(origin);

	outX.PushCoords(origin + CVector3D(coordSystem(0,0), coordSystem(1,0), coordSystem(2,0)));
	outY.PushCoords(origin + CVector3D(coordSystem(0,1), coordSystem(1,1), coordSystem(2,1)));
	outZ.PushCoords(origin + CVector3D(coordSystem(0,2), coordSystem(1,2), coordSystem(2,2)));
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

void SimRender::InterpolatePointsRNS(std::vector<CVector2D>& points, bool closed, float offset, int segmentSamples /* = 4 */)
{
	PROFILE("InterpolatePointsRNS");
	ENSURE(segmentSamples > 0);

	std::vector<CVector2D> newPoints;

	// (This does some redundant computations for adjacent vertices,
	// but it's fairly fast (<1ms typically) so we don't worry about it yet)

	// TODO: Instead of doing a fixed number of line segments between each
	// control point, it should probably be somewhat adaptive to get a nicer
	// curve with fewer points

	size_t n = points.size();

	if (closed)
	{
		if (n < 1)
			return; // we need at least a single point to not crash
	}
	else
	{
		if (n < 2)
			return; // in non-closed mode, we need at least n=2 to not crash
	}

	size_t imax = closed ? n : n-1;
	newPoints.reserve(imax*segmentSamples);

	// these are primarily used inside the loop, but for open paths we need them outside the loop once to compute the last point
	CVector2D a0;
	CVector2D a1;
	CVector2D a2;
	CVector2D a3;

	for (size_t i = 0; i < imax; ++i)
	{
		// Get the relevant points for this spline segment; each step interpolates the segment between p1 and p2; p0 and p3 are the points
		// before p1 and after p2, respectively; they're needed to compute tangents and whatnot.
		CVector2D p0; // normally points[(i-1+n)%n], but it's a bit more complicated due to open/closed paths -- see below
		CVector2D p1 = points[i];
		CVector2D p2 = points[(i+1)%n];
		CVector2D p3; // normally points[(i+2)%n], but it's a bit more complicated due to open/closed paths -- see below

		if (!closed && (i == 0))
			// p0's point index is out of bounds, and we can't wrap around because we're in non-closed mode -- create an artificial point
			// that extends p1 -> p0 (i.e. the first segment's direction)
			p0 = points[0] + (points[0] - points[1]);
		else
			// standard wrap-around case
			p0 = points[(i-1+n)%n]; // careful; don't use (i-1)%n here, as the result is machine-dependent for negative operands (e.g. if i==0, the result could be either -1 or n-1)


		if (!closed && (i == n-2))
			// p3's point index is out of bounds; create an artificial point that extends p_(n-2) -> p_(n-1) (i.e. the last segment's direction)
			// (note that p2's index should not be out of bounds, because in non-closed mode imax is reduced by 1)
			p3 = points[n-1] + (points[n-1] - points[n-2]);
		else
			// standard wrap-around case
			p3 = points[(i+2)%n];


		// Do the RNS computation (based on GPG4 "Nonuniform Splines")
		float l1 = (p2 - p1).Length(); // length of spline segment (i)..(i+1)
		CVector2D s0 = (p1 - p0).Normalized(); // unit vector of spline segment (i-1)..(i)
		CVector2D s1 = (p2 - p1).Normalized(); // unit vector of spline segment (i)..(i+1)
		CVector2D s2 = (p3 - p2).Normalized(); // unit vector of spline segment (i+1)..(i+2)
		CVector2D v1 = (s0 + s1).Normalized() * l1; // spline velocity at i
		CVector2D v2 = (s1 + s2).Normalized() * l1; // spline velocity at i+1

		// Compute standard cubic spline parameters
		a0 = p1*2 + p2*-2 + v1 + v2;
		a1 = p1*-3 + p2*3 + v1*-2 + v2*-1;
		a2 = v1;
		a3 = p1;

		// Interpolate at regular points across the interval
		for (int sample = 0; sample < segmentSamples; sample++)
			newPoints.push_back(EvaluateSpline(sample/((float) segmentSamples), a0, a1, a2, a3, offset));

	}

	if (!closed)
		// if the path is open, we should take care to include the last control point
		// NOTE: we can't just do push_back(points[n-1]) here because that ignores the offset
		newPoints.push_back(EvaluateSpline(1.f, a0, a1, a2, a3, offset));

	points.swap(newPoints);
}

void SimRender::ConstructDashedLine(const std::vector<CVector2D>& keyPoints, SDashedLine& dashedLineOut, const float dashLength, const float blankLength)
{
	// sanity checks
	if (dashLength <= 0)
		return;

	if (blankLength <= 0)
		return;

	if (keyPoints.size() < 2)
		return;

	dashedLineOut.m_Points.clear();
	dashedLineOut.m_StartIndices.clear();

	// walk the line, counting the total length so far at each node point. When the length exceeds dashLength, cut the last segment at the
	// required length and continue for blankLength along the line to start a new dash segment.

	// TODO: we should probably extend this function to also allow for closed lines. I was thinking of slightly scaling the dash/blank length
	// so that it fits the length of the curve, but that requires knowing the length of the curve upfront which is sort of expensive to compute
	// (O(n) and lots of square roots).

	bool buildingDash = true; // true if we're building a dash, false if a blank
	float curDashLength = 0; // builds up the current dash/blank's length as we walk through the line nodes
	CVector2D dashLastPoint = keyPoints[0]; // last point of the current dash/blank being built.

	// register the first starting node of the first dash
	dashedLineOut.m_Points.push_back(keyPoints[0]);
	dashedLineOut.m_StartIndices.push_back(0);

	// index of the next key point on the path. Must always point to a node that is further along the path than dashLastPoint, so we can
	// properly take a direction vector along the path.
	size_t i = 0;

	while(i < keyPoints.size() - 1)
	{
		// get length of this segment
		CVector2D segmentVector = keyPoints[i + 1] - dashLastPoint; // vector from our current point along the path to nextNode
		float segmentLength = segmentVector.Length();

		float targetLength = (buildingDash ? dashLength : blankLength);
		if (curDashLength + segmentLength > targetLength)
		{
			// segment is longer than the dash length we still have to go, so we'll need to cut it; create a cut point along the segment
			// line that is of just the required length to complete the dash, then make it the base point for the next dash/blank.
			float cutLength = targetLength - curDashLength;
			CVector2D cutPoint = dashLastPoint + (segmentVector.Normalized() * cutLength);

			// start a new dash or blank in the next iteration
			curDashLength = 0;
			buildingDash = !buildingDash; // flip from dash to blank and vice-versa
			dashLastPoint = cutPoint;

			// don't increment i, we haven't fully traversed this segment yet so we still need to use the same point to take the
			// direction vector with in the next iteration

			// this cut point is either the end of the current dash or the beginning of a new dash; either way, we're gonna need it
			// in the points array.
			dashedLineOut.m_Points.push_back(cutPoint);

			if (buildingDash)
			{
				// if we're gonna be building a new dash, then cutPoint is now the base point of that new dash, so let's register its
				// index as a start index of a dash.
				dashedLineOut.m_StartIndices.push_back(dashedLineOut.m_Points.size() - 1);
			}

		}
		else
		{
			// the segment from lastDashPoint to keyPoints[i+1] doesn't suffice to complete the dash, so we need to add keyPoints[i+1]
			// to this dash's points and continue from there

			if (buildingDash)
				// still building the dash, add it to the output (we don't need to store the blanks)
				dashedLineOut.m_Points.push_back(keyPoints[i+1]);

			curDashLength += segmentLength;
			dashLastPoint = keyPoints[i+1];
			i++;

		}

	}

}

// TODO: this serves a similar purpose to SplitLine above, but is more general. Also, SplitLine seems to be implemented more
// efficiently, might be nice to take some cues from it
void SimRender::SubdividePoints(std::vector<CVector2D>& points, float maxSegmentLength, bool closed)
{
	size_t numControlPoints = points.size();
	if (numControlPoints < 2)
		return;

	ENSURE(maxSegmentLength > 0);

	size_t endIndex = numControlPoints;
	if (!closed && numControlPoints > 2)
		endIndex--;

	std::vector<CVector2D> newPoints;

	for (size_t i = 0; i < endIndex; i++)
	{
		const CVector2D& curPoint = points[i];
		const CVector2D& nextPoint = points[(i+1) % numControlPoints];
		const CVector2D line(nextPoint - curPoint);
		CVector2D lineDirection = line.Normalized();

		// include control point i + a list of intermediate points between i and i + 1 (excluding i+1 itself)
		newPoints.push_back(curPoint);

		// calculate how many intermediate points are needed so that each segment is of length <= maxSegmentLength
		float lineLength = line.Length();
		size_t numSegments = (size_t) ceilf(lineLength / maxSegmentLength);
		float segmentLength = lineLength / numSegments;

		for (size_t s = 1; s < numSegments; ++s) // start at one, we already included curPoint
		{
			newPoints.push_back(curPoint + lineDirection * (s * segmentLength));
		}
	}

	points.swap(newPoints);
}
