///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "ModelDef.h"
#include "FilePacker.h"
#include "FileUnpacker.h"

///////////////////////////////////////////////////////////////////////////////
// CModelDef Constructor
CModelDef::CModelDef()	
	: m_pVertices(0), m_NumVertices(0), m_pFaces(0), m_NumFaces(0), m_Bones(0), m_NumBones(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// CModelDef Destructor
CModelDef::~CModelDef()
{
	delete[] m_pVertices;
	delete[] m_pFaces;
	delete[] m_Bones;
}


///////////////////////////////////////////////////////////////////////////////
// Load: read and return a new CModelDef initialised with data from given file
CModelDef* CModelDef::Load(const char* filename)
{
	CFileUnpacker unpacker;
	
	// read everything in from file
	unpacker.Read(filename,"PSMD");
			
	// check version
	if (unpacker.GetVersion()<FILE_READ_VERSION) {
		throw CFileUnpacker::CFileVersionError();
	}

	CModelDef* mdef=new CModelDef;
	try {
		// now unpack everything 
		unpacker.UnpackRaw(&mdef->m_NumVertices,sizeof(mdef->m_NumVertices));	
		mdef->m_pVertices=new SModelVertex[mdef->m_NumVertices];
		unpacker.UnpackRaw(mdef->m_pVertices,sizeof(SModelVertex)*mdef->m_NumVertices);
		
		unpacker.UnpackRaw(&mdef->m_NumFaces,sizeof(mdef->m_NumFaces));
		mdef->m_pFaces=new SModelFace[mdef->m_NumFaces];
		unpacker.UnpackRaw(mdef->m_pFaces,sizeof(SModelFace)*mdef->m_NumFaces);
		
		unpacker.UnpackRaw(&mdef->m_NumBones,sizeof(mdef->m_NumBones));
		if (mdef->m_NumBones) {
			mdef->m_Bones=new CBoneState[mdef->m_NumBones];
			unpacker.UnpackRaw(mdef->m_Bones,mdef->m_NumBones*sizeof(CBoneState));
		}
	} catch (...) {
		delete mdef;
		throw CFileUnpacker::CFileEOFError();
	}

	return mdef;
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

	// flush everything out to file
	packer.Write(filename,FILE_VERSION,"PSMD");
}
