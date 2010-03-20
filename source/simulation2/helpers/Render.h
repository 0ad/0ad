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
struct SOverlayLine;

namespace SimRender
{

/**
 * Updates @p overlay so that it represents the given circle, flattened on the terrain.
 */
void ConstructCircleOnGround(const CSimContext& context, float x, float z, float radius, SOverlayLine& overlay);

/**
 * Updates @p overlay so that it represents the given square, flattened on the terrain.
 * @p x and @p z are position of center, @p w and @p h are size of rectangle, @p a is clockwise angle.
 */
void ConstructSquareOnGround(const CSimContext& context, float x, float z, float w, float h, float a, SOverlayLine& overlay);

} // namespace

#endif // INCLUDED_HELPER_RENDER
