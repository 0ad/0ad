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
#include "lib/res/ogl_tex.h"
#include "lib/res/h_mgr.h"
#include "Profile.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "graphics"

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

	Handle h = m_Texture.GetHandle();
	tex_free(h);
	m_Texture.SetHandle(0);
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
	debug_assert(blend.m_Bone[0]!=0xff);

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
	// Need to calculate the object bounds first, if that hasn't already been done
	if (! m_Anim)
		CalcObjectBounds();
	else
	{
		if (m_Anim->m_ObjectBounds.IsEmpty())
			CalcAnimatedObjectBound(m_Anim->m_AnimDef, m_Anim->m_ObjectBounds);
		debug_assert(! m_Anim->m_ObjectBounds.IsEmpty()); // (if this happens, it'll be recalculating the bounds every time)
		m_ObjectBounds = m_Anim->m_ObjectBounds;
	}

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

	// Set the current animation on which to perform calculations (if it's necessary)
	if (anim != m_Anim->m_AnimDef)
	{
		CSkeletonAnim dummyanim;
		dummyanim.m_AnimDef=anim;
		if (!SetAnimation(&dummyanim)) return;
	}

	int numverts=m_pModelDef->GetNumVertices();
	SModelVertex* verts=m_pModelDef->GetVertices();

	// Remove any transformations, so that we calculate the bounding box
	// at the origin. The box is later re-transformed onto the object, without
	// having to recalculate the size of the box.
	CMatrix3D transform, oldtransform = GetTransform();
	transform.SetIdentity();
	SetTransform(transform);

	// Following seems to stomp over the current animation time - which, unsurprisingly,
	// introduces artefacts in the currently playing animation. Save it here and restore it
	// at the end.
	float AnimTime = m_AnimTime;

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

	SetTransform(oldtransform);
	m_AnimTime = AnimTime;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildAnimation: load raw animation frame animation from given file, and build a 
// animation specific to this model
CSkeletonAnim* CModel::BuildAnimation(const char* filename, const char* name, float speed, double actionpos, double actionpos2)
{
	CSkeletonAnimDef* def=g_SkelAnimMan.GetAnimation(filename);
	if (!def) return NULL;


	CSkeletonAnim* anim=new CSkeletonAnim;
	anim->m_Name = name;
	anim->m_AnimDef=def;
	anim->m_Speed=speed;
	anim->m_ActionPos=(float)( actionpos /* * anim->m_AnimDef->GetDuration() */ / speed );
	anim->m_ActionPos2=(float)( actionpos2 /* * anim->m_AnimDef->GetDuration() */ / speed );

	anim->m_ObjectBounds.SetEmpty();
	InvalidateBounds();

	return anim;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Update: update this model by the given time, in seconds
void CModel::Update(float time)
{
	if (m_Anim && m_BoneMatrices) {
		// adjust for animation speed
		float animtime=time*m_AnimSpeed;

		// update animation time, but don't calculate bone matrices - do that (lazily) when
		// something requests them; that saves some calculation work for offscreen models,
		// and also assures the world space, inverted bone matrices (required for normal
		// skinning) are up to date with respect to m_Transform 
		m_AnimTime += animtime;
		

		float duration=m_Anim->m_AnimDef->GetDuration();
		if (m_AnimTime > duration) {
			if( m_Flags & MODELFLAG_NOLOOPANIMATION )
			{
				SetAnimation( m_NextAnim );
			}
			else
				m_AnimTime=(float) fmod(m_AnimTime,duration);

		}
		
		// mark vertices as dirty
		SetDirty(RENDERDATA_UPDATE_VERTICES);
		
		// mark matrices as dirty
		m_BoneMatricesValid = false;
	}

	// update props
	for (uint i=0; i<m_Props.size(); i++) {
		m_Props[i].m_Model->Update(time);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GenerateBoneMatrices: calculate necessary bone transformation matrices for skinning
void CModel::GenerateBoneMatrices()
{
	if (!m_Anim || !m_BoneMatrices) return;

	PROFILE( "generating bone matrices" );

	debug_assert(m_pModelDef->GetNumBones() == m_Anim->m_AnimDef->GetNumKeys());

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
bool CModel::SetAnimation(CSkeletonAnim* anim, bool once, float speed, CSkeletonAnim* next)
{
	m_Anim=NULL; // in case something fails

	if (anim) {
		m_Flags &= ~MODELFLAG_NOLOOPANIMATION;
		if (once)
		{
			m_Flags |= MODELFLAG_NOLOOPANIMATION;
			m_NextAnim = next;
		}

		if (!m_BoneMatrices) {
			// not boned, can't animate
			return false;
		}

		if (anim->m_AnimDef->GetNumKeys() != m_pModelDef->GetNumBones()) {
			// mismatch between model's skeleton and animation's skeleton
			LOG(ERROR, LOG_CATEGORY, "Mismatch between model's skeleton and animation's skeleton (%d model bones != %d animation keys)",
										m_pModelDef->GetNumBones(), anim->m_AnimDef->GetNumKeys());
			return false;
		}

		// reset the cached bounds when the animation is changed
		m_ObjectBounds.SetEmpty();
		InvalidateBounds();

		// start anim from beginning 
		m_AnimTime=0;

		// Adjust speed by animation base rate.
		m_AnimSpeed = speed * anim->m_Speed;
	} 

	m_Anim=anim;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddProp: add a prop to the model on the given point
void CModel::AddProp(SPropPoint* point, CModel* model)
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
			delete prop.m_Model;
			m_Props.erase(iter);
			return;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clone: return a clone of this model
CModel* CModel::Clone() const
{
	CModel* clone = new CModel;
	clone->m_ObjectBounds = m_ObjectBounds;
	clone->InitModel(m_pModelDef);
	clone->SetTexture(m_Texture);
	if (m_Texture.GetHandle())
		h_add_ref(m_Texture.GetHandle());
	clone->SetMaterial(m_Material);
	clone->SetAnimation(m_Anim);
	clone->SetFlags(m_Flags);
	for (uint i=0;i<m_Props.size();i++) {
		// eek!  TODO, RC - need to investigate shallow clone here
		clone->AddProp(m_Props[i].m_Point, m_Props[i].m_Model->Clone());
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
	InvalidateBounds();

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

//////////////////////////////////////////////////////////////////////////

void CModel::SetMaterial(const CMaterial &material)
{
	m_Material = material;
	if(m_Material.GetTexture().Trim(PS_TRIM_BOTH).Length() > 0)
	{
	}
}

void CModel::SetPlayerID(int id)
{
	m_Material.SetPlayerColor(id);
	for (std::vector<Prop>::iterator it = m_Props.begin(); it != m_Props.end(); ++it)
		it->m_Model->SetPlayerID(id);
}

void CModel::SetPlayerColor(CColor& colour)
{
	m_Material.SetPlayerColor(colour);
}
