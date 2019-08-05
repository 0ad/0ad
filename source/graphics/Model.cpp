/* Copyright (C) 2019 Wildfire Games.
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

#include "Decal.h"
#include "ModelDef.h"
#include "maths/Quaternion.h"
#include "maths/BoundingBoxAligned.h"
#include "SkeletonAnim.h"
#include "SkeletonAnimDef.h"
#include "SkeletonAnimManager.h"
#include "MeshManager.h"
#include "ObjectEntry.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/h_mgr.h"
#include "lib/sysdep/rtl.h"
#include "ps/Profile.h"
#include "ps/CLogger.h"
#include "renderer/RenderingOptions.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpTerrain.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
CModel::CModel(CSkeletonAnimManager& skeletonAnimManager, CSimulation2& simulation)
	: m_Flags(0), m_Anim(NULL), m_AnimTime(0), m_Simulation(simulation),
	m_BoneMatrices(NULL), m_AmmoPropPoint(NULL), m_AmmoLoadedProp(0),
	m_SkeletonAnimManager(skeletonAnimManager)
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
	rtl_FreeAligned(m_BoneMatrices);

	for (size_t i = 0; i < m_Props.size(); ++i)
		delete m_Props[i].m_Model;
	m_Props.clear();

	m_pModelDef = CModelDefPtr();
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
		size_t numBlends = modeldef->GetNumBlends();

		// allocate matrices for bone transformations
		// (one extra matrix is used for the special case of bind-shape relative weighting)
		m_BoneMatrices = (CMatrix3D*)rtl_AllocateAligned(sizeof(CMatrix3D) * (numBones + 1 + numBlends), 16);
		for (size_t i = 0; i < numBones + 1 + numBlends; ++i)
		{
			m_BoneMatrices[i].SetIdentity();
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
	{
		if (m_ObjectBounds.IsEmpty())
			CalcStaticObjectBounds();
	}
	else
	{
		if (m_Anim->m_ObjectBounds.IsEmpty())
			CalcAnimatedObjectBounds(m_Anim->m_AnimDef, m_Anim->m_ObjectBounds);
		ENSURE(! m_Anim->m_ObjectBounds.IsEmpty()); // (if this happens, it'll be recalculating the bounds every time)
		m_ObjectBounds = m_Anim->m_ObjectBounds;
	}

	// Ensure the transform is set correctly before we use it
	ValidatePosition();

	// Now transform the object-space bounds to world-space bounds
	m_ObjectBounds.Transform(GetTransform(), m_WorldBounds);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalcObjectBounds: calculate object space bounds of this model, based solely on vertex positions
void CModel::CalcStaticObjectBounds()
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
void CModel::CalcAnimatedObjectBounds(CSkeletonAnimDef* anim, CBoundingBoxAligned& result)
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
	CModelAbstract* oldparent = m_Parent;

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
			result += CModelDef::SkinPoint(verts[i], GetAnimatedBoneMatrices());
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
const CBoundingBoxAligned CModel::GetWorldBoundsRec()
{
	CBoundingBoxAligned bounds = GetWorldBounds();
	for (size_t i = 0; i < m_Props.size(); ++i)
		bounds += m_Props[i].m_Model->GetWorldBoundsRec();
	return bounds;
}

const CBoundingBoxAligned CModel::GetObjectSelectionBoundsRec()
{
	CBoundingBoxAligned objBounds = GetObjectBounds();		// updates the (children-not-included) object-space bounds if necessary

	// now extend these bounds to include the props' selection bounds (if any)
	for (size_t i = 0; i < m_Props.size(); ++i)
	{
		const Prop& prop = m_Props[i];
		if (prop.m_Hidden || !prop.m_Selectable)
			continue; // prop is hidden from rendering, so it also shouldn't be used for selection

		CBoundingBoxAligned propSelectionBounds = prop.m_Model->GetObjectSelectionBoundsRec();
		if (propSelectionBounds.IsEmpty())
			continue;	// submodel does not wish to participate in selection box, exclude it

		// We have the prop's bounds in its own object-space; now we need to transform them so they can be properly added
		// to the bounds in our object-space. For that, we need the transform of the prop attachment point.
		//
		// We have the prop point information; however, it's not trivial to compute its exact location in our object-space
		// since it may or may not be attached to a bone (see SPropPoint), which in turn may or may not be in the middle of
		// an animation. The bone matrices might be of interest, but they're really only meant to be used for the animation
		// system and are quite opaque to use from the outside (see @ref ValidatePosition).
		//
		// However, a nice side effect of ValidatePosition is that it also computes the absolute world-space transform of
		// our props and sets it on their respective models. In particular, @ref ValidatePosition will compute the prop's
		// world-space transform as either
		//
		// T' = T x	B x O
		// or
		// T' = T x O
		//
		// where T' is the prop's world-space transform, T is our world-space transform, O is the prop's local
		// offset/rotation matrix, and B is an optional transformation matrix of the bone the prop is attached to
		// (taking into account animation and everything).
		//
		// From this, it is clear that either O or B x O is the object-space transformation matrix of the prop. So,
		// all we need to do is apply our own inverse world-transform T^(-1) to T' to get our desired result. Luckily,
		// this is precomputed upon setting the transform matrix (see @ref SetTransform), so it is free to fetch.

		CMatrix3D propObjectTransform = prop.m_Model->GetTransform(); // T'
		propObjectTransform.Concatenate(GetInvTransform()); // T^(-1) x T'

		// Transform the prop's bounds into our object coordinate space
		CBoundingBoxAligned transformedPropSelectionBounds;
		propSelectionBounds.Transform(propObjectTransform, transformedPropSelectionBounds);

		objBounds += transformedPropSelectionBounds;
	}

	return objBounds;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BuildAnimation: load raw animation frame animation from given file, and build a
// animation specific to this model
CSkeletonAnim* CModel::BuildAnimation(const VfsPath& pathname, const CStr& name, const CStr& ID, int frequency, float speed, float actionpos, float actionpos2, float soundpos)
{
	CSkeletonAnimDef* def = m_SkeletonAnimManager.GetAnimation(pathname);
	if (!def)
		return NULL;

	CSkeletonAnim* anim = new CSkeletonAnim();
	anim->m_Name = name;
	anim->m_ID = ID;
	anim->m_Frequency = frequency;
	anim->m_AnimDef = def;
	anim->m_Speed = speed;

	if (actionpos == -1.f)
		anim->m_ActionPos = -1.f;
	else
		anim->m_ActionPos = actionpos * anim->m_AnimDef->GetDuration();

	if (actionpos2 == -1.f)
		anim->m_ActionPos2 = -1.f;
	else
		anim->m_ActionPos2 = actionpos2 * anim->m_AnimDef->GetDuration();

	if (soundpos == -1.f)
		anim->m_SoundPos = -1.f;
	else
		anim->m_SoundPos = soundpos * anim->m_AnimDef->GetDuration();

	anim->m_ObjectBounds.SetEmpty();
	InvalidateBounds();

	return anim;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Update: update this model to the given time, in msec
void CModel::UpdateTo(float time)
{
	// update animation time, but don't calculate bone matrices - do that (lazily) when
	// something requests them; that saves some calculation work for offscreen models,
	// and also assures the world space, inverted bone matrices (required for normal
	// skinning) are up to date with respect to m_Transform
	m_AnimTime = time;

	// mark vertices as dirty
	SetDirty(RENDERDATA_UPDATE_VERTICES);

	// mark matrices as dirty
	InvalidatePosition();
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
		ENSURE(!m_Parent || m_Parent->m_PositionValid);
		return;
	}

	if (m_Parent && !m_Parent->m_PositionValid)
	{
		// Make sure we don't base our calculations on
		// a parent animation state that is out of date.
		m_Parent->ValidatePosition();

		// Parent will recursively call our validation.
		ENSURE(m_PositionValid);
		return;
	}

	if (m_Anim && m_BoneMatrices)
	{
//		PROFILE( "generating bone matrices" );

		ENSURE(m_pModelDef->GetNumBones() == m_Anim->m_AnimDef->GetNumKeys());

		m_Anim->m_AnimDef->BuildBoneMatrices(m_AnimTime, m_BoneMatrices, !(m_Flags & MODELFLAG_NOLOOPANIMATION));
	}
	else if (m_BoneMatrices)
	{
		// Bones but no animation - probably a buggy actor forgot to set up the animation,
		// so just render it in its bind pose

		for (size_t i = 0; i < m_pModelDef->GetNumBones(); i++)
		{
			m_BoneMatrices[i].SetIdentity();
			m_BoneMatrices[i].Rotate(m_pModelDef->GetBones()[i].m_Rotation);
			m_BoneMatrices[i].Translate(m_pModelDef->GetBones()[i].m_Translation);
		}
	}

	// For CPU skinning, we precompute as much as possible so that the only
	// per-vertex work is a single matrix*vec multiplication.
	// For GPU skinning, we try to minimise CPU work by doing most computation
	// in the vertex shader instead.
	// Using g_RenderingOptions to detect CPU vs GPU is a bit hacky,
	// and this doesn't allow the setting to change at runtime, but there isn't
	// an obvious cleaner way to determine what data needs to be computed,
	// and GPU skinning is a rarely-used experimental feature anyway.
	bool worldSpaceBoneMatrices = !g_RenderingOptions.GetGPUSkinning();
	bool computeBlendMatrices = !g_RenderingOptions.GetGPUSkinning();

	if (m_BoneMatrices && worldSpaceBoneMatrices)
	{
		// add world-space transformation to m_BoneMatrices
		const CMatrix3D transform = GetTransform();
		for (size_t i = 0; i < m_pModelDef->GetNumBones(); i++)
			m_BoneMatrices[i].Concatenate(transform);
	}

	// our own position is now valid; now we can safely update our props' positions without fearing
	// that doing so will cause a revalidation of this model (see recursion above).
	m_PositionValid = true;

	// re-position and validate all props
	for (size_t j = 0; j < m_Props.size(); ++j)
	{
		const Prop& prop=m_Props[j];

		CMatrix3D proptransform = prop.m_Point->m_Transform;

		if (prop.m_Point->m_BoneIndex != 0xff)
		{
			CMatrix3D boneMatrix = m_BoneMatrices[prop.m_Point->m_BoneIndex];
			if (!worldSpaceBoneMatrices)
				boneMatrix.Concatenate(GetTransform());
			proptransform.Concatenate(boneMatrix);
		}
		else
		{
			// not relative to any bone; just apply world-space transformation (i.e. relative to object-space origin)
			proptransform.Concatenate(m_Transform);
		}

		// Adjust prop height to terrain level when needed
		if (prop.m_MaxHeight != 0.f || prop.m_MinHeight != 0.f)
		{
			CVector3D propTranslation = proptransform.GetTranslation();
			CVector3D objTranslation = m_Transform.GetTranslation();

			CmpPtr<ICmpTerrain> cmpTerrain(m_Simulation, SYSTEM_ENTITY);
			if (cmpTerrain)
			{
				float objTerrain = cmpTerrain->GetExactGroundLevel(objTranslation.X, objTranslation.Z);
				float propTerrain = cmpTerrain->GetExactGroundLevel(propTranslation.X, propTranslation.Z);
				float translateHeight = std::min(prop.m_MaxHeight,
				                                 std::max(prop.m_MinHeight, propTerrain - objTerrain));
				CMatrix3D translate = CMatrix3D();
				translate.SetTranslation(0.f, translateHeight, 0.f);
				proptransform.Concatenate(translate);
			}
		}

		prop.m_Model->SetTransform(proptransform);
		prop.m_Model->ValidatePosition();
	}

	if (m_BoneMatrices)
	{
		for (size_t i = 0; i < m_pModelDef->GetNumBones(); i++)
		{
			m_BoneMatrices[i] = m_BoneMatrices[i] * m_pModelDef->GetInverseBindBoneMatrices()[i];
		}

		// Note: there is a special case of joint influence, in which the vertex
		//	is influenced by the bind-shape transform instead of a particular bone,
		//	which we indicate with the blending bone ID set to the total number
		//	of bones. But since we're skinning in world space, we use the model's
		//	world space transform and store that matrix in this special index.
		//	(see http://trac.wildfiregames.com/ticket/1012)
		m_BoneMatrices[m_pModelDef->GetNumBones()] = m_Transform;

		if (computeBlendMatrices)
			m_pModelDef->BlendBoneMatrices(m_BoneMatrices);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetAnimation: set the given animation as the current animation on this model;
// return false on error, else true
bool CModel::SetAnimation(CSkeletonAnim* anim, bool once)
{
	m_Anim = NULL; // in case something fails

	if (anim)
	{
		m_Flags &= ~MODELFLAG_NOLOOPANIMATION;

		if (once)
			m_Flags |= MODELFLAG_NOLOOPANIMATION;

		if (!m_BoneMatrices && anim->m_AnimDef)
		{
			// not boned, can't animate
			return false;
		}

		if (m_BoneMatrices && !anim->m_AnimDef)
		{
			// boned, but animation isn't valid
			// (e.g. the default (static) idle animation on an animated unit)
			return false;
		}

		if (anim->m_AnimDef && anim->m_AnimDef->GetNumKeys() != m_pModelDef->GetNumBones())
		{
			// mismatch between model's skeleton and animation's skeleton
			LOGERROR("Mismatch between model's skeleton and animation's skeleton (%lu model bones != %lu animation keys)",
					(unsigned long)m_pModelDef->GetNumBones(), (unsigned long)anim->m_AnimDef->GetNumKeys());
			return false;
		}

		// reset the cached bounds when the animation is changed
		m_ObjectBounds.SetEmpty();
		InvalidateBounds();

		// start anim from beginning
		m_AnimTime = 0;
	}

	m_Anim = anim;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CopyAnimation
void CModel::CopyAnimationFrom(CModel* source)
{
	m_Anim = source->m_Anim;
	m_AnimTime = source->m_AnimTime;

	m_Flags &= ~MODELFLAG_CASTSHADOWS;
	if (source->m_Flags & MODELFLAG_CASTSHADOWS)
		m_Flags |= MODELFLAG_CASTSHADOWS;

	m_ObjectBounds.SetEmpty();
	InvalidateBounds();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AddProp: add a prop to the model on the given point
void CModel::AddProp(const SPropPoint* point, CModelAbstract* model, CObjectEntry* objectentry, float minHeight, float maxHeight, bool selectable)
{
	// position model according to prop point position

	// this next call will invalidate the bounds of "model", which will in turn also invalidate the selection box
	model->SetTransform(point->m_Transform);
	model->m_Parent = this;

	Prop prop;
	prop.m_Point = point;
	prop.m_Model = model;
	prop.m_ObjectEntry = objectentry;
	prop.m_MinHeight = minHeight;
	prop.m_MaxHeight = maxHeight;
	prop.m_Selectable = selectable;
	m_Props.push_back(prop);
}

void CModel::AddAmmoProp(const SPropPoint* point, CModelAbstract* model, CObjectEntry* objectentry)
{
	AddProp(point, model, objectentry);
	m_AmmoPropPoint = point;
	m_AmmoLoadedProp = m_Props.size() - 1;
	m_Props[m_AmmoLoadedProp].m_Hidden = true;

	// we only need to invalidate the selection box here if it is based on props and their visibilities
	if (!m_CustomSelectionShape)
		m_SelectionBoxValid = false;
}

void CModel::ShowAmmoProp()
{
	if (m_AmmoPropPoint == NULL)
		return;

	// Show the ammo prop, hide all others on the same prop point
	for (size_t i = 0; i < m_Props.size(); ++i)
		if (m_Props[i].m_Point == m_AmmoPropPoint)
			m_Props[i].m_Hidden = (i != m_AmmoLoadedProp);

	//  we only need to invalidate the selection box here if it is based on props and their visibilities
	if (!m_CustomSelectionShape)
		m_SelectionBoxValid = false;
}

void CModel::HideAmmoProp()
{
	if (m_AmmoPropPoint == NULL)
		return;

	// Hide the ammo prop, show all others on the same prop point
	for (size_t i = 0; i < m_Props.size(); ++i)
		if (m_Props[i].m_Point == m_AmmoPropPoint)
			m_Props[i].m_Hidden = (i == m_AmmoLoadedProp);

	//  we only need to invalidate here if the selection box is based on props and their visibilities
	if (!m_CustomSelectionShape)
		m_SelectionBoxValid = false;
}

CModelAbstract* CModel::FindFirstAmmoProp()
{
	if (m_AmmoPropPoint)
		return m_Props[m_AmmoLoadedProp].m_Model;

	for (size_t i = 0; i < m_Props.size(); ++i)
	{
		CModel* propModel = m_Props[i].m_Model->ToCModel();
		if (propModel)
		{
			CModelAbstract* model = propModel->FindFirstAmmoProp();
			if (model)
				return model;
		}
	}

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clone: return a clone of this model
CModelAbstract* CModel::Clone() const
{
	CModel* clone = new CModel(m_SkeletonAnimManager, m_Simulation);
	clone->m_ObjectBounds = m_ObjectBounds;
	clone->InitModel(m_pModelDef);
	clone->SetMaterial(m_Material);
	clone->SetAnimation(m_Anim);
	clone->SetFlags(m_Flags);

	for (size_t i = 0; i < m_Props.size(); i++)
	{
		// eek!  TODO, RC - need to investigate shallow clone here
		if (m_AmmoPropPoint && i == m_AmmoLoadedProp)
			clone->AddAmmoProp(m_Props[i].m_Point, m_Props[i].m_Model->Clone(), m_Props[i].m_ObjectEntry);
		else
			clone->AddProp(m_Props[i].m_Point, m_Props[i].m_Model->Clone(), m_Props[i].m_ObjectEntry, m_Props[i].m_MinHeight, m_Props[i].m_MaxHeight, m_Props[i].m_Selectable);
	}

	return clone;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetTransform: set the transform on this object, and reorientate props accordingly
void CModel::SetTransform(const CMatrix3D& transform)
{
	// call base class to set transform on this object
	CRenderableObject::SetTransform(transform);
	InvalidatePosition();
}

//////////////////////////////////////////////////////////////////////////

void CModel::AddFlagsRec(int flags)
{
	m_Flags |= flags;

	if (flags & MODELFLAG_IGNORE_LOS)
	{
		m_Material.AddShaderDefine(str_IGNORE_LOS, str_1);
		m_Material.RecomputeCombinedShaderDefines();
	}

	for (size_t i = 0; i < m_Props.size(); ++i)
		if (m_Props[i].m_Model->ToCModel())
			m_Props[i].m_Model->ToCModel()->AddFlagsRec(flags);
}

void CModel::RemoveShadowsRec()
{
	m_Flags &= ~MODELFLAG_CASTSHADOWS;

	m_Material.AddShaderDefine(str_DISABLE_RECEIVE_SHADOWS, str_1);
	m_Material.RecomputeCombinedShaderDefines();

	for (size_t i = 0; i < m_Props.size(); ++i)
	{
		if (m_Props[i].m_Model->ToCModel())
			m_Props[i].m_Model->ToCModel()->RemoveShadowsRec();
		else if (m_Props[i].m_Model->ToCModelDecal())
			m_Props[i].m_Model->ToCModelDecal()->RemoveShadows();
	}
}

void CModel::SetMaterial(const CMaterial &material)
{
	m_Material = material;
}

void CModel::SetPlayerID(player_id_t id)
{
	CModelAbstract::SetPlayerID(id);

	for (std::vector<Prop>::iterator it = m_Props.begin(); it != m_Props.end(); ++it)
		it->m_Model->SetPlayerID(id);
}

void CModel::SetShadingColor(const CColor& color)
{
	CModelAbstract::SetShadingColor(color);

	for (std::vector<Prop>::iterator it = m_Props.begin(); it != m_Props.end(); ++it)
		it->m_Model->SetShadingColor(color);
}
