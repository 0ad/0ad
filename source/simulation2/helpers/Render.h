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

#ifndef INCLUDED_HELPER_RENDER
#define INCLUDED_HELPER_RENDER

/**
 * @file
 * Helper functions related to rendering
 */

class CSimContext;
class CVector2D;
class CVector3D;
class CBoundingBoxAligned;
class CBoundingBoxOriented;
struct SOverlayLine;

namespace SimRender
{

/**
 * Updates @p overlay so that it represents the given line (a list of x, z coordinate pairs),
 * flattened on the terrain (or on the water if @p floating).
 */
void ConstructLineOnGround(const CSimContext& context, const std::vector<float>& xz,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Updates @p overlay so that it represents the given circle, flattened on the terrain.
 */
void ConstructCircleOnGround(const CSimContext& context, float x, float z, float radius,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Updates @p overlay so that it represents the given square, flattened on the terrain.
 * @p x and @p z are position of center, @p w and @p h are size of rectangle, @p a is clockwise angle.
 */
void ConstructSquareOnGround(const CSimContext& context, float x, float z, float w, float h, float a,
		SOverlayLine& overlay,
		bool floating, float heightOffset = 0.25f);

/**
 * Constructs a solid outline of an arbitrarily-aligned box.
 */
void ConstructBoxOutline(const CBoundingBoxOriented& box, SOverlayLine& overlayLine);

/**
 * Constructs a solid outline of an axis-aligned bounding box.
 */
void ConstructBoxOutline(const CBoundingBoxAligned& bound, SOverlayLine& overlayLine);

/**
 * Constructs a simple gimbal outline of radius @p radius at @p center in @p out.
 * @param numSteps The amount of steps to trace a circle's complete outline. Must be a (strictly) positive multiple of four. 
 *     For small radii, you can get away with small values; setting this to 4 will create a diamond shape.
 */
void ConstructGimbal(const CVector3D& center, float radius, SOverlayLine& out, size_t numSteps = 16);

/**
 * Updates @p points so each point is averaged with its neighbours, resulting in
 * a somewhat smoother curve, assuming the points are roughly equally spaced.
 * If @p closed then the points are treated as a closed path (the last is connected
 * to the first).
 */
void SmoothPointsAverage(std::vector<CVector2D>& points, bool closed);

/**
 * Updates @p points to include intermediate points interpolating between the original
 * control points, using a rounded nonuniform spline.
 * The points are also shifted by @p offset in a direction 90 degrees clockwise from
 * the direction of the curve.
 * If @p closed then the points are treated as a closed path (the last is connected
 * to the first).
 */
void InterpolatePointsRNS(std::vector<CVector2D>& points, bool closed, float offset);

} // namespace

#endif // INCLUDED_HELPER_RENDER
