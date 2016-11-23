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

#include "precompiled.h"

#include <boost/bind.hpp>

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"

#include "renderer/MikktspaceWrap.h"

#include "third_party/mikktspace/mikktspace.h"



MikkTSpace::MikkTSpace(const CModelDefPtr& m, std::vector<float>& v, bool gpuSkinning) : m_Model(m),
			m_NewVertices(v), m_GpuSkinning(gpuSkinning)
{
	// ensure that m_NewVertices is empty
	m_NewVertices.clear();
	
	// set up SMikkTSpaceInterface struct
	m_Interface.m_getNumFaces = getNumFaces;
	m_Interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
	m_Interface.m_getPosition = getPosition;
	m_Interface.m_getNormal = getNormal;
	m_Interface.m_getTexCoord = getTexCoord;
	m_Interface.m_setTSpaceBasic = NULL;
	m_Interface.m_setTSpace = setTSpace;

	// set up SMikkTSpaceContext struct
	m_Context.m_pInterface = &m_Interface;
	m_Context.m_pUserData = (void*)this;
}

void MikkTSpace::generate()
{
	genTangSpaceDefault(&m_Context);
}


int MikkTSpace::getNumFaces(const SMikkTSpaceContext *pContext)
{
	return ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetNumFaces();
}


int MikkTSpace::getNumVerticesOfFace(const SMikkTSpaceContext* UNUSED(pContext), const int UNUSED(iFace))
{
	return 3;
}


void MikkTSpace::getPosition(const SMikkTSpaceContext *pContext,
		float fvPosOut[], const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	const CVector3D &p = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetVertices()[i].m_Coords;

	fvPosOut[0] = p.X;
	fvPosOut[1] = p.Y;
	fvPosOut[2] = p.Z;
}


void MikkTSpace::getNormal(const SMikkTSpaceContext *pContext,
		float fvNormOut[], const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	const CVector3D &n = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetVertices()[i].m_Norm;

	fvNormOut[0] = n.X;
	fvNormOut[1] = n.Y;
	fvNormOut[2] = n.Z;
}


void MikkTSpace::getTexCoord(const SMikkTSpaceContext *pContext,
		float fvTexcOut[], const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	SModelVertex &v = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetVertices()[i];

	// the tangents are calculated according to the 'default' UV set
	fvTexcOut[0] = v.m_UVs[0];
	fvTexcOut[1] = 1.0-v.m_UVs[1];		
}


void MikkTSpace::setTSpace(const SMikkTSpaceContext * pContext, const float fvTangent[],
		const float UNUSED(fvBiTangent)[], const float UNUSED(fMagS), const float UNUSED(fMagT),
		const tbool bIsOrientationPreserving, const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	
	SModelVertex* vertices = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetVertices();
	size_t numUVsPerVertex = ((MikkTSpace*)pContext->m_pUserData)->m_Model->GetNumUVsPerVertex();
	std::vector<float>& m_NewVertices = ((MikkTSpace*)pContext->m_pUserData)->m_NewVertices;
	
	const CVector3D &p = vertices[i].m_Coords;
	const CVector3D &n = vertices[i].m_Norm;	
	
	m_NewVertices.push_back(p.X);
	m_NewVertices.push_back(p.Y);
	m_NewVertices.push_back(p.Z);
	
	m_NewVertices.push_back(n.X);
	m_NewVertices.push_back(n.Y);
	m_NewVertices.push_back(n.Z);
	
	m_NewVertices.push_back(fvTangent[0]);
	m_NewVertices.push_back(fvTangent[1]);
	m_NewVertices.push_back(fvTangent[2]);
	m_NewVertices.push_back(bIsOrientationPreserving > 0.5 ? 1.0f : (-1.0f));
	
	if (((MikkTSpace*)pContext->m_pUserData)->m_GpuSkinning)
	{
		for (size_t j = 0; j < 4; ++j)
		{
			m_NewVertices.push_back(vertices[i].m_Blend.m_Bone[j]);
			m_NewVertices.push_back(255.f * vertices[i].m_Blend.m_Weight[j]);
		}
	}

	for (size_t UVset = 0; UVset < numUVsPerVertex; ++UVset)
	{
		m_NewVertices.push_back(vertices[i].m_UVs[UVset * 2]);
		m_NewVertices.push_back(1.0 - vertices[i].m_UVs[UVset * 2 + 1]);
	}
}



