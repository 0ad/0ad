/************************************************************
 *
 * File Name: ModelDef.Cpp
 *
 * Description: CModelDef is essentially a CModelFile, except
 *				that the data is stored in a more convenient
 *				way. To create a CModelDef, call
 *				CModelFile::ReadModelDef();
 *
 ************************************************************/

#include "ModelDef.h"

CModelDef::CModelDef()
{
	m_pVertices = NULL;
	m_pFaces = NULL;
	m_pBones = NULL;
	m_pAnimationKeys = NULL;
	m_pAnimationFrames = NULL;
	m_pAnimations = NULL;
	
	m_NumVertices = 0;
	m_NumFaces = 0;
	m_NumBones = 0;
	m_NumAnimationKeys = 0;
	m_NumAnimationFrames = 0;
	m_NumAnimations = 0;
}

CModelDef::~CModelDef()
{
	Destroy();
}

void CModelDef::Destroy()
{
	if (m_pVertices)
		delete [] m_pVertices;
	if (m_pFaces)
		delete [] m_pFaces;
	if (m_pBones)
		delete [] m_pBones;
	if (m_pAnimationKeys)
		delete [] m_pAnimationKeys;
	if (m_pAnimationFrames)
		delete [] m_pAnimationFrames;
	if (m_pAnimations)
		delete [] m_pAnimations;

	m_pVertices = NULL;
	m_pFaces = NULL;
	m_pBones = NULL;
	m_pAnimationKeys = NULL;
	m_pAnimationFrames = NULL;
	m_pAnimations = NULL;
	
	m_NumVertices = 0;
	m_NumFaces = 0;
	m_NumBones = 0;
	m_NumAnimationKeys = 0;
	m_NumAnimationFrames = 0;
	m_NumAnimations = 0;
}

void CModelDef::SetupBones()
{
	for (int i=0; i<m_NumBones; i++)
	{
		SModelBone *pBone = &m_pBones[i];

		pBone->m_Relative.SetIdentity();
		pBone->m_Absolute.SetIdentity();

		pBone->m_Relative.RotateX (pBone->m_Rotation.X);
		pBone->m_Relative.RotateY (pBone->m_Rotation.Y);
		pBone->m_Relative.RotateZ (pBone->m_Rotation.Z);
//		pBone->m_Relative.RotateX (DEGTORAD(90));

		pBone->m_Relative.Translate (pBone->m_Position);

		if (pBone->m_Parent >= 0)
		{
			SModelBone *pParent = &m_pBones[pBone->m_Parent];
			
			pBone->m_Absolute = pParent->m_Absolute * pBone->m_Relative;
		}
		else
			pBone->m_Absolute = pBone->m_Relative;
	}

	//we need to "un-transform" all the vertices by the initial
	//pose of the bones they are attached to.
	for (i=0; i<m_NumVertices; i++)
	{
		SModelVertex *pVertex = &m_pVertices[i];
		SModelBone *pBone = &m_pBones[pVertex->m_Bone];

		CVector3D BonePos = pBone->m_Absolute.GetTranslation();

		pVertex->m_Coords.X -= BonePos.X;
		pVertex->m_Coords.Y -= BonePos.Y;
		pVertex->m_Coords.Z -= BonePos.Z;

		CMatrix3D BoneInvMat = pBone->m_Absolute.GetTranspose();

		pVertex->m_Coords = BoneInvMat.Rotate (pVertex->m_Coords);
	}
}
