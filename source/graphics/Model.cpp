/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Mesh object with texture and skinning information
 */

#include "precompiled.h"

#include "Model.h"
#include "ModelDef.h"
#include "maths/Quaternion.h"
#include "maths/Bound.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h"
#include "SkeletonAnimManager.h"
#include "MeshManager.h"
#include "ObjectEntry.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/h_mgr.h"
#include "ps/Profile.h"

#include "ps/CLogger.h"
#define LOG_CATEGORY L"graphics"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
CModel::CModel(CSkeletonAnimManager& skeletonAnimManager)
	: m_Parent(NULL), m_Flags(0), m_Anim(NULL), m_AnimTime(0), 
	m_BoneMatrices(NULL), m_InverseBindBoneMatrices(NULL),
	m_PositionValid(false), m_ShadingColor(1,1,1,1),
	m_SkeletonAnimManager(skeletonAnimManager)
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
	delete[] m_InverseBindBoneMatrices;
	for (size_t i = 0; i < m_Props.size(); ++i)
	{
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
bool CModel::InitModel(const CModelDefPtr& modeldef)
{
	// clean up any existing data first
	ReleaseData();

	m_pModelDef = modeldef;
	
	size_t numBones = modeldef->GetNumBones();
	if (numBones != 0)
	{
		// allocate matrices for bone transformations
		m_BoneMatrices = new CMatrix3D[numBones];
		m_InverseBindBoneMatrices = new CMatrix3D[numBones];

		// store default pose until animation assigned
		CBoneState* defpose = modeldef->GetBones();
		for (size_t i = 0; i < numBones; ++i)
		{
			m_BoneMatrices[i].SetIdentity();
			m_BoneMatrices[i].Rotate(defpose[i].m_Rotation);
			m_BoneMatrices[i].Translate(defpose[i].m_Translation);

			m_InverseBindBoneMatrices[i].SetIdentity();
			m_InverseBindBoneMatrices[i].Translate(-defpose[i].m_Translation);
			m_InverseBindBoneMatrices[i].Rotate(defpose[i].m_Rotation.GetInverse());
		}
	}

	m_PositionValid = true;
	
	return true;
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
		for (size_t i=0;i<numverts;i++)
		{
			result += CModelDef::SkinPoint(verts[i], GetAnimatedBoneMatrices(), GetInverseBindBoneMatrices());
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
CSkeletonAnim* CModel::BuildAnimation(const VfsPath& pathname, const char* name, float speed, double actionpos, double actionpos2)
{
	CSkeletonAnimDef* def = m_SkeletonAnimManager.GetAnimation(pathname);
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
	if (m_Anim && m_Anim->m_AnimDef && m_BoneMatrices)
	{
		// adjust for animation speed
		float animTimeDelta = time*m_AnimSpeed;

		float oldAnimTime = m_AnimTime;

		// update animation time, but don't calculate bone matrices - do that (lazily) when
		// something requests them; that saves some calculation work for offscreen models,
		// and also assures the world space, inverted bone matrices (required for normal
		// skinning) are up to date with respect to m_Transform 
		m_AnimTime += animTimeDelta;
		
		float duration = m_Anim->m_AnimDef->GetDuration();
		if (m_AnimTime > duration)
		{
			if (m_Flags & MODELFLAG_NOLOOPANIMATION)
			{
				if (m_NextAnim)
					SetAnimation(m_NextAnim);
				else
				{
					// Changing to no animation - probably becoming a corpse.
					// Make sure the last displayed frame is the final frame
					// of the animation.
					float nearlyEnd = duration - 1.f; // 1 msec
					if (fabs(oldAnimTime - nearlyEnd) < 1.f)
						SetAnimation(NULL);
					else
						m_AnimTime = nearlyEnd;
				}
			}
			else
				m_AnimTime = fmod(m_AnimTime, duration);
		}
		
		// mark vertices as dirty
		SetDirty(RENDERDATA_UPDATE_VERTICES);
		
		// mark matrices as dirty
		InvalidatePosition();
	}

	// update props
	for (size_t i = 0; i < m_Props.size(); ++i)
		m_Props[i].m_Model->Update(time);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CModel::NeedsNewAnim(float time) const
{
	// TODO: fix UnitAnimation so it correctly loops animated props

	if (m_Anim && m_Anim->m_AnimDef && m_BoneMatrices)
	{
		// adjust for animation speed
		float animtime = time * m_AnimSpeed;

		float duration = m_Anim->m_AnimDef->GetDuration();
		if (m_AnimTime + animtime > duration)
			return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InvalidatePosition
void CModel::InvalidatePosition()
{
	m_PositionValid = false;

	for (size_t i = 0; i < m_Props.size(); ++i)
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
	
		m_Anim->m_AnimDef->BuildBoneMatrices(m_AnimTime, m_BoneMatrices, !(m_Flags & MODELFLAG_NOLOOPANIMATION));
	
		const CMatrix3D& transform=GetTransform();
		for (size_t i=0;i<m_pModelDef->GetNumBones();i++) {
			m_BoneMatrices[i].Concatenate(transform);
		}
	}
	
	m_PositionValid = true;
	
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
			LOG(CLogger::Error, LOG_CATEGORY, L"Mismatch between model's skeleton and animation's skeleton (%lu model bones != %lu animation keys)",
										(unsigned long)m_pModelDef->GetNumBones(), (unsigned long)anim->m_AnimDef->GetNumKeys());
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
// CopyAnimation
void CModel::CopyAnimationFrom(CModel* source)
{
	m_Anim = source->m_Anim;
	m_NextAnim = source->m_NextAnim;
	m_AnimTime = source->m_AnimTime;
	m_AnimSpeed = source->m_AnimSpeed;

	m_Flags &= ~MODELFLAG_CASTSHADOWS;
	if (source->m_Flags & MODELFLAG_CASTSHADOWS)
		m_Flags |= MODELFLAG_CASTSHADOWS;

	m_ObjectBounds.SetEmpty();
	InvalidateBounds();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddProp: add a prop to the model on the given point
void CModel::AddProp(SPropPoint* point, CModel* model, CObjectEntry* objectentry)
{
	// position model according to prop point position
	model->SetTransform(point->m_Transform);
	model->m_Parent = this;

	Prop prop;
	prop.m_Point = point;
	prop.m_Model = model;
	prop.m_ObjectEntry = objectentry;
	m_Props.push_back(prop);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RemoveProp: remove any props from the given point
void CModel::RemoveProp(SPropPoint* point)
{
	for (size_t i = 0; i < m_Props.size(); i++) {
		if (m_Props[i].m_Point == point) {
			delete m_Props[i].m_Model;
			// (when a prop is removed it will automatically remove its prop point)
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clone: return a clone of this model
CModel* CModel::Clone() const
{
	CModel* clone = new CModel(m_SkeletonAnimManager);
	clone->m_ObjectBounds = m_ObjectBounds;
	clone->InitModel(m_pModelDef);
	clone->SetTexture(m_Texture);
	if (m_Texture.GetHandle())
		h_add_ref(m_Texture.GetHandle());
	clone->SetMaterial(m_Material);
	clone->SetAnimation(m_Anim);
	clone->SetFlags(m_Flags);
	for (size_t i=0;i<m_Props.size();i++) {
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
	if(m_Material.GetTexture().Trim(PS_TRIM_BOTH).length() > 0)
	{
		// [TODO: uh, shouldn't this be doing something?]
	}
}

void CModel::SetPlayerID(size_t id)
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
	for( std::vector<Prop>::iterator it = m_Props.begin(); it != m_Props.end(); ++it )
	{
		it->m_Model->SetShadingColor(colour);
	}
}
