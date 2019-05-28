/* Copyright (C) 2019 Wildfire Games.
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

#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "third_party/mikktspace/mikktspace.h"

#include <vector>

class CVector3D;
class MikkTSpace
{

public:

	MikkTSpace(const CModelDefPtr& m, std::vector<float>& v, bool gpuSkinning);

	void Generate();

private:

	SMikkTSpaceInterface m_Interface;

	SMikkTSpaceContext m_Context;

	const CModelDefPtr& m_Model;

	std::vector<float>& m_NewVertices;

	bool m_GpuSkinning;

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @returns - the number of faces (triangles/quads) on the mesh to be processed.
	 */
	static int GetNumFaces(const SMikkTSpaceContext *pContext);

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @param[in] iFace - Number in the range { 0, 1, ..., getNumFaces() - 1 }.
	 * @returns - the number of faces (triangles/quads) on the mesh to be processed.
	 */
	static int GetNumVerticesOfFace(const SMikkTSpaceContext *pContext, const int iFace);

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @returns - The MikkTSpace.
	 */
	static MikkTSpace* GetUserDataFromContext(const SMikkTSpaceContext *pContext);

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @param[in] iFace - Number in the range { 0, 1, ..., getNumFaces() - 1 }.
	 * @param[in] iVert - Number in the range { 0, 1, 2 } for triangles and { 0, 1, 2, 3 } for quads.
	 * @returns - The MikkTSpace.
	 */
	static SModelVertex GetVertex(const SMikkTSpaceContext *pContext, const int iFace, const int iVert);

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @param[out] fvPosOut - The array containing the face.
	 * @param[in] iFace - Number in the range { 0, 1, ..., getNumFaces() - 1 }.
	 * @param[in] iVert - Number in the range { 0, 1, 2 } for triangles and { 0, 1, 2, 3 } for quads.
	 */
	static void GetPosition(const SMikkTSpaceContext *pContext,
			float fvPosOut[3], const int iFace, const int iVert);

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @param[out] fvNormOut iVert - The array containing the normal.
	 * @param[in] iFace - Number in the range { 0, 1, ..., getNumFaces() - 1 }.
	 * @param[in] iVert - Number in the range { 0, 1, 2 } for triangles and { 0, 1, 2, 3 } for quads.
	 */
	static void GetNormal(const SMikkTSpaceContext *pContext,
			float fvNormOut[3], const int iFace, const int iVert);

	/**
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @param[out] fvTexcOut iVert - Array containing the UV.
	 * @param[in] iFace - Number in the range { 0, 1, ..., getNumFaces() - 1 }.
	 * @param[in] iVert - Number in the range { 0, 1, 2 } for triangles and { 0, 1, 2, 3 } for quads.
	 */
	static void GetTexCoord(const SMikkTSpaceContext *pContext,
			float fvTexcOut[2], const int iFace, const int iVert);

	/**
	 * @brief This function is used to return tangent space results to the application.
	 * For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
	 * fSign = bIsOrientationPreserving ? 1.0f : (-1.0f);
	 * bitangent = fSign * cross(vN, tangent);
	 * @param[in] pContext - Pointer to the MikkTSpace context.
	 * @param[in] fvTangent - fvTangent - The tangent vector.
	 * @param[in] fvBiTangent - The "real" bitangent vector. Not be perpendicular to fvTangent. However, both are perpendicular to the vertex normal.
	 * @param[in] fMagS - magniture of the fvTangent vector.
	 * @param[in] fMagT - magniture of the fvBiTangent vector.
	 * @param[in] bIsOrientationPreserving - Whether the orientation should be preserved.
	 * @param[in] iFace - Number in the range {0,1,2} for triangles and {0,1,2,3} for quads.
	 * @param[in] iVert - Array containing the position vector of the face.
	 */
	static void SetTSpace(const SMikkTSpaceContext * pContext, const float fvTangent[],
			const float UNUSED(fvBiTangent)[], const float UNUSED(fMagS), const float UNUSED(fMagT),
			const tbool bIsOrientationPreserving, const int iFace, const int iVert);

};


#endif // INCLUDED_MIKKWRAP
