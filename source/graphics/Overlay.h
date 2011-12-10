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

	/// Utility function; pushes three vertex coordinates at once onto the coordinates array
	void PushCoords(const float x, const float y, const float z) { m_Coords.push_back(x); m_Coords.push_back(y); m_Coords.push_back(z); }
	/// Utility function; pushes a vertex location onto the coordinates array
	void PushCoords(const CVector3D& v) { PushCoords(v.X, v.Y, v.Z); }
};

/**
 * Textured line overlay, with world-space coordinates, rendered in the world
 * onto the terrain. Designed for territory borders.
 */
struct SOverlayTexturedLine
{
	enum LineCapType
	{
		LINECAP_FLAT, ///< no line ending; abrupt stop of the line (aka. butt ending)

		/**
		 * Semi-circular line ending. The texture is mapped by curving the left vertical edge around the semi-circle's rim. That is,
		 * the center point has UV coordinates (0.5;0.5), and the rim vertices all have U coordinate 0 and a V coordinate that ranges
		 * from 0 to 1 as the rim is traversed.
		 */
		LINECAP_ROUND,
		LINECAP_SHARP, ///< sharp point ending
		LINECAP_SQUARE, ///< square end that extends half the line width beyond the line end
	};

	SOverlayTexturedLine()
		: m_Terrain(NULL), m_Thickness(1.0f), m_Closed(false), m_AlwaysVisible(false), m_StartCapType(LINECAP_FLAT), m_EndCapType(LINECAP_FLAT)
	{}

	CTerrain* m_Terrain;
	CTexturePtr m_TextureBase;
	CTexturePtr m_TextureMask;
	CColor m_Color; ///< Color to apply to the line texture
	std::vector<float> m_Coords; ///< (x, z) vertex coordinate pairs; y is computed automatically
	float m_Thickness; ///< Half-width of the line, in world-space units

	bool m_Closed; ///< Should this line be treated as a closed loop? (if set, the end cap settings are ignored)
	bool m_AlwaysVisible; ///< Should this line be rendered even under the SoD?
	LineCapType m_StartCapType; ///< LineCapType to be used at the start of the line
	LineCapType m_EndCapType; ///< LineCapType to be used at the end of the line

	shared_ptr<CRenderData> m_RenderData; ///< Cached renderer data (shared_ptr so that copies/deletes are automatic)

	/**
	 * Converts a string line cap type into its corresponding LineCap enum value, and returns the resulting value.
	 * If the input string is unrecognized, a warning is issued and a default value is returned.
	 */
	static LineCapType StrToLineCapType(const std::wstring& str);
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
