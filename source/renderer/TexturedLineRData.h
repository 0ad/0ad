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

#ifndef INCLUDED_TEXTUREDLINERDATA
#define INCLUDED_TEXTUREDLINERDATA

#include "graphics/Overlay.h"
#include "graphics/RenderableObject.h"
#include "graphics/ShaderProgram.h"
#include "graphics/TextureManager.h"
#include "renderer/VertexBufferManager.h"

/**
 * Rendering data for a single textured overlay line. Used by the OverlayRenderer.
 */
class CTexturedLineRData : public CRenderData
{
public:

	/**
	 * @param line Overlay line to associate this render data with. Must not be null.
	 */
	CTexturedLineRData(SOverlayTexturedLine* line) : m_Line(line), m_VB(NULL), m_VBIndices(NULL)
	{ ENSURE(m_Line && "Cannot create textured line render data for null overlay line"); }

	~CTexturedLineRData()
	{
		if (m_VB)
			g_VBMan.Release(m_VB);
		if (m_VBIndices)
			g_VBMan.Release(m_VBIndices);
	}

	void Update();

	void Render(const CShaderProgramPtr& shader);

protected:

	struct SVertex
	{
		SVertex(CVector3D pos, float u, float v) : m_Position(pos) { m_UVs[0] = u; m_UVs[1] = v; }
		CVector3D m_Position;
		GLfloat m_UVs[2];
		float _padding[3]; // get a pow2 struct size
	};
	cassert(sizeof(SVertex) == 32);

	/**
	 * Creates a line cap of the specified type @p endCapType at the end of the segment going in direction @p normal, and appends
	 * the vertices to @p verticesOut in GL_TRIANGLES order.
	 *
	 * @param corner1 One of the two butt-end corner points of the line to which the cap should be attached.
	 * @param corner2 One of the two butt-end corner points of the line to which the cap should be attached.
	 * @param normal Normal vector indicating the direction of the segment to which the cap should be attached.
	 * @param endCapType The type of end cap to produce.
	 * @param verticesOut Output vector of vertices for passing to the renderer.
	 * @param indicesOut Output vector of vertex indices for passing to the renderer.
	 */
	void CreateLineCap(const CVector3D& corner1, const CVector3D& corner2, const CVector3D& normal,
		               SOverlayTexturedLine::LineCapType endCapType, std::vector<SVertex>& verticesOut, std::vector<u16>& indicesOut);

	/// Small utility function; grabs the centroid of the positions of two vertices
	inline CVector3D Centroid(const SVertex& v1, const SVertex& v2)
	{
		return (v1.m_Position + v2.m_Position) * 0.5;
	}

	SOverlayTexturedLine* m_Line;
	CVertexBuffer::VBChunk* m_VB;
	CVertexBuffer::VBChunk* m_VBIndices;
};

#endif // INCLUDED_TEXTUREDLINERDATA