///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////


#include "pmdexp_types.h"
#include "ModelDef.h"
#include "FilePacker.h"

///////////////////////////////////////////////////////////////////////////////
// CModelDef Constructor
CModelDef::CModelDef()	
	: m_pVertices(0), m_NumVertices(0), m_pFaces(0), m_NumFaces(0), m_Bones(0), m_NumBones(0),
	m_NumPropPoints(0), m_PropPoints(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// CModelDef Destructor
CModelDef::~CModelDef()
{
	delete[] m_pVertices;
	delete[] m_pFaces;
	delete[] m_Bones;
	delete[] m_PropPoints;
}

///////////////////////////////////////////////////////////////////////////////
// FindPropPoint: find and return pointer to prop point matching given name; 
// return null if no match (case insensitive search)
SPropPoint* CModelDef::FindPropPoint(const char* name) const
{
	for (uint i=0;i<m_NumPropPoints;i++) {
		if (stricmp(name,m_PropPoints[i].m_Name)==0) {
			return &m_PropPoints[i];
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Save: write the given CModelDef to the given file
void CModelDef::Save(const char* filename,const CModelDef* mdef)
{
	CFilePacker packer;

	// pack everything up
	u32 numVertices=mdef->GetNumVertices();
	packer.PackRaw(&numVertices,sizeof(numVertices));
	packer.PackRaw(mdef->GetVertices(),sizeof(SModelVertex)*numVertices);
	
	u32 numFaces=mdef->GetNumFaces();
	packer.PackRaw(&numFaces,sizeof(numFaces));
	packer.PackRaw(mdef->GetFaces(),sizeof(SModelFace)*numFaces);
	
	packer.PackRaw(&mdef->m_NumBones,sizeof(mdef->m_NumBones));
	if (mdef->m_NumBones) {
		packer.PackRaw(mdef->m_Bones,sizeof(CBoneState)*mdef->m_NumBones);
	}

	packer.PackRaw(&mdef->m_NumPropPoints,sizeof(mdef->m_NumPropPoints));
	for (u32 i=0;i<mdef->m_NumPropPoints;i++) {
		packer.PackString(mdef->m_PropPoints[i].m_Name);
		packer.PackRaw(&mdef->m_PropPoints[i].m_Position.X,sizeof(mdef->m_PropPoints[i].m_Position));
		packer.PackRaw(&mdef->m_PropPoints[i].m_Rotation.m_V.X,sizeof(mdef->m_PropPoints[i].m_Rotation));
		packer.PackRaw(&mdef->m_PropPoints[i].m_BoneIndex,sizeof(mdef->m_PropPoints[i].m_BoneIndex));
	}
	

	// flush everything out to file
	packer.Write(filename,FILE_VERSION,"PSMD");
}

