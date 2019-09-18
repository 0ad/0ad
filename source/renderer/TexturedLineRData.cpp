/* Copyright (C) 2017 Wildfire Games.
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

#include "precompiled.h"

#include "TexturedLineRData.h"

#include "graphics/Terrain.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "renderer/OverlayRenderer.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/SimContext.h"
#include "simulation2/components/ICmpWaterManager.h"

/* Note: this implementation uses g_VBMan directly rather than access it through the nicer VertexArray interface,
 * because it allows you to work with variable amounts of vertices and indices more easily. New code should prefer
 * to use VertexArray where possible, though. */

void CTexturedLineRData::Render(const SOverlayTexturedLine& line, const CShaderProgramPtr& shader)
{
	if (!m_VB || !m_VBIndices)
		return; // might have failed to allocate

	// -- render main line quad strip ----------------------

	const int streamFlags = shader->GetStreamFlags();

	shader->BindTexture(str_baseTex, line.m_TextureBase->GetHandle());
	shader->BindTexture(str_maskTex, line.m_TextureMask->GetHandle());
	shader->Uniform(str_objectColor, line.m_Color);

	GLsizei stride = sizeof(CTexturedLineRData::SVertex);
	CTexturedLineRData::SVertex* vertexBase = reinterpret_cast<CTexturedLineRData::SVertex*>(m_VB->m_Owner->Bind());

	if (streamFlags & STREAM_POS)
		shader->VertexPointer(3, GL_FLOAT, stride, &vertexBase->m_Position[0]);

	if (streamFlags & STREAM_UV0)
		shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, &vertexBase->m_UVs[0]);

	if (streamFlags & STREAM_UV1)
		shader->TexCoordPointer(GL_TEXTURE1, 2, GL_FLOAT, stride, &vertexBase->m_UVs[0]);

	u8* indexBase = m_VBIndices->m_Owner->Bind();

	shader->AssertPointersBound();
	glDrawElements(GL_TRIANGLES, m_VBIndices->m_Count, GL_UNSIGNED_SHORT, indexBase + sizeof(u16)*m_VBIndices->m_Index);

	g_Renderer.GetStats().m_DrawCalls++;
	g_Renderer.GetStats().m_OverlayTris += m_VBIndices->m_Count/3;
}

