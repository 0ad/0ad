///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:		Model.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "Model.h"
#include "ModelDef.h"
#include "Quaternion.h"
#include "Bound.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h"
#include "SkeletonAnimManager.h"
#include "MeshManager.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
CModel::CModel() 
	: m_Flags(0), m_Anim(0), m_AnimTime(0), 
	m_BoneMatrices(0), m_InvBoneMatrices(0), m_BoneMatricesValid(false)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
CModel::~CModel()
{
	ReleaseData();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReleaseData: delete anything allocated by the model
void CModel::ReleaseData()
{
	delete[] m_BoneMatrices;
	delete[] m_InvBoneMatrices;
	for (size_t i=0;i<m_Props.size();i++) {
		delete m_Props[i].m_Model;
	}
	m_Props.clear();
	m_pModelDef = CModelDefPtr();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InitModel: setup model from given geometry
bool CModel::InitModel(CModelDefPtr modeldef)
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
		m_BoneMatricesValid=true;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SkinPoint: skin the given point using the given blend and matrix data
static CVector3D SkinPoint(const CVector3D& pos,const SVertexBlend& blend,
						   const CMatrix3D* bonestates)
{
	CVector3D result,tmp;

	// must have at least one valid bone if we're using SkinPoint
	assert(blend.m_Bone[0]!=0xff);

	const CMatrix3D& m=bonestates[blend.m_Bone[0]];
	m.Transform(pos,result);
	result*=blend.m_Weight[0];

	for (int i=1;i<SVertexBlend::SIZE && blend.m_Bone[i]!=0xff;i++) {
		const CMatrix3D& m=bonestates[blend.m_Bone[i]];
		m.Transform(pos,tmp);
		result+=tmp*blend.m_Weight[i];
	}

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalcBound: calculate the world space bounds of this model
void CModel::CalcBounds()
{
	m_ObjectBounds.Transform(GetTransform(),m_Bounds);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalcObjectBounds: calculate object space bounds of this model, based solely on vertex positions
void CModel::CalcObjectBounds()
{
	m_ObjectBounds.SetEmpty();

	int numverts=m_pModelDef->GetNumVertices();
	SModelVertex* verts=m_pModelDef->GetVertices();

	for (int i=0;i<numverts;i++) {
		m_ObjectBounds+=verts[i].m_Coords;
	}		
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalcAnimatedObjectBound: calculate bounds encompassing all vertex positions for given animation 
void CModel::CalcAnimatedObjectBound(CSkeletonAnimDef* anim,CBound& result)
{
	result.SetEmpty();

	CSkeletonAnim dummyanim;
	dummyanim.m_AnimDef=anim;
	if (!SetAnimation(&dummyanim)) return;

	int numverts=m_pModelDef->GetNumVertices();
	SModelVertex* verts=m_pModelDef->GetVertices();
	
	// iterate through every frame of the animation
	for (uint j=0;j<anim->GetNumFrames();j++) {		
		// extend bounds by vertex positions at the frame
		for (int i=0;i<numverts;i++) {
			CVector3D tmp=SkinPoint(verts[i].m_Coords,verts[i].m_Blend,GetBoneMatrices());
			result+=tmp;
		}		
		// advance to next frame
		m_AnimTime+=anim->GetFrameTime();
		m_BoneMatricesValid=false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildAnimation: load raw animation frame animation from given file, and build a 
// animation specific to this model
CSkeletonAnim* CModel::BuildAnimation(const char* filename,float speed)
{
	CSkeletonAnimDef* def=g_SkelAnimMan.GetAnimation(filename);
	if (!def) return 0;

	CSkeletonAnim* anim=new CSkeletonAnim;
	anim->m_AnimDef=def;
	anim->m_Speed=speed;
	CalcAnimatedObjectBound(def,anim->m_ObjectBounds);

	return anim;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Update: update this model by the given time, in seconds
void CModel::Update(float time)
{
	if (m_Anim && m_BoneMatrices) {
		// convert to ms and adjust for animation speed
		float animtime=time*1000*m_Anim->m_Speed;

		// update animation time, but don't calculate bone matrices - do that (lazily) when
		// something requests them; that saves some calculation work for offscreen models,
		// and also assures the world space, inverted bone matrices (required for normal
		// skinning) are up to date with respect to m_Transform 
		m_AnimTime+=animtime;
		
		float duration=m_Anim->m_AnimDef->GetDuration();
		if (m_AnimTime>duration) {
			if( m_Flags & MODELFLAG_NOLOOPANIMATION )
				SetAnimation( NULL );
			m_AnimTime=(float) fmod(m_AnimTime,duration);
		}
		
		// mark vertices as dirty
		SetDirty(RENDERDATA_UPDATE_VERTICES);
		
		// mark matrices as dirty
		m_BoneMatricesValid=false;
	}

	// update props
	for (uint i=0;i<m_Props.size();i++) {
		m_Props[i].m_Model->Update(time);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GenerateBoneMatrices: calculate necessary bone transformation matrices for skinning
void CModel::GenerateBoneMatrices()
{
	if (!m_Anim || !m_BoneMatrices) return;

	m_Anim->m_AnimDef->BuildBoneMatrices(m_AnimTime,m_BoneMatrices);

	const CMatrix3D& transform=GetTransform();
	for (int i=0;i<m_pModelDef->GetNumBones();i++) {
		m_BoneMatrices[i].Concatenate(transform);
		m_BoneMatrices[i].GetInverse(m_InvBoneMatrices[i]);
	}

	// update transform of boned props 
	// TODO, RC - ugh, we'll be doing this twice (for boned props, at least) - once here, 
	// and once again in SetTransform; better to just do it in Update? 
	for (size_t j=0;j<m_Props.size();j++) {
		const Prop& prop=m_Props[j];

		if (prop.m_Point->m_BoneIndex!=0xff) {
			CMatrix3D proptransform=prop.m_Point->m_Transform;;
			if (prop.m_Point->m_BoneIndex!=0xff) {
				proptransform.Concatenate(m_BoneMatrices[prop.m_Point->m_BoneIndex]);
			} else {			
				proptransform.Concatenate(transform);
			}
			prop.m_Model->SetTransform(proptransform);
		} 			
	}

	m_BoneMatricesValid=true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetAnimation: set the given animation as the current animation on this model;
// return false on error, else true
bool CModel::SetAnimation(CSkeletonAnim* anim, bool once)
{
	m_Anim=anim;
	if (m_Anim) {
		m_Flags &= ~MODELFLAG_NOLOOPANIMATION;
		if( once )
			m_Flags |= MODELFLAG_NOLOOPANIMATION;

		if (!m_BoneMatrices) {
			// not boned, can't animate
			return false;
		}

		if (anim->m_AnimDef->GetNumKeys()!=m_pModelDef->GetNumBones()) {
			// mismatch between models skeleton and animations skeleton
			return false;
		}

		// update object bounds to the bounds when given animation applied
		m_ObjectBounds=m_Anim->m_ObjectBounds;
		// start anim from beginning 
		m_AnimTime=0; 
	} 

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddProp: add a prop to the model on the given point
void CModel::AddProp(SPropPoint* point,CModel* model)
{
	// position model according to prop point position
	model->SetTransform(point->m_Transform);

	// check if we're already using this point, and replace
	// model on it if so
	uint i;
	for (i=0;i<m_Props.size();i++) {
		if (m_Props[i].m_Point==point) {
			delete m_Props[i].m_Model;
			m_Props[i].m_Model=model;
			return;
		}
	}

	// not using point; add new prop
	Prop prop;
	prop.m_Point=point;
	prop.m_Model=model;
	m_Props.push_back(prop);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RemoveProp: remove a prop from the given point
void CModel::RemoveProp(SPropPoint* point)
{
	typedef std::vector<Prop>::iterator Iter;
	for (Iter iter=m_Props.begin();iter!=m_Props.end();++iter) {
		const Prop& prop=*iter;
		if (prop.m_Point==point) {
			m_Props.erase(iter);
			return;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clone: return a clone of this model
CModel* CModel::Clone() const
{
	CModel* clone=new CModel;
	clone->m_ObjectBounds=m_ObjectBounds;
	clone->InitModel(m_pModelDef);
	clone->SetTexture(m_Texture);
    clone->SetMaterial(m_Material);
	clone->SetAnimation(m_Anim);
	clone->SetFlags(m_Flags);
	for (uint i=0;i<m_Props.size();i++) {
		// eek!  TODO, RC - need to investigate shallow clone here
		clone->AddProp(m_Props[i].m_Point,m_Props[i].m_Model->Clone());
	}
	return clone;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTransform: set the transform on this object, and reorientate props accordingly
void CModel::SetTransform(const CMatrix3D& transform) 
{
	// call base class to set transform on this object
	CRenderableObject::SetTransform(transform);
	m_BoneMatricesValid=false;

	// now set transforms on props
	const CMatrix3D* bonematrices=GetBoneMatrices();
	for (size_t i=0;i<m_Props.size();i++) {
		const Prop& prop=m_Props[i];

		CMatrix3D proptransform=prop.m_Point->m_Transform;;
		if (prop.m_Point->m_BoneIndex!=0xff) {
			proptransform.Concatenate(m_BoneMatrices[prop.m_Point->m_BoneIndex]);
		} else {
			proptransform.Concatenate(transform);
		}
		prop.m_Model->SetTransform(proptransform);
	}
}

void CModel::SetMaterial(const CMaterial &material)
{
    m_Material = material;
    if(m_Material.GetTexture().Trim(PS_TRIM_BOTH).Length() > 0)
    {
    }
}
