/**
 * =========================================================================
 * File        : ModelDef.cpp
 * Project     : 0 A.D.
 * Description : Defines a raw 3d model.
 * =========================================================================
 */

#include "precompiled.h"

#include "ModelDef.h"
#include "graphics/SkeletonAnimDef.h"
#include "ps/FileIo.h"
#include "maths/Vector4D.h"

CVector3D CModelDef::SkinPoint(const SModelVertex& vtx,
							   const CMatrix3D newPoseMatrices[],
							   const CMatrix3D inverseBindMatrices[])
{
	CVector3D result (0, 0, 0);

	for (int i = 0; i < SVertexBlend::SIZE && vtx.m_Blend.m_Bone[i] != 0xff; ++i)
	{
		CVector3D bindSpace = inverseBindMatrices[vtx.m_Blend.m_Bone[i]].Transform(vtx.m_Coords);
		CVector3D worldSpace = newPoseMatrices[vtx.m_Blend.m_Bone[i]].Transform(bindSpace);
		result += worldSpace * vtx.m_Blend.m_Weight[i];
	}

	return result;
}

CVector3D CModelDef::SkinNormal(const SModelVertex& vtx,
								const CMatrix3D newPoseMatrices[],
								const CMatrix3D inverseBindMatrices[])
{
	// To be correct, the normal vectors apparently need to be multiplied by the
	// inverse of the transpose. Unfortunately inverses are slow.
	// If a matrix is orthogonal, M * M^T = I and so the inverse of the transpose
	// is the original matrix. But that's not entirely relevant here, because
	// the bone matrices include translation components and so they're not
	// orthogonal.
	// But that's okay because we have
	//   M = T * R
	// and want to find
	//   n' = (M^T^-1) * n
	//      = (T * R)^T^-1 * n
	//      = (R^T * T^T)^-1 * n
	//      = (T^T^-1 * R^T^-1) * n
	// R is indeed orthogonal so R^T^-1 = R. T isn't orthogonal at all.
	// But n is only a 3-vector, and from the forms of T and R (which have
	// lots of zeroes) I can convince myself that replacing T with T^T^-1 has no
	// effect on anything but the fourth component of M^T^-1 - and the fourth
	// component is discarded since it has no effect on n', and so we can happily
	// use n' = M*n.
	//
	// (This isn't very good as a proof, but it's better than assuming M is
	// orthogonal when it's clearly not.)

	CVector3D result (0, 0, 0);

	for (int i = 0; i < SVertexBlend::SIZE && vtx.m_Blend.m_Bone[i] != 0xff; ++i)
	{
		CVector3D bindSpace = inverseBindMatrices[vtx.m_Blend.m_Bone[i]].Rotate(vtx.m_Norm);
		CVector3D worldSpace = newPoseMatrices[vtx.m_Blend.m_Bone[i]].Rotate(bindSpace);
		result += worldSpace * vtx.m_Blend.m_Weight[i];
	}
	
	// If there was more than one influence, the result is probably not going
	// to be of unit length (since it's a weighted sum of several independent
	// unit vectors), so we need to normalise it.
	// (It's fairly common to only have one influence, so it seems sensible to
	// optimise that case a bit.)
	if (vtx.m_Blend.m_Bone[1] != 0xff) // if more than one influence
		result.Normalize();

	return result;
}

// CModelDef Constructor
CModelDef::CModelDef()	
	: m_NumVertices(0), m_pVertices(0), m_NumFaces(0), m_pFaces(0), m_NumBones(0), m_Bones(0),
	m_NumPropPoints(0), m_PropPoints(0), m_Name("[not loaded]")
{
}

// CModelDef Destructor
CModelDef::~CModelDef()
{
	for(RenderDataMap::iterator it = m_RenderData.begin(); it != m_RenderData.end(); ++it)
		delete it->second;
	delete[] m_pVertices;
	delete[] m_pFaces;
	delete[] m_Bones;
	delete[] m_PropPoints;
}

// FindPropPoint: find and return pointer to prop point matching given name; 
// return null if no match (case insensitive search)
SPropPoint* CModelDef::FindPropPoint(const char* name) const
{
	for (size_t i = 0; i < m_NumPropPoints; ++i)
		if (m_PropPoints[i].m_Name == name)
			return &m_PropPoints[i];

	return 0;
}

