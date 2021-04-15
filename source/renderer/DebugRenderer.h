/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_DEBUGRENDERER
#define INCLUDED_DEBUGRENDERER

#include "graphics/ShaderProgramPtr.h"

class CBoundingBoxAligned;
class CBrush;
class CCamera;

// Helper for unoptimized rendering of geometrics primitives. Should not be
// used for regular passes.
class CDebugRenderer
{
public:
	/**
	 * Render: Renders the camera's frustum in world space.
	 * The caller should set the color using glColorXy before calling Render.
	 *
	 * @param intermediates determines how many intermediate distance planes should
	 * be hinted at between the near and far planes
	 */
	void DrawCameraFrustum(const CCamera& camera, int intermediates = 0) const;

	/**
	 * Render the surfaces of the bound box as triangles.
	 */
	void DrawBoundingBox(const CBoundingBoxAligned& boundingBox, const CShaderProgramPtr& shader) const;

	/**
	 * Render the outline of the bound box as lines.
	 */
	void DrawBoundingBoxOutline(const CBoundingBoxAligned& boundingBox, const CShaderProgramPtr& shader) const;

	/**
	 * Render the surfaces of the brush as triangles.
	 */
	void DrawBrush(const CBrush& brush, const CShaderProgramPtr& shader) const;

	/**
	 * Render the outline of the brush as lines.
	 */
	void DrawBrushOutline(const CBrush& brush, const CShaderProgramPtr& shader) const;
};

#endif // INCLUDED_DEBUGRENDERER
