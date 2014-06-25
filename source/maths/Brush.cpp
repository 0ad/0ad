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

/*
 * Implementation of CBrush, a class representing a convex object
 */

#include "precompiled.h"

#include "lib/ogl.h"

#include <float.h>

#include "Brush.h"
#include "BoundingBoxAligned.h"
#include "graphics/Frustum.h"


///////////////////////////////////////////////////////////////////////////////
// Convert the given bounds into a brush
CBrush::CBrush(const CBoundingBoxAligned& bounds)
{
	m_Vertices.resize(8);

	for(size_t i = 0; i < 8; ++i)
	{
		m_Vertices[i][0] = bounds[(i & 1) ? 1 : 0][0]; // X
		m_Vertices[i][1] = bounds[(i & 2) ? 1 : 0][1]; // Y
		m_Vertices[i][2] = bounds[(i & 4) ? 1 : 0][2]; // Z
	}

	// construct cube face indices, 5 vertex indices per face (start vertex included twice)

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
void CBrush::Bounds(CBoundingBoxAligned& result) const
{
	result.SetEmpty();

	for(size_t i = 0; i < m_Vertices.size(); ++i)
		result += m_Vertices[i];
}


///////////////////////////////////////////////////////////////////////////////
// Cut the brush according to a given plane

/// Holds information about what happens to a single vertex in a brush during a slicing operation.
struct SliceOpVertexInfo
{
	float planeDist;    ///< Signed distance from this vertex to the slicing plane.
	size_t resIdx;      ///< Index of this vertex in the resulting brush (or NO_VERTEX if cut away)
};

/// Holds information about a newly introduced vertex on an edge in a brush as the result of a slicing operation.
struct SliceOpNewVertexInfo
{
	/// Indices of adjacent edge vertices in original brush
	size_t edgeIdx1, edgeIdx2;
	/// Index of newly introduced vertex in resulting brush
	size_t resIdx;
	
	/**
	 * Index into SliceOpInfo.nvInfo; hold the indices of this new vertex's direct neighbours in the slicing plane face,
	 * with no consistent winding direction around the face for either field (e.g., the neighb1 of X can point back to
	 * X with either its neighb1 or neighb2).
	 */
	size_t neighbIdx1, neighbIdx2;
};

/// Holds support information during a CBrush/CPlane slicing operation.
struct SliceOpInfo
{
	CBrush* result;
	const CBrush* original;

	/**
	 * Holds information about what happens to each vertex in the original brush after the slice operation.
	 * Same size as m_Vertices of the brush getting sliced.
	 */
	std::vector<SliceOpVertexInfo> ovInfo;
	
	/// Holds information about newly inserted vertices during a slice operation.
	std::vector<SliceOpNewVertexInfo> nvInfo;
	
	/**
	 * Indices into nvInfo; during the execution of the slicing algorithm, holds the previously inserted new vertex on 
	 * one of the edges of the face that's currently being evaluated for slice points, or NO_VERTEX if no such vertex 
	 * exists.
	 */
	size_t thisFaceNewVertexIdx;
};

struct CBrush::Helper
{
	/**
	 * Creates a new vertex between the given two vertices (indexed into the original brush).
	 * Returns the index of the new vertex in the resulting brush.
	 */
	static size_t SliceNewVertex(SliceOpInfo& sliceInfo, size_t v1, size_t v2);
};

size_t CBrush::Helper::SliceNewVertex(SliceOpInfo& sliceOp, size_t edgeIdx1, size_t edgeIdx2)
{
	// check if a new vertex has already been inserted on this edge
	size_t idx;
	for(idx = 0; idx < sliceOp.nvInfo.size(); ++idx)
	{
		if ((sliceOp.nvInfo[idx].edgeIdx1 == edgeIdx1 && sliceOp.nvInfo[idx].edgeIdx2 == edgeIdx2) ||
		    (sliceOp.nvInfo[idx].edgeIdx1 == edgeIdx2 && sliceOp.nvInfo[idx].edgeIdx2 == edgeIdx1))
			break;
	}

	if (idx >= sliceOp.nvInfo.size())
	{
		// no previously inserted new vertex found on this edge; insert a new one
		SliceOpNewVertexInfo nvi;
		CVector3D newPos;
		
		// interpolate between the two vertices based on their distance from the plane
		float inv = 1.0 / (sliceOp.ovInfo[edgeIdx1].planeDist - sliceOp.ovInfo[edgeIdx2].planeDist);
		newPos = sliceOp.original->m_Vertices[edgeIdx2] * ( sliceOp.ovInfo[edgeIdx1].planeDist * inv) +
		         sliceOp.original->m_Vertices[edgeIdx1] * (-sliceOp.ovInfo[edgeIdx2].planeDist * inv);

		nvi.edgeIdx1 = edgeIdx1;
		nvi.edgeIdx2 = edgeIdx2;
		nvi.resIdx = sliceOp.result->m_Vertices.size();
		nvi.neighbIdx1 = NO_VERTEX;
		nvi.neighbIdx2 = NO_VERTEX;

		sliceOp.result->m_Vertices.push_back(newPos);
		sliceOp.nvInfo.push_back(nvi);
	}

	// at this point, 'idx' is the index into nvInfo of the vertex inserted onto the edge

	if (sliceOp.thisFaceNewVertexIdx != NO_VERTEX)
	{
		// a vertex has been previously inserted onto another edge of this face; link them together as neighbours
		// (using whichever one of the neighbIdx1 or -2 links is still available)

		if (sliceOp.nvInfo[sliceOp.thisFaceNewVertexIdx].neighbIdx1 == NO_VERTEX)
			sliceOp.nvInfo[sliceOp.thisFaceNewVertexIdx].neighbIdx1 = idx;
		else
			sliceOp.nvInfo[sliceOp.thisFaceNewVertexIdx].neighbIdx2 = idx;

		if (sliceOp.nvInfo[idx].neighbIdx1 == NO_VERTEX)
			sliceOp.nvInfo[idx].neighbIdx1 = sliceOp.thisFaceNewVertexIdx;
		else
			sliceOp.nvInfo[idx].neighbIdx2 = sliceOp.thisFaceNewVertexIdx;

		// a plane should slice a face only in two locations, so reset for the next face
		sliceOp.thisFaceNewVertexIdx = NO_VERTEX;
	}
	else
	{
		// store the index of the inserted vertex on this edge, so that we can retrieve it when the plane slices
		// this face again in another edge
		sliceOp.thisFaceNewVertexIdx = idx;
	}

	return sliceOp.nvInfo[idx].resIdx;
}

void CBrush::Slice(const CPlane& plane, CBrush& result) const
{
	ENSURE(&result != this);

	SliceOpInfo sliceOp;

	sliceOp.original = this;
	sliceOp.result = &result;
	sliceOp.thisFaceNewVertexIdx = NO_VERTEX;
	sliceOp.ovInfo.resize(m_Vertices.size());
	sliceOp.nvInfo.reserve(m_Vertices.size() / 2);

	result.m_Vertices.resize(0); // clear any left-overs
	result.m_Faces.resize(0);
	result.m_Vertices.reserve(m_Vertices.size() + 2);
	result.m_Faces.reserve(m_Faces.size() + 5);

	// Copy vertices that weren't sliced away by the plane to the resulting brush.
	for(size_t i = 0; i < m_Vertices.size(); ++i)
	{
		const CVector3D& vtx = m_Vertices[i];            // current vertex
		SliceOpVertexInfo& vtxInfo = sliceOp.ovInfo[i];  // slicing operation info about current vertex

		vtxInfo.planeDist = plane.DistanceToPlane(vtx);
		if (vtxInfo.planeDist >= 0.0)
		{
			// positive side of the plane; not sliced away
			vtxInfo.resIdx = result.m_Vertices.size();
			result.m_Vertices.push_back(vtx);
		}
		else
		{
			// other side of the plane; sliced away
			vtxInfo.resIdx = NO_VERTEX;
		}
	}

	// Transfer faces. (Recall how faces are specified; see CBrush::m_Faces). The idea is to examine each face separately,
	// and see where its edges cross the slicing plane (meaning that exactly one of the vertices of that edge was cut away).
	// On those edges, new vertices are introduced where the edge intersects the plane, and the resulting brush's m_Faces 
	// array is updated to refer to the newly inserted vertices instead of the original one that got cut away.

	size_t currentFaceStartIdx = NO_VERTEX; // index of the first vertex of the current face in the original brush
	size_t resultFaceStartIdx = NO_VERTEX;  // index of the first vertex of the current face in the resulting brush

	for(size_t i = 0; i < m_Faces.size(); ++i)
	{
		if (currentFaceStartIdx == NO_VERTEX)
		{
			// starting a new face
			ENSURE(sliceOp.thisFaceNewVertexIdx == NO_VERTEX);

			currentFaceStartIdx = m_Faces[i];
			resultFaceStartIdx = result.m_Faces.size();
			continue;
		}

		size_t prevIdx = m_Faces[i-1];  // index of previous vertex in this face list
		size_t curIdx = m_Faces[i];     // index of current vertex in this face list

		if (sliceOp.ovInfo[prevIdx].resIdx == NO_VERTEX)
		{
			// previous face vertex got sliced away by the plane; see if the edge (prev,current) crosses the slicing plane
			if (sliceOp.ovInfo[curIdx].resIdx != NO_VERTEX)
			{
				// re-entering the front side of the plane; insert vertex on intersection of plane and (prev,current) edge
				result.m_Faces.push_back(Helper::SliceNewVertex(sliceOp, prevIdx, curIdx));
				result.m_Faces.push_back(sliceOp.ovInfo[curIdx].resIdx);
			}
		}
		else
		{
			// previous face vertex didn't get sliced away; see if the edge (prev,current) crosses the slicing plane
			if (sliceOp.ovInfo[curIdx].resIdx != NO_VERTEX)
			{
				// perfectly normal edge; doesn't cross the plane
				result.m_Faces.push_back(sliceOp.ovInfo[curIdx].resIdx);
			}
			else
			{
				// leaving the front side of the plane; insert vertex on intersection of plane and edge (prev, current)
				result.m_Faces.push_back(Helper::SliceNewVertex(sliceOp, prevIdx, curIdx));
			}
		}

		// if we're back at the first vertex of the current face, then we've completed the face
		if (curIdx == currentFaceStartIdx)
		{
			// close the index loop
			if (result.m_Faces.size() > resultFaceStartIdx)
				result.m_Faces.push_back(result.m_Faces[resultFaceStartIdx]);

			currentFaceStartIdx = NO_VERTEX; // start a new face
		}
	}

	ENSURE(currentFaceStartIdx == NO_VERTEX);

	// Create the face that lies in the slicing plane. Remember, all the intersections of the slicing plane with face
	// edges of the brush have been stored in sliceOp.nvInfo by the SliceNewVertex function, and refer to their direct 
	// neighbours in the slicing plane face using the neighbIdx1 and neighbIdx2 fields (in no consistent winding order).

	if (sliceOp.nvInfo.size())
	{
		// push the starting vertex
		result.m_Faces.push_back(sliceOp.nvInfo[0].resIdx);
		
		// At this point, there is no consistent winding order in the neighbX fields, so at each vertex we need to figure 
		// out whether neighb1 or neighb2 points 'onwards' along the face, according to an initially chosen winding direction.
		// (or, equivalently, which one points back to the one we were just at). At each vertex, we then set neighb1 to be the 
		// one to point onwards, deleting any pointers which we no longer need to complete the trace.

		size_t idx;
		size_t prev = 0;

		idx = sliceOp.nvInfo[0].neighbIdx2; // pick arbitrary starting direction
		sliceOp.nvInfo[0].neighbIdx2 = NO_VERTEX;

		while(idx != 0)
		{
			ENSURE(idx < sliceOp.nvInfo.size());
			if (idx >= sliceOp.nvInfo.size())
				break;

			if (sliceOp.nvInfo[idx].neighbIdx1 == prev)
			{
				// neighb1 is pointing the wrong way; we want to normalize it to point onwards in the direction
				// we initially chose, so swap it with neighb2 and delete neighb2 (no longer needed)
				sliceOp.nvInfo[idx].neighbIdx1 = sliceOp.nvInfo[idx].neighbIdx2;
				sliceOp.nvInfo[idx].neighbIdx2 = NO_VERTEX;
			}
			else
			{
				// neighb1 isn't pointing to the previous vertex, so neighb2 must be (otherwise a pair of vertices failed to
				// get paired properly during face/plane slicing).
				ENSURE(sliceOp.nvInfo[idx].neighbIdx2 == prev);
				sliceOp.nvInfo[idx].neighbIdx2 = NO_VERTEX;
			}

			result.m_Faces.push_back(sliceOp.nvInfo[idx].resIdx);

			// move to next vertex; neighb1 has been normalized to point onward
			prev = idx;
			idx = sliceOp.nvInfo[idx].neighbIdx1;
			sliceOp.nvInfo[prev].neighbIdx1 = NO_VERTEX; // no longer needed, we've moved on
		}

		// push starting vertex again to close the shape
		result.m_Faces.push_back(sliceOp.nvInfo[0].resIdx);
	}
}



///////////////////////////////////////////////////////////////////////////////
// Intersect with frustum by repeated slicing
void CBrush::Intersect(const CFrustum& frustum, CBrush& result) const
{
	ENSURE(&result != this);

	if (!frustum.GetNumPlanes())
	{
		result = *this;
		return;
	}

	CBrush buf;
	const CBrush* prev = this;
	CBrush* next;

	// Repeatedly slice this brush with each plane of the frustum, alternating between 'result' and 'buf' to 
	// save intermediate results. Set up the starting brush so that the final version always ends up in 'result'.

	if (frustum.GetNumPlanes() & 1)
		next = &result;
	else
		next = &buf;

	for(size_t i = 0; i < frustum.GetNumPlanes(); ++i)
	{
		prev->Slice(frustum[i], *next);
		prev = next;
		if (prev == &buf)
			next = &result;
		else
			next = &buf;
	}

	ENSURE(prev == &result);
}
std::vector<CVector3D> CBrush::GetVertices() const 
{
	return m_Vertices;
}

void CBrush::GetFaces(std::vector<std::vector<size_t> >& out) const
{
	// split the back-to-back faces into separate face vectors, so that they're in a 
	// user-friendlier format than the back-to-back vertex index array
	// i.e. split 'x--xy------yz----z' into 'x--x', 'y-------y', 'z---z'

	size_t faceStartIdx = 0;
	while (faceStartIdx < m_Faces.size())
	{
		// start new face
		std::vector<size_t> singleFace;
		singleFace.push_back(m_Faces[faceStartIdx]);

		// step over all the values in the face until we hit the starting value again (which closes the face)
		size_t j = faceStartIdx + 1;
		while (j < m_Faces.size() && m_Faces[j] != m_Faces[faceStartIdx])
		{
			singleFace.push_back(m_Faces[j]);
			j++;
		}

		// each face must be closed by the same value that started it
		ENSURE(m_Faces[faceStartIdx] == m_Faces[j]);

		singleFace.push_back(m_Faces[j]);
		out.push_back(singleFace);

		faceStartIdx = j + 1;
	}
}

void CBrush::Render(CShaderProgramPtr& shader) const
{
	std::vector<float> data;

	std::vector<std::vector<size_t> > faces;
	GetFaces(faces);

#define ADD_VERT(a) \
	STMT( \
		data.push_back(u); \
		data.push_back(v); \
		data.push_back(m_Vertices[faces[i][a]].X); \
		data.push_back(m_Vertices[faces[i][a]].Y); \
		data.push_back(m_Vertices[faces[i][a]].Z); \
	)

	for (size_t i = 0; i < faces.size(); ++i)
	{
		// Triangulate into (0,1,2), (0,2,3), ...
		for (size_t j = 1; j < faces[i].size() - 2; ++j)
		{
			float u = 0;
			float v = 0;
			ADD_VERT(0);
			ADD_VERT(j);
			ADD_VERT(j+1);
		}
	}

#undef ADD_VERT

	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 5*sizeof(float), &data[0]);
	shader->VertexPointer(3, GL_FLOAT, 5*sizeof(float), &data[2]);

	shader->AssertPointersBound();
	glDrawArrays(GL_TRIANGLES, 0, data.size() / 5);
}

void CBrush::RenderOutline(CShaderProgramPtr& shader) const
{
	std::vector<float> data;

	std::vector<std::vector<size_t> > faces;
	GetFaces(faces);

#define ADD_VERT(a) \
	STMT( \
		data.push_back(u); \
		data.push_back(v); \
		data.push_back(m_Vertices[faces[i][a]].X); \
		data.push_back(m_Vertices[faces[i][a]].Y); \
		data.push_back(m_Vertices[faces[i][a]].Z); \
	)

	for (size_t i = 0; i < faces.size(); ++i)
	{
		for (size_t j = 0; j < faces[i].size() - 1; ++j)
		{
			float u = 0;
			float v = 0;
			ADD_VERT(j);
			ADD_VERT(j+1);
		}
	}

#undef ADD_VERT

	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 5*sizeof(float), &data[0]);
	shader->VertexPointer(3, GL_FLOAT, 5*sizeof(float), &data[2]);

	shader->AssertPointersBound();
	glDrawArrays(GL_LINES, 0, data.size() / 5);
}
