/* Copyright (C) 2011 Wildfire Games.
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

#include "graphics/RenderableObject.h"
#include "graphics/Texture.h"
#include "maths/Vector3D.h"
#include "ps/Overlay.h" // CColor  (TODO: that file has nothing to do with overlays, it should be renamed)

class CTerrain;

/**
 * Line-based overlay, with world-space coordinates, rendered in the world
 * potentially behind other objects. Designed for selection circles and debug info.
 */
struct SOverlayLine
{
	SOverlayLine() : m_Thickness(1) { }

	CColor m_Color;
	std::vector<float> m_Coords; // (x, y, z) vertex coordinate triples; shape is not automatically closed
	u8 m_Thickness; // pixels
};

/**
 * Textured line overlay, with world-space coordinates, rendered in the world
 * onto the terrain. Designed for territory borders.
 */
struct SOverlayTexturedLine
{
	SOverlayTexturedLine() : m_Terrain(NULL), m_Thickness(1.0f) { }

	CTerrain* m_Terrain;
	CTexturePtr m_TextureBase;
	CTexturePtr m_TextureMask;
	CColor m_Color;
	std::vector<float> m_Coords; // (x, z) vertex coordinate pairs; y is computed automatically; shape is automatically closed
	float m_Thickness; // world-space units

	shared_ptr<CRenderData> m_RenderData; // cached renderer data (shared_ptr so that copies/deletes are automatic)
};

/**
 * Billboard sprite overlay, with world-space coordinates, rendered on top
 * of all other objects. Designed for health bars and rank icons.
 */
struct SOverlaySprite
{
	CTexturePtr m_Texture;
	CVector3D m_Position; // base position
	float m_X0, m_Y0, m_X1, m_Y1; // billboard corner coordinates, relative to base position
};

// TODO: OverlayText

#endif // INCLUDED_GRAPHICS_OVERLAY
