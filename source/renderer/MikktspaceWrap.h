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

#ifndef INCLUDED_MIKKWRAP
#define INCLUDED_MIKKWRAP


#include "third_party/mikktspace/mikktspace.h"

class MikkTSpace
{

public:

	MikkTSpace(const CModelDefPtr& m, std::vector<float>& v, bool gpuSkinning);

	void generate();

private:

	SMikkTSpaceInterface m_Interface;
	SMikkTSpaceContext m_Context;

	const CModelDefPtr& m_Model;

	std::vector<float>& m_NewVertices;
	bool m_GpuSkinning;


	// Returns the number of faces (triangles/quads) on the mesh to be processed.
	static int getNumFaces(const SMikkTSpaceContext *pContext);


	// Returns the number of vertices on face number iFace
	// iFace is a number in the range {0, 1, ..., getNumFaces()-1}
	static int getNumVerticesOfFace(const SMikkTSpaceContext *pContext, const int iFace);


	// returns the position/normal/texcoord of the referenced face of vertex number iVert.
	// iVert is in the range {0,1,2} for triangles and {0,1,2,3} for quads.
	static void getPosition(const SMikkTSpaceContext *pContext,
			float fvPosOut[], const int iFace, const int iVert);

	static void getNormal(const SMikkTSpaceContext *pContext,
			float fvNormOut[], const int iFace, const int iVert);

	static void getTexCoord(const SMikkTSpaceContext *pContext,
			float fvTexcOut[], const int iFace, const int iVert);


	// This function is used to return tangent space results to the application.
	// fvTangent and fvBiTangent are unit length vectors and fMagS and fMagT are their
	// true magnitudes which can be used for relief mapping effects.
	// fvBiTangent is the "real" bitangent and thus may not be perpendicular to fvTangent.
	// However, both are perpendicular to the vertex normal.
	// For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
	// fSign = bIsOrientationPreserving ? 1.0f : (-1.0f);
	// bitangent = fSign * cross(vN, tangent);
	static void setTSpace(const SMikkTSpaceContext * pContext, const float fvTangent[],
			const float fvBiTangent[], const float fMagS, const float fMagT,
			const tbool bIsOrientationPreserving, const int iFace, const int iVert);


};


#endif // INCLUDED_MIKKWRAP
