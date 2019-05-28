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

#include "precompiled.h"

#include "renderer/MikktspaceWrap.h"

MikkTSpace::MikkTSpace(const CModelDefPtr& m, std::vector<float>& v, bool gpuSkinning) : m_Model(m),
			m_NewVertices(v), m_GpuSkinning(gpuSkinning)
{
	// ensure that m_NewVertices is empty
	m_NewVertices.clear();

	// set up SMikkTSpaceInterface struct
	m_Interface.m_getNumFaces = GetNumFaces;
	m_Interface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
	m_Interface.m_getPosition = GetPosition;
	m_Interface.m_getNormal = GetNormal;
	m_Interface.m_getTexCoord = GetTexCoord;
	m_Interface.m_setTSpaceBasic = nullptr;
	m_Interface.m_setTSpace = SetTSpace;

	// set up SMikkTSpaceContext struct
	m_Context.m_pInterface = &m_Interface;
	m_Context.m_pUserData = static_cast<void*>(this);
}

void MikkTSpace::Generate()
{
	genTangSpaceDefault(&m_Context);
}

int MikkTSpace::GetNumFaces(const SMikkTSpaceContext *pContext)
{
	return GetUserDataFromContext(pContext)->m_Model->GetNumFaces();
}

int MikkTSpace::GetNumVerticesOfFace(const SMikkTSpaceContext* UNUSED(pContext), const int UNUSED(iFace))
{
	return 3;
}

void MikkTSpace::GetPosition(const SMikkTSpaceContext *pContext,
		float fvPosOut[3], const int iFace, const int iVert)
{
	const CVector3D& position = GetVertex(pContext, iFace, iVert).m_Coords;

	fvPosOut[0] = position.X;
	fvPosOut[1] = position.Y;
	fvPosOut[2] = position.Z;
}


void MikkTSpace::GetNormal(const SMikkTSpaceContext *pContext,
		float fvNormOut[3], const int iFace, const int iVert)
{
	const CVector3D& normal = GetVertex(pContext, iFace, iVert).m_Norm;

	fvNormOut[0] = normal.X;
	fvNormOut[1] = normal.Y;
	fvNormOut[2] = normal.Z;
}


void MikkTSpace::GetTexCoord(const SMikkTSpaceContext *pContext,
		float fvTexcOut[2], const int iFace, const int iVert)
{
	const MikkTSpace* userData = GetUserDataFromContext(pContext);
	const SModelFace& face = userData->m_Model->GetFaces()[iFace];
	const SModelVertex& v = userData->m_Model->GetVertices()[face.m_Verts[iVert]];

	// The tangents are calculated according to the 'default' UV set
	fvTexcOut[0] = v.m_UVs[0];
	fvTexcOut[1] = 1.0 - v.m_UVs[1];
}


void MikkTSpace::SetTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[],
	const float UNUSED(fvBiTangent)[], const float UNUSED(fMagS), const float UNUSED(fMagT),
	const tbool bIsOrientationPreserving, const int iFace, const int iVert)
{
	const MikkTSpace* userData = GetUserDataFromContext(pContext);
	const SModelFace& face = userData->m_Model->GetFaces()[iFace];
	const SModelVertex& vertex = userData->m_Model->GetVertices()[face.m_Verts[iVert]];

	const CVector3D &p = vertex.m_Coords;
	userData->m_NewVertices.push_back(p.X);
	userData->m_NewVertices.push_back(p.Y);
	userData->m_NewVertices.push_back(p.Z);

	const CVector3D& n = vertex.m_Norm;
	userData->m_NewVertices.push_back(n.X);
	userData->m_NewVertices.push_back(n.Y);
	userData->m_NewVertices.push_back(n.Z);

	userData->m_NewVertices.push_back(fvTangent[0]);
	userData->m_NewVertices.push_back(fvTangent[1]);
	userData->m_NewVertices.push_back(fvTangent[2]);
	userData->m_NewVertices.push_back(bIsOrientationPreserving != 0 ? 1.f : -1.f);

	if (userData->m_GpuSkinning)
	{
		for (u8 j = 0; j < 4; ++j)
		{
			userData->m_NewVertices.push_back(vertex.m_Blend.m_Bone[j]);
			userData->m_NewVertices.push_back(255.f * vertex.m_Blend.m_Weight[j]);
		}
	}

	size_t numUVsPerVertex = userData->m_Model->GetNumUVsPerVertex();
	for (size_t UVset = 0; UVset < numUVsPerVertex; ++UVset)
	{
		userData->m_NewVertices.push_back(vertex.m_UVs[UVset * 2]);
		userData->m_NewVertices.push_back(1.f - vertex.m_UVs[UVset * 2 + 1]);
	}
}

MikkTSpace* MikkTSpace::GetUserDataFromContext(const SMikkTSpaceContext *pContext)
{
	return static_cast<MikkTSpace*>(pContext->m_pUserData);
}

SModelVertex MikkTSpace::GetVertex(const SMikkTSpaceContext *pContext, const int iFace, const int iVert)
{
	const MikkTSpace* userData = GetUserDataFromContext(pContext);
	const SModelFace& f = userData->m_Model->GetFaces()[iFace];
	return userData->m_Model->GetVertices()[f.m_Verts[iVert]];
}
