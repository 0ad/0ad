///////////////////////////////////////////////////////////////////////////////
//
// Name:		SkeletonAnim.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "SkeletonAnim.h"
#include "FilePacker.h"
#include "FileUnpacker.h"


///////////////////////////////////////////////////////////////////////////////////////////
// CSkeletonAnim constructor
CSkeletonAnim::CSkeletonAnim() : m_Keys(0), m_NumKeys(0), m_NumFrames(0), m_FrameTime(0)
{
	m_Name[0]='\0';
}

///////////////////////////////////////////////////////////////////////////////////////////
// CSkeletonAnim destructor
CSkeletonAnim::~CSkeletonAnim() 
{
	delete[] m_Keys;
}

///////////////////////////////////////////////////////////////////////////////////////////
// BuildBoneMatrices: build matrices for all bones at the given time (in MS) in this 
// animation
void CSkeletonAnim::BuildBoneMatrices(float time,CMatrix3D* matrices) const
{
	float fstartframe=time/m_FrameTime;
	u32 startframe=u32(time/m_FrameTime);
	float deltatime=fstartframe-startframe;

	startframe%=m_NumFrames; 

	u32 endframe=startframe+1;
	endframe%=m_NumFrames; 

	u32 i;
	for (i=0;i<m_NumKeys;i++) {
		const Key& startkey=GetKey(startframe,i);
		const Key& endkey=GetKey(endframe,i);
		
		CVector3D trans=startkey.m_Translation*(1-deltatime)+endkey.m_Translation*deltatime;
		CQuaternion rot;
		rot.Slerp(startkey.m_Rotation,endkey.m_Rotation,deltatime);

		matrices[i].SetIdentity();
		matrices[i].Rotate(rot);
		matrices[i].Translate(trans);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
// Load: try to load the anim from given file; return a new anim if successful
CSkeletonAnim* CSkeletonAnim::Load(const char* filename)
{
	CFileUnpacker unpacker;
	unpacker.Read(filename,"PSSA");
			
	// check version
	if (unpacker.GetVersion()<FILE_READ_VERSION) {
		throw CFileUnpacker::CFileVersionError();
	}

	// unpack the data
	CSkeletonAnim* anim=new CSkeletonAnim;
	try {
		CStr str;
		unpacker.UnpackString(str);
		strcpy(anim->m_Name,(const char*) str);

		unpacker.UnpackRaw(&anim->m_FrameTime,sizeof(anim->m_FrameTime));
		unpacker.UnpackRaw(&anim->m_NumKeys,sizeof(anim->m_NumKeys));
		unpacker.UnpackRaw(&anim->m_NumFrames,sizeof(anim->m_NumFrames));
		anim->m_Keys=new Key[anim->m_NumKeys*anim->m_NumFrames];
		unpacker.UnpackRaw(anim->m_Keys,anim->m_NumKeys*anim->m_NumFrames*sizeof(Key));
	} catch (...) {
		delete anim;
		throw CFileUnpacker::CFileEOFError();
	}

	return anim;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Save: try to save anim to file
void CSkeletonAnim::Save(const char* filename,const CSkeletonAnim* anim)
{
	CFilePacker packer;

	// pack up all the data
	packer.PackString(CStr(anim->m_Name));
	packer.PackRaw(&anim->m_FrameTime,sizeof(anim->m_FrameTime));
	packer.PackRaw(&anim->m_NumKeys,sizeof(anim->m_NumKeys));
	packer.PackRaw(&anim->m_NumFrames,sizeof(anim->m_NumFrames));
	packer.PackRaw(anim->m_Keys,anim->m_NumKeys*anim->m_NumFrames*sizeof(Key));

	// now write it
	packer.Write(filename,FILE_VERSION,"PSSA");
}

