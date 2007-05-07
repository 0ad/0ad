/**
 * =========================================================================
 * File        : Brush.h
 * Project     : Pyrogenesis
 * Description : Implementation of CBrush, a class representing a convex object
 * =========================================================================
 */

#include "precompiled.h"

#include "lib/ogl.h"

#include <float.h>

#include "Brush.h"
#include "Bound.h"
#include "graphics/Frustum.h"


///////////////////////////////////////////////////////////////////////////////
// Convert the given bounds into a brush
CBrush::CBrush(const CBound& bounds)
{
	m_Vertices.resize(8);

	for(uint i = 0; i < 8; ++i)
	{
		m_Vertices[i][0] = bounds[(i & 1) ? 1 : 0][0];
		m_Vertices[i][1] = bounds[(i & 2) ? 1 : 0][1];
		m_Vertices[i][2] = bounds[(i & 4) ? 1 : 0][2];
	}

	m_Faces.resize(30);

	m_Faces[0] = 0; m_Faces[1] = 1; m_Faces[2] = 3; m_Faces[3] = 2; m_Faces[4] = 0; // Z = min
	m_Faces[5] = 4; m_Faces[6] = 5; m_Faces[7] = 7; m_Faces[8] = 6; m_Faces[9] = 4; // Z = max

	m_Faces[10] = 0; m_Faces[11] = 2; m_Faces[12] = 6; m_Faces[13] = 4; m_Faces[14] = 0; // X = min
	m_Faces[15] = 1; m_Faces[16] = 3; m_Faces[17] = 7; m_Faces[18] = 5; m_Faces[19] = 1; // X = max

	m_Faces[20] = 0; m_Faces[21] = 1; m_Faces[22] = 5; m_Faces[23] = 4; m_Faces[24] = 0; // Y = min
	m_Faces[25] = 2; m_Faces[26] = 3; m_Faces[27] = 7; m_Faces[28] = 6; m_Faces[29] = 2; // Y = max
}


///////////////////////////////////////////////////////////////////////////////
// Calculate bounds of this brush
void CBrush::Bounds(CBound& result) const
{
	result.SetEmpty();

	for(uint i = 0; i < m_Vertices.size(); ++i)
		result += m_Vertices[i];
}


///////////////////////////////////////////////////////////////////////////////
// Cut the brush according to a given plane
struct SliceVertexInfo {
	float d; // distance
	uint res; // index in result brush (or no_vertex if cut away)
};

struct NewVertexInfo {
	uint v1, v2; // adjacent vertices in original brush
	uint res; // index in result brush

	uint neighb1, neighb2; // index into newv
};

struct SliceInfo {
	std::vector<SliceVertexInfo> v;
	std::vector<NewVertexInfo> newv;
	uint thisFaceNewVertex; // index into newv
	const CBrush* original;
	CBrush* result;
};

struct CBrush::Helper
{
	static uint SliceNewVertex(SliceInfo& si, uint v1, uint v2);
};

// create a new vertex between the given two vertices (index into original brush)
// returns the index of the new vertex in the resulting brush
uint CBrush::Helper::SliceNewVertex(SliceInfo& si, uint v1, uint v2)
{
	uint idx;

	for(idx = 0; idx < si.newv.size(); ++idx)
	{
		if ((si.newv[idx].v1 == v1 && si.newv[idx].v2 == v2) ||
		    (si.newv[idx].v1 == v2 && si.newv[idx].v2 == v1))
			break;
	}

	if (idx >= si.newv.size())
	{
		NewVertexInfo nvi;
		CVector3D newpos;
		float inv = 1.0 / (si.v[v1].d - si.v[v2].d);

		newpos = si.original->m_Vertices[v2]*(si.v[v1].d*inv) +
			 si.original->m_Vertices[v1]*(-si.v[v2].d*inv);

		nvi.v1 = v1;
		nvi.v2 = v2;
		nvi.res = (uint)si.result->m_Vertices.size();
		nvi.neighb1 = no_vertex;
		nvi.neighb2 = no_vertex;
		si.result->m_Vertices.push_back(newpos);
		si.newv.push_back(nvi);
	}

	if (si.thisFaceNewVertex != no_vertex)
	{
		if (si.newv[si.thisFaceNewVertex].neighb1 == no_vertex)
			si.newv[si.thisFaceNewVertex].neighb1 = idx;
		else
			si.newv[si.thisFaceNewVertex].neighb2 = idx;

		if (si.newv[idx].neighb1 == no_vertex)
			si.newv[idx].neighb1 = si.thisFaceNewVertex;
		else
			si.newv[idx].neighb2 = si.thisFaceNewVertex;

		si.thisFaceNewVertex = no_vertex;
	}
	else
	{
		si.thisFaceNewVertex = idx;
	}

	return si.newv[idx].res;
}

