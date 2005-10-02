///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "ModelDef.h"
#include "FilePacker.h"
#include "FileUnpacker.h"

///////////////////////////////////////////////////////////////////////////////
// CModelDef Constructor
CModelDef::CModelDef()	
	: m_pVertices(0), m_NumVertices(0), m_pFaces(0), m_NumFaces(0), m_Bones(0), m_NumBones(0),
	m_NumPropPoints(0), m_PropPoints(0), m_RenderData(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// CModelDef Destructor
CModelDef::~CModelDef()
{
	delete m_RenderData;
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

		if (unpacker.GetVersion()>=2) {
			// versions >=2 also have prop point data
			unpacker.UnpackRaw(&mdef->m_NumPropPoints,sizeof(mdef->m_NumPropPoints));
			if (mdef->m_NumPropPoints) {
				mdef->m_PropPoints=new SPropPoint[mdef->m_NumPropPoints];
				for (u32 i=0;i<mdef->m_NumPropPoints;i++) {
					unpacker.UnpackString(mdef->m_PropPoints[i].m_Name);
					unpacker.UnpackRaw(&mdef->m_PropPoints[i].m_Position.X,sizeof(mdef->m_PropPoints[i].m_Position));
					unpacker.UnpackRaw(&mdef->m_PropPoints[i].m_Rotation.m_V.X,sizeof(mdef->m_PropPoints[i].m_Rotation));
					unpacker.UnpackRaw(&mdef->m_PropPoints[i].m_BoneIndex,sizeof(mdef->m_PropPoints[i].m_BoneIndex));

					// build prop point transform
					mdef->m_PropPoints[i].m_Transform.SetIdentity();
					mdef->m_PropPoints[i].m_Transform.Rotate(mdef->m_PropPoints[i].m_Rotation);
					mdef->m_PropPoints[i].m_Transform.Translate(mdef->m_PropPoints[i].m_Position);
				}
			}
		}
	} catch (CFileUnpacker::CFileEOFError) {
		delete mdef;
		throw CFileUnpacker::CFileEOFError();
	}

	return mdef;
}

///////////////////////////////////////////////////////////////////////////////
// Save: write the given CModelDef to the given file
void CModelDef::Save(const char* filename,const CModelDef* mdef)
{
	CFilePacker packer(FILE_VERSION, "PSMD");

	// pack everything up
	u32 numVertices=(u32)mdef->GetNumVertices();
	packer.PackRaw(&numVertices,sizeof(numVertices));
	packer.PackRaw(mdef->GetVertices(),sizeof(SModelVertex)*numVertices);
	
	u32 numFaces=(u32)mdef->GetNumFaces();
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
	packer.Write(filename);
}


// Set render data. This can only be done once at the moment.
// TODO: Is there a need to re-create render data? Perhaps reacting to render path changes?
void CModelDef::SetRenderData(CSharedRenderData* data)
{
	debug_assert(m_RenderData == 0);
	m_RenderData = data;
}

