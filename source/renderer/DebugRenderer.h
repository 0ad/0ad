/* Copyright (C) 2023 Wildfire Games.
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

#include "graphics/ShaderTechniquePtr.h"
#include "ps/CStrIntern.h"
#include "renderer/backend/IShaderProgram.h"

#include <unordered_map>
#include <vector>

class CBoundingBoxAligned;
class CBrush;
class CCamera;
class CMatrix3D;
class CVector3D;

struct CColor;

// Helper for unoptimized rendering of geometrics primitives. Should not be
// used for regular passes.
class CDebugRenderer
{
public:
	void Initialize();

	/**
	 * Render the line in world space.
	 */
	void DrawLine(const CVector3D& from, const CVector3D& to,
		const CColor& color, const float width, const bool depthTestEnabled = true);
	void DrawLine(const std::vector<CVector3D>& line,
		const CColor& color, const float width, const bool depthTestEnabled = true);

	/**
	 * Render the circle in world space oriented to the view camera.
	 */
	void DrawCircle(const CVector3D& origin, const float radius, const CColor& color);

	/**
	 * Render: Renders the camera's frustum in world space.
	 *
	 * @param intermediates determines how many intermediate distance planes should
	 * be hinted at between the near and far planes
	 */
	void DrawCameraFrustum(const CCamera& camera, const CColor& color, int intermediates = 0, bool wireframe = false);

	/**
	 * Render the surfaces of the bound box as triangles.
	 */
	void DrawBoundingBox(const CBoundingBoxAligned& boundingBox, const CColor& color, bool wireframe = false);
	void DrawBoundingBox(const CBoundingBoxAligned& boundingBox, const CColor& color, const CMatrix3D& transform, bool wireframe = false);

	/**
	 * Render the surfaces of the brush as triangles.
	 */
	void DrawBrush(const CBrush& brush, const CColor& color, bool wireframe = false);

private:
	const CShaderTechniquePtr& GetShaderTechnique(
		const CStrIntern name, const CColor& color, const bool depthTestEnabled = true,
		const bool wireframe = false);

	struct ShaderTechniqueKey
	{
		CStrIntern name;
		bool transparent;
		bool depthTestEnabled;
		bool wireframe;
	};
	struct ShaderTechniqueKeyHash
	{
		size_t operator()(const ShaderTechniqueKey& key) const;
	};
	struct ShaderTechniqueKeyEqual
	{
		bool operator()(const ShaderTechniqueKey& lhs, const ShaderTechniqueKey& rhs) const;
	};
	std::unordered_map<ShaderTechniqueKey, CShaderTechniquePtr, ShaderTechniqueKeyHash, ShaderTechniqueKeyEqual>
		m_ShaderTechniqueMapping;

	Renderer::Backend::IVertexInputLayout* m_VertexInputLayout = nullptr;
};

#endif // INCLUDED_DEBUGRENDERER
