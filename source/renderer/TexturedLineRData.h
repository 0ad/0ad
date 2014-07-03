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
#include "graphics/ShaderProgramPtr.h"
#include "graphics/TextureManager.h"
#include "renderer/VertexBufferManager.h"

/**
 * Rendering data for an STexturedOverlayLine.
 * 
 * Note that instances may be shared amongst multiple copies of the same STexturedOverlayLine instance.
 * The reason is that this rendering data is non-copyable, but we do wish to maintain copyability of
 * SOverlayTexturedLineData to not limit its usage patterns too much (particularly the practice of storing
 * them into containers).
 * 
 * For this reason, instead of storing a reverse pointer back to any single SOverlayTexturedLine, the methods
 * in this class accept references to STexturedOverlayLines to work with. It is up to client code to pass in
 * SOverlayTexturedLines to all methods that are consistently the same instance or non-modified copies of it.
 */
class CTexturedLineRData : public CRenderData
{
	// we hold raw pointers to vertex buffer chunks that are handed out by the vertex buffer manager
	// and can not be safely duplicated by us.
	NONCOPYABLE(CTexturedLineRData);

public:

	CTexturedLineRData() : m_VB(NULL), m_VBIndices(NULL) { }

	~CTexturedLineRData()
	{
		if (m_VB)
			g_VBMan.Release(m_VB);
		if (m_VBIndices)
			g_VBMan.Release(m_VBIndices);
	}

	void Update(const SOverlayTexturedLine& line);
	void Render(const SOverlayTexturedLine& line, const CShaderProgramPtr& shader);

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
	void CreateLineCap(const SOverlayTexturedLine& line, const CVector3D& corner1, const CVector3D& corner2, const CVector3D& normal,
		               SOverlayTexturedLine::LineCapType endCapType, std::vector<SVertex>& verticesOut, std::vector<u16>& indicesOut);

	/// Small utility function; grabs the centroid of the positions of two vertices
	inline CVector3D Centroid(const SVertex& v1, const SVertex& v2)
	{
		return (v1.m_Position + v2.m_Position) * 0.5;
	}

	CVertexBuffer::VBChunk* m_VB;
	CVertexBuffer::VBChunk* m_VBIndices;
};

#endif // INCLUDED_TEXTUREDLINERDATA
