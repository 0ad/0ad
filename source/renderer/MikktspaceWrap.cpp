
#include "precompiled.h"

#include <boost/bind.hpp>

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextureManager.h"
#include <graphics/mikktspace.h>

#include <renderer/MikktspaceWrap.h>



MikkTSpace::MikkTSpace(const CModelDefPtr& m, std::vector<float>& v) : model(m), newVertices(v)
{
	// ensure that newVertices is empty
	newVertices.clear();
	
	// set up SMikkTSpaceInterface struct
	interface.m_getNumFaces = getNumFaces;
	interface.m_getNumVerticesOfFace = getNumVerticesOfFace;
	interface.m_getPosition = getPosition;
	interface.m_getNormal = getNormal;
	interface.m_getTexCoord = getTexCoord;
	interface.m_setTSpaceBasic = NULL;
	interface.m_setTSpace = setTSpace;

	// set up SMikkTSpaceContext struct
	context.m_pInterface = &interface;
	context.m_pUserData = (void*)this;
}

void MikkTSpace::generate()
{
	genTangSpaceDefault(&context);
}


int MikkTSpace::getNumFaces(const SMikkTSpaceContext *pContext)
{
	return ((MikkTSpace*)pContext->m_pUserData)->model->GetNumFaces();
}


int MikkTSpace::getNumVerticesOfFace(const SMikkTSpaceContext* UNUSED(pContext), const int UNUSED(iFace))
{
	return 3;
}


void MikkTSpace::getPosition(const SMikkTSpaceContext *pContext, 
		float fvPosOut[], const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	const CVector3D &p = ((MikkTSpace*)pContext->m_pUserData)->model->GetVertices()[i].m_Coords;

	fvPosOut[0] = p.X;
	fvPosOut[1] = p.Y;
	fvPosOut[2] = p.Z;
}


void MikkTSpace::getNormal(const SMikkTSpaceContext *pContext, 
		float fvNormOut[], const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	const CVector3D &n = ((MikkTSpace*)pContext->m_pUserData)->model->GetVertices()[i].m_Norm;

	fvNormOut[0] = n.X;
	fvNormOut[1] = n.Y;
	fvNormOut[2] = n.Z;
}


void MikkTSpace::getTexCoord(const SMikkTSpaceContext *pContext, 
		float fvTexcOut[], const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	SModelVertex &v = ((MikkTSpace*)pContext->m_pUserData)->model->GetVertices()[i];

	// the tangents are calculated according to the 'default' UV set
	fvTexcOut[0] = v.m_UVs[0];
	fvTexcOut[1] = 1.0-v.m_UVs[1];		
}


void MikkTSpace::setTSpace(const SMikkTSpaceContext * pContext, const float fvTangent[], 
		const float UNUSED(fvBiTangent)[], const float UNUSED(fMagS), const float UNUSED(fMagT), 
		const tbool bIsOrientationPreserving, const int iFace, const int iVert)
{
	SModelFace &face = ((MikkTSpace*)pContext->m_pUserData)->model->GetFaces()[iFace];
	long i = face.m_Verts[iVert];
	
	SModelVertex* vertices = ((MikkTSpace*)pContext->m_pUserData)->model->GetVertices();
	size_t numUVsPerVertex = ((MikkTSpace*)pContext->m_pUserData)->model->GetNumUVsPerVertex();
	std::vector<float>& newVertices = ((MikkTSpace*)pContext->m_pUserData)->newVertices;
	
	const CVector3D &p = vertices[i].m_Coords;
	const CVector3D &n = vertices[i].m_Norm;	
	
	newVertices.push_back(p.X);
	newVertices.push_back(p.Y);
	newVertices.push_back(p.Z);
	
	newVertices.push_back(n.X);
	newVertices.push_back(n.Y);
	newVertices.push_back(n.Z);
	
	newVertices.push_back(fvTangent[0]);
	newVertices.push_back(fvTangent[1]);
	newVertices.push_back(fvTangent[2]);
	newVertices.push_back(bIsOrientationPreserving > 0.5 ? 1.0f : (-1.0f));

	for (size_t UVset = 0; UVset < numUVsPerVertex; ++UVset)
	{
		newVertices.push_back(vertices[i].m_UVs[UVset * 2]);
		newVertices.push_back(1.0 - vertices[i].m_UVs[UVset * 2 + 1]);
	}
}



