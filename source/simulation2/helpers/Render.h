/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_HELPER_RENDER
#define INCLUDED_HELPER_RENDER

/**
 * @file
 * Helper functions related to rendering
 */

#include "maths/Vector2D.h"

class CSimContext;
class CVector2D;
class CVector3D;
class CMatrix3D;
class CBoundingBoxAligned;
class CBoundingBoxOriented;
struct SOverlayLine;

struct SDashedLine
{
	/// Packed array of consecutive dashes' points. Use m_StartIndices to navigate it.
	std::vector<CVector2D> m_Points;

	/**
	 * Start indices in m_Points of each dash. Dash n starts at point m_StartIndices[n] and ends at the point with index
	 * m_StartIndices[n+1] - 1, or at the end of the m_Points vector. Use the GetEndIndex(n) convenience method to abstract away the
	 * difference and get the (exclusive) end index of dash n.
	 */
	std::vector<size_t> m_StartIndices;

	/// Returns the (exclusive) end point index (i.e. index within m_Points) of dash n.
	size_t GetEndIndex(size_t i)
	{
		// for the last dash, there is no next starting index, so we need to use the end index of the m_Points array instead
		return (i < m_StartIndices.size() - 1 ? m_StartIndices[i+1] : m_Points.size());
	}
};

namespace SimRender
{

/**
 * Constructs overlay line from given points, conforming to terrain.
 *
 * @param[in] xz List of x,z coordinate pairs representing the line.
 * @param[in,out] overlay Updated overlay line now conforming to terrain.
 * @param[in] floating If true, the line conforms to water as well.
 * @param[in] heightOffset Height above terrain to offset the line.
 */
void ConstructLineOnGround(
		const CSimContext& context, const std::vector<float>& xz,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Constructs overlay line as a circle with given center and radius, conforming to terrain.
 *
 * @param[in] x,z Coordinates of center of circle.
 * @param[in] radius Radius of circle to construct.
 * @param[in,out] overlay Updated overlay line representing this circle.
 * @param[in] floating If true, the circle conforms to water as well.
 * @param[in] heightOffset Height above terrain to offset the circle.
 * @param heightOffset The vertical offset to apply to points, to raise the line off the terrain a bit.
 */
void ConstructCircleOnGround(
		const CSimContext& context, float x, float z, float radius,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Constructs overlay line as an outlined circle sector (an arc with straight lines between the
 * endpoints and the circle's center), conforming to terrain.
 */
void ConstructClosedArcOnGround(
		const CSimContext& context, float x, float z, float radius,
		float start, float end,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Constructs overlay line as rectangle with given center and dimensions, conforming to terrain.
 *
 * @param[in] x,z Coordinates of center of rectangle.
 * @param[in] w,h Width/height dimensions of the rectangle.
 * @param[in] a Clockwise angle to orient the rectangle.
 * @param[in,out] overlay Updated overlay line representing this rectangle.
 * @param[in] floating If true, the rectangle conforms to water as well.
 * @param[in] heightOffset Height above terrain to offset the rectangle.
 */
void ConstructSquareOnGround(
		const CSimContext& context, float x, float z, float w, float h, float a,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Constructs a solid outline of an arbitrarily-aligned bounding @p box.
 *
 * @param[in] box
 * @param[in,out] overlayLine Updated overlay line representing the oriented box.
 */
void ConstructBoxOutline(const CBoundingBoxOriented& box, SOverlayLine& overlayLine);

/**
 * Constructs a solid outline of an axis-aligned bounding @p box.
 *
 * @param[in] bound
 * @param[in,out] overlayLine Updated overlay line representing the AABB.
 */
void ConstructBoxOutline(const CBoundingBoxAligned& box, SOverlayLine& overlayLine);

/**
 * Constructs a simple gimbal outline with the given radius and center.
 *
 * @param[in] center
 * @param[in] radius
 * @param[in,out] out Updated overlay line representing the gimbal.
 * @param[in] numSteps The amount of steps to trace a circle's complete outline. Must be a (strictly) positive multiple of four.
 *     For small radii, you can get away with small values; setting this to 4 will create a diamond shape.
 */
void ConstructGimbal(const CVector3D& center, float radius, SOverlayLine& out, size_t numSteps = 16);

/**
 * Constructs 3D axis marker overlay lines for the given coordinate system.
 * The XYZ axes are colored RGB, respectively.
 *
 * @param[in] coordSystem Specifies the coordinate system.
 * @param[out] outX,outY,outZ Constructed overlay lines for each axes.
 */
void ConstructAxesMarker(const CMatrix3D& coordSystem, SOverlayLine& outX, SOverlayLine& outY, SOverlayLine& outZ);

/**
 * Updates the given points so each point is averaged with its neighbours, resulting in
 * a somewhat smoother curve, assuming the points are roughly equally spaced.
 *
 * @param[in,out] points List of points to smooth.
 * @param[in] closed if true, then the points are treated as a closed path (the last is connected
 *	to the first).
 */
void SmoothPointsAverage(std::vector<CVector2D>& points, bool closed);

/**
 * Updates the given points to include intermediate points interpolating between the original
 * control points, using a rounded nonuniform spline.
 *
 * @param[in,out] points List of points to interpolate.
 * @param[in] closed if true, then the points are treated as a closed path (the last is connected
 *	to the first).
 * @param[in] offset The points are shifted by this distance in a direction 90 degrees clockwise from
 *	the direction of the curve.
 * @param[in] segmentSamples Amount of intermediate points to sample between every two control points.
 */
void InterpolatePointsRNS(std::vector<CVector2D>& points, bool closed, float offset, int segmentSamples = 4);

/**
 * Creates a dashed line from the given line, dash length, and blank space between.
 *
 * @param[in] linePoints List of points specifying the input line.
 * @param[out] dashedLineOut The dashed line returned as a list of smaller lines
 * @param[in] dashLength Length of a single dash. Must be strictly positive.
 * @param[in] blankLength Length of a single blank between dashes. Must be strictly positive.
 */
void ConstructDashedLine(const std::vector<CVector2D>& linePoints, SDashedLine& dashedLineOut,
		const float dashLength, const float blankLength);

/**
 * Computes angular step parameters @p out_stepAngle and @p out_numSteps, given a @p maxChordLength on a circle of radius @p radius.
 * The resulting values satisfy @p out_numSteps * @p out_stepAngle = 2*PI.
 *
 * This function is used to find the angular step parameters when drawing a circle outline approximated by several connected chords;
 * it returns the step angle and number of steps such that the length of each resulting chord is less than or equal to @p maxChordLength.
 * By stating that each chord cannot be longer than a particular length, a certain level of visual smoothness of the resulting circle
 * outline can be guaranteed independently of the radius of the outline.
 *
 * @param radius Radius of the circle. Must be strictly positive.
 * @param maxChordLength Desired maximum length of individual chords. Must be strictly positive.
 */
void AngularStepFromChordLen(const float maxChordLength, const float radius, float& out_stepAngle, unsigned& out_numSteps);

/**
 * Subdivides a list of @p points into segments of maximum length @p maxSegmentLength that are of equal size between every two
 * control points. The resulting subdivided list of points is written back to @p points.
 *
 * @param points The list of intermediate points to subdivide.
 * @param maxSegmentLength The maximum length of a single segment after subdivision. Must be strictly positive.
 * @param closed Should the provided list of points be treated as a closed shape? If true, the resulting list of points will include
 *        extra subdivided points between the last and the first point.
 */
void SubdividePoints(std::vector<CVector2D>& points, float maxSegmentLength, bool closed);

} // namespace

#endif // INCLUDED_HELPER_RENDER
