///////////////////////////////////////////////////////////////////////////////
//
// Name:		Model.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "Model.h"
#include "Quaternion.h"
#include "Bound.h"

///////////////////////////////////////////////////////////////////////////////
// Constructor
CModel::CModel() 
	: m_pModelDef(0), m_Anim(0), m_AnimTime(0), 
	m_BoneMatrices(0), m_InvBoneMatrices(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
CModel::~CModel()
{
	ReleaseData();
}

///////////////////////////////////////////////////////////////////////////////
// ReleaseData: delete anything allocated by the model
void CModel::ReleaseData()
{
	delete[] m_BoneMatrices;
	delete[] m_InvBoneMatrices;
}

///////////////////////////////////////////////////////////////////////////////
// InitModel: setup model from given geometry
bool CModel::InitModel(CModelDef* modeldef)
{
	// clean up any existing data first
	ReleaseData();

	m_pModelDef = modeldef;
	
	u32 numBones=modeldef->GetNumBones();
	if (numBones>0) {
		// allocate matrices for bone transformations
		m_BoneMatrices=new CMatrix3D[numBones];
		m_InvBoneMatrices=new CMatrix3D[numBones];
		// store default pose until animation assigned
		CBoneState* defpose=modeldef->GetBones();
		for (uint i=0;i<numBones;i++) {
			CMatrix3D& m=m_BoneMatrices[i];
			m.SetIdentity();
			m.Rotate(defpose[i].m_Rotation);
			m.Translate(defpose[i].m_Translation);
			m.GetInverse(m_InvBoneMatrices[i]);
		}
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// SkinPoint: skin the given point using the given blend and bonestate data
static CVector3D SkinPoint(const CVector3D& pos,const SVertexBlend& blend,
						   const CBoneState* bonestates)
{
	CVector3D result(0,0,0);
	for (int i=0;i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff;i++) {
		CMatrix3D m;
		m.SetIdentity();
		m.Rotate(bonestates[blend.m_Bone[i]].m_Rotation);		
		m.Translate(bonestates[blend.m_Bone[i]].m_Translation);		

		CVector3D tmp=m.Transform(pos);
		result+=tmp*blend.m_Weight[i];
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////
// CalcBound: calculate the world space bounds of this model
//
// TODO,RC 11/03/04: need to calculate (and store somewhere) the object space 
// bounds, and then just retransform the bounds as necessary, rather than 
// recalculating them from vertex data every time the transform changes
void CModel::CalcBounds()
{
	m_Bounds.SetEmpty();

	int numverts=m_pModelDef->GetNumVertices();
	SModelVertex* verts=m_pModelDef->GetVertices();
	
	u32 numbones=m_pModelDef->GetNumBones();
	if (numbones>0) {
		// Boned object: tricky to get an ideal bound - for the minute, just use the bound of 
		// the reference pose.  There's no guarantee that when animations are applied to the 
		// model, the bounds will be within this bound - ideally, we want the bound of the 
		// object to be the union of the bounds of the model for each animation
		for (int i=0;i<numverts;i++) {
			CVector3D tmp=SkinPoint(verts[i].m_Coords,verts[i].m_Blend,m_pModelDef->GetBones());
			m_Bounds+=m_Transform.Transform(tmp);
		}
	} else {
		for (int i=0;i<numverts;i++) {
			m_Bounds+=m_Transform.Transform(verts[i].m_Coords);
		}	
	}
}

///////////////////////////////////////////////////////////////////////////////
// Update: update this model by the given time, in seconds
void CModel::Update(float time)
{
	// convert to ms 
	time*=1000;

	if (m_Anim && m_BoneMatrices) {
		m_AnimTime+=time;
		
		float duration=m_Anim->GetDuration();
		if (m_AnimTime>duration) {
			m_AnimTime=(float) fmod(m_AnimTime,duration);
		}

		m_Anim->BuildBoneMatrices(m_AnimTime,m_BoneMatrices);
		for (int i=0;i<m_pModelDef->GetNumBones();i++) {
			m_BoneMatrices[i].GetInverse(m_InvBoneMatrices[i]);
		}

		if (m_RenderData) m_RenderData->m_UpdateFlags|=RENDERDATA_UPDATE_VERTICES;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// SetAnimation: set the given animation as the current animation on this model;
// return false on error, else true
bool CModel::SetAnimation(CSkeletonAnim* anim)
{ 
	if (anim) {
		if (!m_BoneMatrices) {
			// not boned, can't animate
			return false;
		}

		if (anim->GetNumKeys()!=m_pModelDef->GetNumBones()) {
			// mismatch between models skeleton and animations skeleton
			return false;
		}
	} 

	m_AnimTime=0; 
	m_Anim=anim; 

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// Clone: return a clone of this model
CModel* CModel::Clone() const
{
	CModel* clone=new CModel;
	clone->InitModel(m_pModelDef);
	clone->SetTexture(m_Texture);
	clone->SetAnimation(m_Anim);
	return clone;
}