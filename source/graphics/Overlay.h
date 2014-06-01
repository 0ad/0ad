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

#ifndef INCLUDED_GRAPHICS_OVERLAY
#define INCLUDED_GRAPHICS_OVERLAY

#include "graphics/RenderableObject.h"
#include "graphics/Texture.h"
#include "maths/Vector2D.h"
#include "maths/Vector3D.h"
#include "maths/FixedVector3D.h"
#include "ps/Overlay.h" // CColor  (TODO: that file has nothing to do with overlays, it should be renamed)

class CTerrain;
class CSimContext;
class CTexturedLineRData;

/**
 * Line-based overlay, with world-space coordinates, rendered in the world
 * potentially behind other objects. Designed for selection circles and debug info.
 */
struct SOverlayLine
{
	SOverlayLine() : m_Thickness(1) { }

	CColor m_Color;
	std::vector<float> m_Coords; // (x, y, z) vertex coordinate triples; shape is not automatically closed
	u8 m_Thickness; // in pixels

	void PushCoords(const CVector3D& v) { PushCoords(v.X, v.Y, v.Z); }
	void PushCoords(const float x, const float y, const float z)
	{
		m_Coords.push_back(x);
		m_Coords.push_back(y);
		m_Coords.push_back(z);
	}
};

/**
 * Textured line overlay, with world-space coordinates, rendered in the world onto the terrain.
 * Designed for relatively static textured lines, i.e. territory borders, originally.
 * 
 * Once submitted for rendering, instances must not be copied afterwards. The reason is that they
 * are assigned rendering data that is unique to the submitted instance, and non-transferable to
 * any copies that would otherwise be made. Amongst others, this restraint includes that they must
 * not be submitted by their address inside a std::vector storing them by value.
 */
struct SOverlayTexturedLine
{
	enum LineCapType
	{
		LINECAP_FLAT, ///< no line ending; abrupt stop of the line (aka. butt ending)

		/**
		 * Semi-circular line ending. The texture is mapped by curving the left vertical edge
		 * around the semi-circle's rim. That is, the center point has UV coordinates (0.5;0.5),
		 * and the rim vertices all have U coordinate 0 and a V coordinate that ranges from 0 to
		 * 1 as the rim is traversed.
		 */
		LINECAP_ROUND,
		LINECAP_SHARP, ///< sharp point ending
		LINECAP_SQUARE, ///< square end that extends half the line width beyond the line end
	};

	SOverlayTexturedLine()
		: m_Thickness(1.0f), m_Closed(false), m_AlwaysVisible(false),
		  m_StartCapType(LINECAP_FLAT), m_EndCapType(LINECAP_FLAT), m_SimContext(NULL)
	{ }

	CTexturePtr m_TextureBase;
	CTexturePtr m_TextureMask;

	/// Color to apply to the line texture, where indicated by the mask.
	CColor m_Color;
	/// (x, z) vertex coordinate pairs; y is computed automatically.
	std::vector<float> m_Coords;
	/// Half-width of the line, in world-space units.
	float m_Thickness;
	/// Should this line be treated as a closed loop? If set, any end cap settings are ignored.
	bool m_Closed;
	/// Should this line be rendered fully visible at all times, even under the SoD?
	bool m_AlwaysVisible;

	LineCapType m_StartCapType;
	LineCapType m_EndCapType;

	/**
	 * Simulation context applicable for this overlay line; used to obtain terrain information
	 * during automatic computation of Y coordinates.
	 */
	const CSimContext* m_SimContext;

	/**
	 * Cached renderer data, because expensive to compute. Allocated by the renderer when necessary 
	 * for rendering purposes.
	 * 
	 * Note: the rendering data may be shared between copies of this object to prevent having to 
	 * recompute it, while at the same time maintaining copyability of this object (see also docs on 
	 * CTexturedLineRData).
	 */
	shared_ptr<CTexturedLineRData> m_RenderData;

	/**
	 * Converts a string line cap type into its corresponding LineCap enum value, and returns
	 * the resulting value. If the input string is unrecognized, a warning is issued and a
	 * default value is returned.
	 */
	static LineCapType StrToLineCapType(const std::wstring& str);

	void PushCoords(const float x, const float z) { m_Coords.push_back(x); m_Coords.push_back(z); }
	void PushCoords(const CVector2D& v) { PushCoords(v.X, v.Y); }
	void PushCoords(const std::vector<CVector2D>& points)
	{
		for (size_t i = 0; i < points.size(); ++i)
			PushCoords(points[i]);
	}
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

/**
 * Rectangular single-quad terrain overlay, in world space coordinates. The vertices of the quad
 * are not required to be coplanar; the quad is arbitrarily triangulated with no effort being made
 * to find a best fit to the underlying terrain.
 */
struct SOverlayQuad
{
	CTexturePtr m_Texture;
	CTexturePtr m_TextureMask;
	CVector3D m_Corners[4];
	CColor m_Color;
};

struct SOverlaySphere
{
	SOverlaySphere() : m_Radius(0) { }

	CVector3D m_Center;
	float m_Radius;
	CColor m_Color;
};

// TODO: OverlayText

#endif // INCLUDED_GRAPHICS_OVERLAY