// Load: read and return a new CModelDef initialised with data from given file
CModelDef* CModelDef::Load(const VfsPath& filename, const char* name)
{
	CFileUnpacker unpacker;

	// read everything in from file
	unpacker.Read(filename,"PSMD");
			
	// check version
	if (unpacker.GetVersion()<FILE_READ_VERSION) {
		throw PSERROR_File_InvalidVersion();
	}

	std::auto_ptr<CModelDef> mdef (new CModelDef());
	mdef->m_Name = name;

	// now unpack everything 
	mdef->m_NumVertices = unpacker.UnpackSize();
	mdef->m_pVertices=new SModelVertex[mdef->m_NumVertices];
	unpacker.UnpackRaw(mdef->m_pVertices,sizeof(SModelVertex)*mdef->m_NumVertices);
	
	mdef->m_NumFaces = unpacker.UnpackSize();
	mdef->m_pFaces=new SModelFace[mdef->m_NumFaces];
	unpacker.UnpackRaw(mdef->m_pFaces,sizeof(SModelFace)*mdef->m_NumFaces);
	
	mdef->m_NumBones = unpacker.UnpackSize();
	if (mdef->m_NumBones)
	{
		mdef->m_Bones=new CBoneState[mdef->m_NumBones];
		unpacker.UnpackRaw(mdef->m_Bones,mdef->m_NumBones*sizeof(CBoneState));
	}

	if (unpacker.GetVersion() >= 2)
	{
		// versions >=2 also have prop point data
		mdef->m_NumPropPoints = unpacker.UnpackSize();
		if (mdef->m_NumPropPoints) {
			mdef->m_PropPoints=new SPropPoint[mdef->m_NumPropPoints];
			for (size_t i=0;i<mdef->m_NumPropPoints;i++) {
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

	if (unpacker.GetVersion() <= 2)
	{
		// Versions <=2 store the vertexes relative to the bind pose. That
		// isn't useful when you want to do correct skinning, so later versions
		// store them in world space. So, fix the old models by skinning each
		// vertex:

		if (mdef->m_NumBones) // only do skinned models
		{
			CMatrix3D identity;
			identity.SetIdentity();
			std::vector<CMatrix3D> identityBones (mdef->m_NumBones, identity);

			std::vector<CMatrix3D> bindPose (mdef->m_NumBones);

			for (size_t i = 0; i < mdef->m_NumBones; ++i)
			{
				bindPose[i].SetIdentity();
				bindPose[i].Rotate(mdef->m_Bones[i].m_Rotation);
				bindPose[i].Translate(mdef->m_Bones[i].m_Translation);
			}

			for (size_t i = 0; i < mdef->m_NumVertices; ++i)
			{
				mdef->m_pVertices[i].m_Coords = SkinPoint(mdef->m_pVertices[i], &bindPose[0], &identityBones[0]);
				mdef->m_pVertices[i].m_Norm = SkinNormal(mdef->m_pVertices[i], &bindPose[0], &identityBones[0]);
			}
		}
	}

	return mdef.release();
}

// Save: write the given CModelDef to the given file
void CModelDef::Save(const VfsPath& filename,const CModelDef* mdef)
{
	CFilePacker packer(FILE_VERSION, "PSMD");

	// pack everything up
	const size_t numVertices = mdef->GetNumVertices();
	packer.PackSize(numVertices);
	packer.PackRaw(mdef->GetVertices(),sizeof(SModelVertex)*numVertices);
	
	const size_t numFaces = mdef->GetNumFaces();
	packer.PackSize(numFaces);
	packer.PackRaw(mdef->GetFaces(),sizeof(SModelFace)*numFaces);
	
	const size_t numBones = mdef->m_NumBones;
	packer.PackSize(numBones);
	if (numBones)
		packer.PackRaw(mdef->m_Bones,sizeof(CBoneState)*numBones);

	const size_t numPropPoints = mdef->m_NumPropPoints;
	packer.PackSize(numPropPoints);
	for (size_t i=0;i<mdef->m_NumPropPoints;i++) {
		packer.PackString(mdef->m_PropPoints[i].m_Name);
		packer.PackRaw(&mdef->m_PropPoints[i].m_Position.X,sizeof(mdef->m_PropPoints[i].m_Position));
		packer.PackRaw(&mdef->m_PropPoints[i].m_Rotation.m_V.X,sizeof(mdef->m_PropPoints[i].m_Rotation));
		packer.PackRaw(&mdef->m_PropPoints[i].m_BoneIndex,sizeof(mdef->m_PropPoints[i].m_BoneIndex));
	}
	
	// flush everything out to file
	packer.Write(filename);
}

// SetRenderData: Set the render data object for the given key,
void CModelDef::SetRenderData(const void* key, CModelDefRPrivate* data)
{
	delete m_RenderData[key];
	m_RenderData[key] = data;
}

// GetRenderData: Get the render data object for the given key,
// or 0 if no such object exists.
// Reference count of the render data object is automatically increased.
CModelDefRPrivate* CModelDef::GetRenderData(const void* key) const
{
	RenderDataMap::const_iterator it = m_RenderData.find(key);
	
	if (it != m_RenderData.end())
		return it->second;
	
	return 0;
}
