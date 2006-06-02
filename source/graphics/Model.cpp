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
#include "maths/Quaternion.h"
#include "maths/Bound.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h"
#include "SkeletonAnimManager.h"
#include "MeshManager.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/h_mgr.h"
#include "ps/Profile.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY "graphics"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
CModel::CModel() 
	: m_Parent(0), m_Flags(0), m_Anim(0), m_AnimTime(0), 
	m_BoneMatrices(0), m_InvTranspBoneMatrices(0),
	m_PositionValid(false), m_InvTranspValid(false), m_ShadingColor(1,1,1,1)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
CModel::~CModel()
{
	// Detach us from our parent
	if (m_Parent)
	{
		for(std::vector<Prop>::iterator iter = m_Parent->m_Props.begin();
		    iter != m_Parent->m_Props.end();
		    ++iter)
		{
			if (iter->m_Model == this)
			{
				m_Parent->m_Props.erase(iter);
				break;
			}
		}
		
		m_Parent = 0;
	}
	
	ReleaseData();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ReleaseData: delete anything allocated by the model
void CModel::ReleaseData()
{
	delete[] m_BoneMatrices;
	delete[] m_InvTranspBoneMatrices;
	for (size_t i=0;i<m_Props.size();i++) {
		m_Props[i].m_Model->m_Parent = 0;
		delete m_Props[i].m_Model;
	}
	m_Props.clear();
	m_pModelDef = CModelDefPtr();

	Handle h = m_Texture.GetHandle();
	ogl_tex_free(h);
	m_Texture.SetHandle(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InitModel: setup model from given geometry
bool CModel::InitModel(CModelDefPtr modeldef)
{
	// clean up any existing data first
	ReleaseData();

	m_pModelDef = modeldef;
	
	size_t numBones=modeldef->GetNumBones();
	if (numBones != 0) {
		// allocate matrices for bone transformations
		m_BoneMatrices=new CMatrix3D[numBones];
		m_InvTranspBoneMatrices=new CMatrix3D[numBones];
		// store default pose until animation assigned
		CBoneState* defpose=modeldef->GetBones();
		for (uint i=0;i<numBones;i++) {
			CMatrix3D& m=m_BoneMatrices[i];
			m.SetIdentity();
			m.Rotate(defpose[i].m_Rotation);
			m.Translate(defpose[i].m_Translation);
			m_InvTranspValid = false;
		}
	}

	m_PositionValid = true;
	
	return true;
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
	if (! (m_Anim && m_Anim->m_AnimDef))
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

	size_t numverts=m_pModelDef->GetNumVertices();
	SModelVertex* verts=m_pModelDef->GetVertices();

	for (size_t i=0;i<numverts;i++) {
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

	size_t numverts=m_pModelDef->GetNumVertices();
	SModelVertex* verts=m_pModelDef->GetVertices();

	// Remove any transformations, so that we calculate the bounding box
	// at the origin. The box is later re-transformed onto the object, without
	// having to recalculate the size of the box.
	CMatrix3D transform, oldtransform = GetTransform();
	CModel* oldparent = m_Parent;
	
	m_Parent = 0;
	transform.SetIdentity();
	CRenderableObject::SetTransform(transform);

	// Following seems to stomp over the current animation time - which, unsurprisingly,
	// introduces artefacts in the currently playing animation. Save it here and restore it
	// at the end.
	float AnimTime = m_AnimTime;

	// iterate through every frame of the animation
	for (size_t j=0;j<anim->GetNumFrames();j++) {
		m_PositionValid = false;
		ValidatePosition();

		// extend bounds by vertex positions at the frame
		for (size_t i=0;i<numverts;i++) {
			CVector3D tmp = SkinPoint(verts[i].m_Coords,verts[i].m_Blend,GetBoneMatrices());
			result+=tmp;
		}		
		// advance to next frame
		m_AnimTime += anim->GetFrameTime();
	}

	m_PositionValid = false;
	m_Parent = oldparent;
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
	if (m_Anim && m_Anim->m_AnimDef && m_BoneMatrices) {
		// adjust for animation speed
		float animtime=time*m_AnimSpeed;

		// update animation time, but don't calculate bone matrices - do that (lazily) when
		// something requests them; that saves some calculation work for offscreen models,
		// and also assures the world space, inverted bone matrices (required for normal
		// skinning) are up to date with respect to m_Transform 
		m_AnimTime += animtime;
		

		float duration=m_Anim->m_AnimDef->GetDuration();
		if (m_AnimTime > duration) {
			if (m_Flags & MODELFLAG_NOLOOPANIMATION)
				SetAnimation(m_NextAnim);
			else
				m_AnimTime = (float) fmod(m_AnimTime, duration);
		}
		
		// mark vertices as dirty
		SetDirty(RENDERDATA_UPDATE_VERTICES);
		
		// mark matrices as dirty
		InvalidatePosition();
	}

	// update props
	for (uint i=0; i<m_Props.size(); i++)
	{
		m_Props[i].m_Model->Update(time);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InvalidatePosition
void CModel::InvalidatePosition()
{
	m_PositionValid = false;

	for (uint i = 0; i < m_Props.size(); ++i)
		m_Props[i].m_Model->InvalidatePosition();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ValidatePosition: ensure that current transform and bone matrices are both uptodate
void CModel::ValidatePosition()
{
	if (m_PositionValid)
	{
		debug_assert(!m_Parent || m_Parent->m_PositionValid);
		return;
	}
	
	if (m_Parent && !m_Parent->m_PositionValid)
	{
		// Make sure we don't base our calculations on
		// a parent animation state that is out of date.
		m_Parent->ValidatePosition();
		
		// Parent will recursively call our validation.
		debug_assert(m_PositionValid);
		return;
	}

	if (m_Anim && m_BoneMatrices)
	{
		PROFILE( "generating bone matrices" );
	
		debug_assert(m_pModelDef->GetNumBones() == m_Anim->m_AnimDef->GetNumKeys());
	
		m_Anim->m_AnimDef->BuildBoneMatrices(m_AnimTime,m_BoneMatrices);
	
		const CMatrix3D& transform=GetTransform();
		for (size_t i=0;i<m_pModelDef->GetNumBones();i++) {
			m_BoneMatrices[i].Concatenate(transform);
		}
	}
	
	m_PositionValid = true;
	m_InvTranspValid = false;
	
	// re-position and validate all props
	for (size_t j = 0; j < m_Props.size(); ++j)
	{
		const Prop& prop=m_Props[j];

		CMatrix3D proptransform = prop.m_Point->m_Transform;;
		if (prop.m_Point->m_BoneIndex != 0xff)
			proptransform.Concatenate(m_BoneMatrices[prop.m_Point->m_BoneIndex]);
		else
			proptransform.Concatenate(m_Transform);
		
		prop.m_Model->SetTransform(proptransform);
		prop.m_Model->ValidatePosition();
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalcInvTranspBoneMatrices
void CModel::CalcInvTranspBoneMatrices()
{
	debug_assert(m_BoneMatrices);
	
	PROFILE( "invert transpose bone matrices" );
	
	CMatrix3D tmp;
	for(size_t i = 0; i < m_pModelDef->GetNumBones(); ++i)
	{
		m_BoneMatrices[i].GetInverse(tmp);
		tmp.GetTranspose(m_InvTranspBoneMatrices[i]);
	}
	
	m_InvTranspValid = true;
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

		if (!m_BoneMatrices && anim->m_AnimDef) {
			// not boned, can't animate
			return false;
		}

		if (m_BoneMatrices && !anim->m_AnimDef) {
			// boned, but animation isn't valid
			// (e.g. the default (static) idle animation on an animated unit)
			return false;
		}

		if (anim->m_AnimDef && anim->m_AnimDef->GetNumKeys() != m_pModelDef->GetNumBones()) {
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

	m_Anim = anim;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddProp: add a prop to the model on the given point
void CModel::AddProp(SPropPoint* point, CModel* model, CObjectEntry* objectentry)
{
	// position model according to prop point position
	model->SetTransform(point->m_Transform);
	model->m_Parent = this;

	// check if we're already using this point, and remove it if so
	// (when a prop is removed it will also remove the prop point)
	uint i;
	for (i = 0; i < m_Props.size(); i++) {
		if (m_Props[i].m_Point == point) {
			delete m_Props[i].m_Model;
			break;
		}
	}

	// not using point; add new prop
	Prop prop;
	prop.m_Point = point;
	prop.m_Model = model;
	prop.m_ObjectEntry = objectentry;
	m_Props.push_back(prop);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RemoveProp: remove a prop from the given point
void CModel::RemoveProp(SPropPoint* point)
{
	uint i;
	for (i=0;i<m_Props.size();i++) {
		if (m_Props[i].m_Point==point) {
			delete m_Props[i].m_Model;
			// (when a prop is removed it will automatically remove its prop point)
			break;
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
		clone->AddProp(m_Props[i].m_Point, m_Props[i].m_Model->Clone(), m_Props[i].m_ObjectEntry);
	}
	return clone;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTransform: set the transform on this object, and reorientate props accordingly
void CModel::SetTransform(const CMatrix3D& transform)
{
	// call base class to set transform on this object
	CRenderableObject::SetTransform(transform);
	InvalidateBounds();
	InvalidatePosition();
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

void CModel::SetShadingColor(CColor& colour)
{
	m_ShadingColor = colour;
	for (std::vector<Prop>::iterator it = m_Props.begin(); it != m_Props.end(); ++it)
		it->m_Model->SetShadingColor(colour);
}