void CTexturedLineRData::Update(const SOverlayTexturedLine& line)
{
	if (m_VB)
	{
		g_VBMan.Release(m_VB);
		m_VB = NULL;
	}
	if (m_VBIndices)
	{
		g_VBMan.Release(m_VBIndices);
		m_VBIndices = NULL;
	}

	if (!line.m_SimContext)
	{
		debug_warn(L"[TexturedLineRData] No SimContext set for textured overlay line, cannot render (no terrain data)");
		return;
	}

	float v = 0.f;
	std::vector<SVertex> vertices;
	std::vector<u16> indices;

	size_t n = line.m_Coords.size() / 2; // number of line points
	bool closed = line.m_Closed;

	ENSURE(n >= 2); // minimum needed to avoid errors (also minimum value to make sense, can't draw a line between 1 point)

	// In each iteration, p1 is the position of vertex i, p0 is i-1, p2 is i+1.
	// To avoid slightly expensive terrain computations we cycle these around and
	// recompute p2 at the end of each iteration.

	CVector3D p0;
	CVector3D p1(line.m_Coords[0], 0, line.m_Coords[1]);
	CVector3D p2(line.m_Coords[2], 0, line.m_Coords[3]);

	if (closed)
		// grab the ending point so as to close the loop
		p0 = CVector3D(line.m_Coords[(n-1)*2], 0, line.m_Coords[(n-1)*2+1]);
	else
		// we don't want to loop around and use the direction towards the other end of the line, so create an artificial p0 that
		// extends the p2 -> p1 direction, and use that point instead
		p0 = p1 + (p1 - p2);

	bool p1floating = false;
	bool p2floating = false;

	// Compute terrain heights, clamped to the water height (and remember whether
	// each point was floating on water, for normal computation later)

	// TODO: if we ever support more than one water level per map, recompute this per point
	CmpPtr<ICmpWaterManager> cmpWaterManager(*line.m_SimContext, SYSTEM_ENTITY);
	float w = cmpWaterManager ? cmpWaterManager->GetExactWaterLevel(p0.X, p0.Z) : 0.f;

	const CTerrain& terrain = line.m_SimContext->GetTerrain();

	p0.Y = terrain.GetExactGroundLevel(p0.X, p0.Z);
	if (p0.Y < w)
		p0.Y = w;

	p1.Y = terrain.GetExactGroundLevel(p1.X, p1.Z);
	if (p1.Y < w)
	{
		p1.Y = w;
		p1floating = true;
	}

	p2.Y = terrain.GetExactGroundLevel(p2.X, p2.Z);
	if (p2.Y < w)
	{
		p2.Y = w;
		p2floating = true;
	}

	for (size_t i = 0; i < n; ++i)
	{
		// For vertex i, compute bisector of lines (i-1)..(i) and (i)..(i+1)
		// perpendicular to terrain normal

		// Normal is vertical if on water, else computed from terrain
		CVector3D norm;
		if (p1floating)
			norm = CVector3D(0, 1, 0);
		else
			norm = terrain.CalcExactNormal(p1.X, p1.Z);

		CVector3D b = ((p1 - p0).Normalized() + (p2 - p1).Normalized()).Cross(norm);

		// Adjust bisector length to match the line thickness, along the line's width
		float l = b.Dot((p2 - p1).Normalized().Cross(norm));
		if (fabs(l) > 0.000001f) // avoid unlikely divide-by-zero
			b *= line.m_Thickness / l;

		// Push vertices and indices for each quad in GL_TRIANGLES order. The two triangles of each quad are indexed using
		// the winding orders (BR, BL, TR) and (TR, BL, TL) (where BR is bottom-right of this iteration's quad, TR top-right etc).
		SVertex vertex1(p1 + b + norm*OverlayRenderer::OVERLAY_VOFFSET, 0.f, v);
		SVertex vertex2(p1 - b + norm*OverlayRenderer::OVERLAY_VOFFSET, 1.f, v);
		vertices.push_back(vertex1);
		vertices.push_back(vertex2);

		u16 index1 = vertices.size() - 2; // index of vertex1 in this iteration (TR of this quad)
		u16 index2 = vertices.size() - 1; // index of the vertex2 in this iteration (TL of this quad)

		if (i == 0)
		{
			// initial two vertices to continue building triangles from (n must be >= 2 for this to work)
			indices.push_back(index1);
			indices.push_back(index2);
		}
		else
		{
			u16 index1Prev = vertices.size() - 4; // index of the vertex1 in the previous iteration (BR of this quad)
			u16 index2Prev = vertices.size() - 3; // index of the vertex2 in the previous iteration (BL of this quad)
			ENSURE(index1Prev < vertices.size());
			ENSURE(index2Prev < vertices.size());
			// Add two corner points from last iteration and join with one of our own corners to create triangle 1
			// (don't need to do this if i == 1 because i == 0 are the first two ones, they don't need to be copied)
			if (i > 1)
			{
				indices.push_back(index1Prev);
				indices.push_back(index2Prev);
			}
			indices.push_back(index1); // complete triangle 1

			// create triangle 2, specifying the adjacent side's vertices in the opposite order from triangle 1
			indices.push_back(index1);
			indices.push_back(index2Prev);
			indices.push_back(index2);
		}

		// alternate V coordinate for debugging
		v = 1 - v;

		// cycle the p's and compute the new p2
		p0 = p1;
		p1 = p2;
		p1floating = p2floating;

		// if in closed mode, wrap around the coordinate array for p2 -- otherwise, extend linearly
		if (!closed && i == n-2)
			// next iteration is the last point of the line, so create an artificial p2 that extends the p0 -> p1 direction
			p2 = p1 + (p1 - p0);
		else
			p2 = CVector3D(line.m_Coords[((i+2) % n)*2], 0, line.m_Coords[((i+2) % n)*2+1]);

		p2.Y = terrain.GetExactGroundLevel(p2.X, p2.Z);
		if (p2.Y < w)
		{
			p2.Y = w;
			p2floating = true;
		}
		else
			p2floating = false;
	}

	if (closed)
	{
		// close the path
		if (n % 2 == 0)
		{
			indices.push_back(vertices.size()-2);
			indices.push_back(vertices.size()-1);
			indices.push_back(0);

			indices.push_back(0);
			indices.push_back(vertices.size()-1);
			indices.push_back(1);
		}
		else
		{
			// add two vertices to have the good UVs for the last quad
			SVertex vertex1(vertices[0].m_Position, 0.f, 1.f);
			SVertex vertex2(vertices[1].m_Position, 1.f, 1.f);
			vertices.push_back(vertex1);
			vertices.push_back(vertex2);

			indices.push_back(vertices.size()-4);
			indices.push_back(vertices.size()-3);
			indices.push_back(vertices.size()-2);

			indices.push_back(vertices.size()-2);
			indices.push_back(vertices.size()-3);
			indices.push_back(vertices.size()-1);
		}
	}
	else
	{
		// Create start and end caps. On either end, this is done by taking the centroid between the last and second-to-last pair of
		// vertices that was generated along the path (i.e. the vertex1's and vertex2's from above), taking a directional vector
		// between them, and drawing the line cap in the plane given by the two butt-end corner points plus said vector.
		std::vector<u16> capIndices;
		std::vector<SVertex> capVertices;

		// create end cap
		CreateLineCap(
			line,
			// the order of these vertices is important here, swapping them produces caps at the wrong side
			vertices[vertices.size()-2].m_Position, // top-right vertex of last quad
			vertices[vertices.size()-1].m_Position, // top-left vertex of last quad
			// directional vector between centroids of last vertex pair and second-to-last vertex pair
			(Centroid(vertices[vertices.size()-2], vertices[vertices.size()-1]) - Centroid(vertices[vertices.size()-4], vertices[vertices.size()-3])).Normalized(),
			line.m_EndCapType,
			capVertices,
			capIndices
		);

		for (unsigned i = 0; i < capIndices.size(); i++)
			capIndices[i] += vertices.size();

		vertices.insert(vertices.end(), capVertices.begin(), capVertices.end());
		indices.insert(indices.end(), capIndices.begin(), capIndices.end());

		capIndices.clear();
		capVertices.clear();

		// create start cap
		CreateLineCap(
			line,
			// the order of these vertices is important here, swapping them produces caps at the wrong side
			vertices[1].m_Position,
			vertices[0].m_Position,
			// directional vector between centroids of first vertex pair and second vertex pair
			(Centroid(vertices[1], vertices[0]) - Centroid(vertices[3], vertices[2])).Normalized(),
			line.m_StartCapType,
			capVertices,
			capIndices
		);

		for (unsigned i = 0; i < capIndices.size(); i++)
			capIndices[i] += vertices.size();

		vertices.insert(vertices.end(), capVertices.begin(), capVertices.end());
		indices.insert(indices.end(), capIndices.begin(), capIndices.end());
	}

	ENSURE(indices.size() % 3 == 0); // GL_TRIANGLES indices, so must be multiple of 3

	m_VB = g_VBMan.Allocate(sizeof(SVertex), vertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
	if (m_VB) // allocation might fail (e.g. due to too many vertices)
	{
		m_VB->m_Owner->UpdateChunkVertices(m_VB, &vertices[0]); // copy data into VBO

		for (size_t k = 0; k < indices.size(); ++k)
			indices[k] += m_VB->m_Index;

		m_VBIndices = g_VBMan.Allocate(sizeof(u16), indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
		if (m_VBIndices)
			m_VBIndices->m_Owner->UpdateChunkVertices(m_VBIndices, &indices[0]);
	}

}

void CTexturedLineRData::CreateLineCap(const SOverlayTexturedLine& line, const CVector3D& corner1, const CVector3D& corner2,
	const CVector3D& lineDirectionNormal, SOverlayTexturedLine::LineCapType endCapType, std::vector<SVertex>& verticesOut,
	std::vector<u16>& indicesOut)
{
	if (endCapType == SOverlayTexturedLine::LINECAP_FLAT)
		return; // no action needed, this is the default

	// When not in closed mode, we've created artificial points for the start- and endpoints that extend the line in the
	// direction of the first and the last segment, respectively. Thus, we know both the start and endpoints have perpendicular
	// butt endings, i.e. the end corner vertices on either side of the line extend perpendicularly from the segment direction.
	// That is to say, when viewed from the top, we will have something like
	//                                                 .
	//  this:                     and not like this:  /|
	//         ----+                                 / |
	//             |                                /  .
	//             |                                  /
	//         ----+                                 /
	//

	int roundCapPoints = 8; // amount of points to sample along the semicircle for rounded caps (including corner points)
	float radius = line.m_Thickness;

	CVector3D centerPoint = (corner1 + corner2) * 0.5f;
	SVertex centerVertex(centerPoint, 0.5f, 0.5f);
	u16 indexOffset = verticesOut.size(); // index offset in verticesOut from where we start adding our vertices

	switch (endCapType)
	{
	case SOverlayTexturedLine::LINECAP_SHARP:
		{
			roundCapPoints = 3; // creates only one point directly ahead
			radius *= 1.5f; // make it a bit sharper (note that we don't use the radius for the butt-end corner points so it should be ok)
			centerVertex.m_UVs[0] = 0.480f; // slight visual correction to make the texture match up better at the corner points
		}
		FALLTHROUGH;
	case SOverlayTexturedLine::LINECAP_ROUND:
		{
			// Draw a rounded line cap in the 3D plane of the line specified by the two corner points and the normal vector of the
			// line's direction. The terrain normal at the centroid between the two corner points is perpendicular to this plane.
			// The way this works is by taking a vector from the corner points' centroid to one of the corner points (which is then
			// of radius length), and rotate it around the terrain normal vector in that centroid. This will rotate the vector in
			// the line's plane, producing the desired rounded cap.

			// To please OpenGL's winding order, this angle needs to be negated depending on whether we start rotating from
			// the (center -> corner1) or (center -> corner2) vector. For the (center -> corner2) vector, we apparently need to use
			// the negated angle.
			float stepAngle = -(float)(M_PI/(roundCapPoints-1));

			// Push the vertices in triangle fan order (easy to generate GL_TRIANGLES indices for afterwards)
			// Note that we're manually adding the corner vertices instead of having them be generated by the rotating vector.
			// This is because we want to support an overly large radius to make the sharp line ending look sharper.
			verticesOut.push_back(centerVertex);
			verticesOut.push_back(SVertex(corner2, 0.f, 0.f));

			// Get the base vector that we will incrementally rotate in the cap plane to produce the radial sample points.
			// Normally corner2 - centerPoint would suffice for this since it is of radius length, but we want to support custom
			// radii to support tuning the 'sharpness' of sharp end caps (see above)
			CVector3D rotationBaseVector = (corner2 - centerPoint).Normalized() * radius;
			// Calculate the normal vector of the plane in which we're going to be drawing the line cap. This is the vector that
			// is perpendicular to both baseVector and the 'lineDirectionNormal' vector indicating the direction of the line.
			// Note that we shouldn't use terrain->CalcExactNormal() here because if the line is being rendered on top of water,
			// then CalcExactNormal will return the normal vector of the terrain that's underwater (which can be quite funky).
			CVector3D capPlaneNormal = lineDirectionNormal.Cross(rotationBaseVector).Normalized();

			for (int i = 1; i < roundCapPoints - 1; ++i)
			{
				// Rotate the centerPoint -> corner vector by i*stepAngle radians around the cap plane normal at the center point.
				CQuaternion quatRotation;
				quatRotation.FromAxisAngle(capPlaneNormal, i * stepAngle);
				CVector3D worldPos3D = centerPoint + quatRotation.Rotate(rotationBaseVector);

				// Let v range from 0 to 1 as we move along the semi-circle, keep u fixed at 0 (i.e. curve the left vertical edge
				// of the texture around the edge of the semicircle)
				float u = 0.f;
				float v = Clamp((i / static_cast<float>(roundCapPoints - 1)), 0.f, 1.f); // pos, u, v
				verticesOut.push_back(SVertex(worldPos3D, u, v));
			}

			// connect back to the other butt-end corner point to complete the semicircle
			verticesOut.push_back(SVertex(corner1, 0.f, 1.f));

			// now push indices in GL_TRIANGLES order; vertices[indexOffset] is the center vertex, vertices[indexOffset + 1] is the
			// first corner point, then a bunch of radial samples, and then at the end we have the other corner point again. So:
			for (int i=1; i < roundCapPoints; ++i)
			{
				indicesOut.push_back(indexOffset); // center vertex
				indicesOut.push_back(indexOffset + i);
				indicesOut.push_back(indexOffset + i + 1);
			}
		}
		break;

	case SOverlayTexturedLine::LINECAP_SQUARE:
		{
			// Extend the (corner1 -> corner2) vector along the direction normal and draw a square line ending consisting of
			// three triangles (sort of like a triangle fan)
			// NOTE: The order in which the vertices are pushed out determines the visibility, as they
			// are rendered only one-sided; the wrong order of vertices will make the cap visible only from the bottom.
			verticesOut.push_back(centerVertex);
			verticesOut.push_back(SVertex(corner2, 0.f, 0.f));
			verticesOut.push_back(SVertex(corner2 + (lineDirectionNormal * (line.m_Thickness)), 0.f, 0.33333f)); // extend butt corner point 2 along the normal vector
			verticesOut.push_back(SVertex(corner1 + (lineDirectionNormal * (line.m_Thickness)), 0.f, 0.66666f)); // extend butt corner point 1 along the normal vector
			verticesOut.push_back(SVertex(corner1, 0.f, 1.0f)); // push butt corner point 1

			for (int i=1; i < 4; ++i)
			{
				indicesOut.push_back(indexOffset); // center point
				indicesOut.push_back(indexOffset + i);
				indicesOut.push_back(indexOffset + i + 1);
			}
		}
		break;

	default:
		break;
	}

}