void CBrush::Slice(const CPlane& plane, CBrush& result) const
{
	debug_assert(&result != this);

	SliceInfo si;

	si.original = this;
	si.result = &result;
	si.thisFaceNewVertex = no_vertex;
	si.newv.reserve(m_Vertices.size() / 2);

	result.m_Vertices.resize(0); // clear any left-overs
	result.m_Faces.resize(0);
	result.m_Vertices.reserve(m_Vertices.size() + 2);
	result.m_Faces.reserve(m_Faces.size() + 5);

	// Classify and copy vertices
	si.v.resize(m_Vertices.size());

	for(uint i = 0; i < m_Vertices.size(); ++i)
	{
		si.v[i].d = plane.DistanceToPlane(m_Vertices[i]);
		if (si.v[i].d >= 0.0)
		{
			si.v[i].res = (uint)result.m_Vertices.size();
			result.m_Vertices.push_back(m_Vertices[i]);
		}
		else
		{
			si.v[i].res = no_vertex;
		}
	}

	// Transfer faces
	uint firstInFace = no_vertex; // in original brush
	uint startInResultFaceArray = ~0u;

	for(uint i = 0; i < m_Faces.size(); ++i)
	{
		if (firstInFace == no_vertex)
		{
			debug_assert(si.thisFaceNewVertex == no_vertex);

			firstInFace = m_Faces[i];
			startInResultFaceArray = (uint)result.m_Faces.size();
			continue;
		}

		uint prev = m_Faces[i-1];
		uint cur = m_Faces[i];

		if (si.v[prev].res == no_vertex)
		{
			if (si.v[cur].res != no_vertex)
			{
				// re-entering the front side of the plane
				result.m_Faces.push_back(Helper::SliceNewVertex(si, prev, cur));
				result.m_Faces.push_back(si.v[cur].res);
			}
		}
		else
		{
			if (si.v[cur].res != no_vertex)
			{
				// perfectly normal edge
				result.m_Faces.push_back(si.v[cur].res);
			}
			else
			{
				// leaving the front side of the plane
				result.m_Faces.push_back(Helper::SliceNewVertex(si, prev, cur));
			}
		}

		if (cur == firstInFace)
		{
			if (result.m_Faces.size() > startInResultFaceArray)
				result.m_Faces.push_back(result.m_Faces[startInResultFaceArray]);
			firstInFace = no_vertex; // start a new face
		}
	}

	debug_assert(firstInFace == no_vertex);

	// Create the face that lies in the slicing plane
	if (si.newv.size())
	{
		uint prev = 0;
		uint idx;

		result.m_Faces.push_back(si.newv[0].res);
		idx = si.newv[0].neighb2;
		si.newv[0].neighb2 = no_vertex;

		while(idx != 0)
		{
			debug_assert(idx < si.newv.size());

			if (si.newv[idx].neighb1 == prev)
			{
				si.newv[idx].neighb1 = si.newv[idx].neighb2;
				si.newv[idx].neighb2 = no_vertex;
			}
			else
			{
				debug_assert(si.newv[idx].neighb2 == prev);

				si.newv[idx].neighb2 = no_vertex;
			}

			result.m_Faces.push_back(si.newv[idx].res);

			prev = idx;
			idx = si.newv[idx].neighb1;
			si.newv[prev].neighb1 = no_vertex;
		}

		result.m_Faces.push_back(si.newv[0].res);
	}
}


///////////////////////////////////////////////////////////////////////////////
// Intersect with frustum by repeated slicing
void CBrush::Intersect(const CFrustum& frustum, CBrush& result) const
{
	debug_assert(&result != this);

	if (!frustum.GetNumPlanes())
	{
		result = *this;
		return;
	}

	CBrush buf;
	const CBrush* prev = this;
	CBrush* next;

	if (frustum.GetNumPlanes() & 1)
		next = &result;
	else
		next = &buf;

	for(uint i = 0; i < frustum.GetNumPlanes(); ++i)
	{
		prev->Slice(frustum[i], *next);
		prev = next;
		if (prev == &buf)
			next = &result;
		else
			next = &buf;
	}

	debug_assert(prev == &result);
}


///////////////////////////////////////////////////////////////////////////////
// Dump the faces to OpenGL
void CBrush::Render() const
{
	uint firstInFace = no_vertex;

	for(uint i = 0; i < m_Faces.size(); ++i)
	{
		if (firstInFace == no_vertex)
		{
			glBegin(GL_POLYGON);
			firstInFace = m_Faces[i];
			continue;
		}

		const CVector3D& vertex = m_Vertices[m_Faces[i]];

		glVertex3fv(&vertex.X);

		if (firstInFace == m_Faces[i])
		{
			glEnd();
			firstInFace = no_vertex;
		}
	}

	debug_assert(firstInFace == no_vertex);
}


