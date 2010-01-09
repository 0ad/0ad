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

#ifndef INCLUDED_GRAPHICS_OVERLAY
#define INCLUDED_GRAPHICS_OVERLAY

#include "ps/Overlay.h" // CColor  (TODO: that file has nothing to do with overlays, it should be renamed)

/**
 * Line-based overlay. Exists in world-space, but gets rendered on top
 * of all other objects. Designed for selection circles.
 */
struct SOverlayLine
{
	CColor m_Color;
	std::vector<float> m_Coords; // (x, y, z) vertex coordinate triples; shape is not automatically closed
};

// TODO: OverlaySprite, OverlayText

#endif // INCLUDED_GRAPHICS_OVERLAY
