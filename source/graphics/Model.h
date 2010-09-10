/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_MODEL
#define INCLUDED_MODEL

#include <vector>

#include "Texture.h"
#include "MeshManager.h"
#include "RenderableObject.h"
#include "Material.h"
#include "ps/Overlay.h"

struct SPropPoint;
class CObjectEntry;
class CSkeletonAnim;
class CSkeletonAnimDef;
class CSkeletonAnimManager;

#define MODELFLAG_CASTSHADOWS		(1<<0)
#define MODELFLAG_NOLOOPANIMATION	(1<<1)

///////////////////////////////////////////////////////////////////////////////
// CModel: basically, a mesh object - holds the texturing and skinning 
// information for a model in game
class CModel : public CRenderableObject
{
	NONCOPYABLE(CModel);

public:
	struct Prop
	{
		Prop() : m_Point(0), m_Model(0), m_ObjectEntry(0), m_Hidden(false) {}

		SPropPoint* m_Point;
		CModel* m_Model;
		CObjectEntry* m_ObjectEntry;

		bool m_Hidden; // temporarily removed from rendering
	};

public:
	// constructor
	CModel(CSkeletonAnimManager& skeletonAnimManager);
	// destructor
	~CModel();

	// setup model from given geometry
	bool InitModel(const CModelDefPtr& modeldef);
	// calculate the world space bounds of this model
	void CalcBounds();
	// update this model's state; 'time' is the absolute time since the start of the animation, in MS
	void UpdateTo(float time);

	// get the model's geometry data
	CModelDefPtr GetModelDef() { return m_pModelDef; }

	// set the model's texture
	void SetTexture(const CTexturePtr& tex) { m_Texture=tex; }
	// set the model's material
	void SetMaterial(const CMaterial &material);
	// set the model's player ID, recursively through props
	void SetPlayerID(size_t id);
	// get the model's player ID, recursively through props; initial default is (size_t)-1
	size_t GetPlayerID();
	// set the model's player colour
	void SetPlayerColor(const CColor& colour);
	// set the models mod color
	void SetShadingColor(const CColor& colour);
	// get the model's texture
	CTexturePtr& GetTexture() { return m_Texture; }
	// get the model's material
	CMaterial& GetMaterial() { return m_Material; }
	// get the model's shading color
	CColor GetShadingColor() { return m_ShadingColor; }

	// set the given animation as the current animation on this model
	bool SetAnimation(CSkeletonAnim* anim, bool once = false);

	// get the currently playing animation, if any
	CSkeletonAnim* GetAnimation() const { return m_Anim; }

	// set the animation state to be the same as from another; both models should
	// be compatible types (same type of skeleton)
	void CopyAnimationFrom(CModel* source);

	// set object flags
	void SetFlags(int flags) { m_Flags=flags; }
	// get object flags
	int GetFlags() const { return m_Flags; }

	// recurse down tree setting dirty bits
	void SetDirtyRec(int dirtyflags) {
		SetDirty(dirtyflags);
		for (size_t i=0;i<m_Props.size();i++) {
			m_Props[i].m_Model->SetDirtyRec(dirtyflags);
		}
	}

	// calculate object space bounds of this model, based solely on vertex positions
	void CalcObjectBounds();
	// calculate bounds encompassing all vertex positions for given animation 
	void CalcAnimatedObjectBound(CSkeletonAnimDef* anim,CBound& result);

	/**
	 * Set transform of this object.
	 * 
	 * @note In order to ensure that all child props are updated properly,
	 * you must call ValidatePosition().
	 */
	void SetTransform(const CMatrix3D& transform);
	
	/**
	 * Return whether this is a skinned/skeletal model. If it is, Get*BoneMatrices()
	 * will return valid non-NULL arrays.
	 */
	bool IsSkinned() { return (m_BoneMatrices != NULL); }

	// return the models bone matrices
	const CMatrix3D* GetAnimatedBoneMatrices() { 
		debug_assert(m_PositionValid);
		return m_BoneMatrices;
	}
	const CMatrix3D* GetInverseBindBoneMatrices() { 
		return m_InverseBindBoneMatrices;
	}

	/**
	 * Load raw animation frame animation from given file, and build an
	 * animation specific to this model.
	 * @param pathname animation file to load
	 * @param name animation name (e.g. "idle")
	 * @param speed animation speed as a factor of the default animation speed
	 * @param actionpos offset of 'action' event, in range [0, 1]
	 * @param actionpos2 offset of 'action2' event, in range [0, 1]
	 * @return new animation, or NULL on error
	 */
	CSkeletonAnim* BuildAnimation(const VfsPath& pathname, const char* name, float speed, float actionpos, float actionpos2);

	/**
	 * Add a prop to the model on the given point.
	 */
	void AddProp(SPropPoint* point, CModel* model, CObjectEntry* objectentry);

	/**
	 * Add a prop to the model on the given point, and treat it as the ammo prop.
	 * The prop will be hidden by default.
	 */
	void AddAmmoProp(SPropPoint* point, CModel* model, CObjectEntry* objectentry);

	/**
	 * Show the ammo prop (if any), and hide any other props on that prop point.
	 */
	void ShowAmmoProp();

	/**
	 * Hide the ammo prop (if any), and show any other props on that prop point.
	 */
	void HideAmmoProp();

	/**
	 * Find the first prop used for ammo, by this model or its own props.
	 */
	CModel* FindFirstAmmoProp();

	// return prop list
	std::vector<Prop>& GetProps() { return m_Props; }
	const std::vector<Prop>& GetProps() const { return m_Props; }

	// return a clone of this model
	CModel* Clone() const;

	/**
	 * Ensure that both the transformation and the bone
	 * matrices are correct for this model and all its props.
	 */
	void ValidatePosition();

private:
	// delete anything allocated by the model
	void ReleaseData();

	/**
	 * Mark this model's position and bone matrices,
	 * and all props' positions as invalid.
	 */
	void InvalidatePosition();
	
	/**
	 * If non-null, m_Parent points to the model that we
	 * are attached to.
	 */
	CModel* m_Parent;

	// object flags
	int m_Flags;
	// texture used by model
	CTexturePtr m_Texture;
	// model's material
	CMaterial m_Material;
	// pointer to the model's raw 3d data
	CModelDefPtr m_pModelDef;
	// object space bounds of model - accounts for bounds of all possible animations
	// that can play on a model. Not always up-to-date - currently CalcBounds()
	// updates it when necessary.
	CBound m_ObjectBounds;
	// animation currently playing on this model, if any
	CSkeletonAnim* m_Anim;
	// time (in MS) into the current animation
	float m_AnimTime;
	// current state of all bones on this model; null if associated modeldef isn't skeletal
	CMatrix3D* m_BoneMatrices;
	// inverse matrices for the bind pose's bones; null if not skeletal
	CMatrix3D* m_InverseBindBoneMatrices;
	// list of current props on model
	std::vector<Prop> m_Props;

	/**
	 * The prop point to which the ammo prop is attached, or NULL if none
	 */
	SPropPoint* m_AmmoPropPoint;

	/**
	 * If m_AmmoPropPoint is not NULL, then the index in m_Props of the ammo prop
	 */
	size_t m_AmmoLoadedProp;

	/**
	 * true if both transform and and bone matrices are valid.
	 */
	bool m_PositionValid;

	// modulating color
	CColor m_ShadingColor;

	size_t m_PlayerID;

	// manager object which can load animations for us
	CSkeletonAnimManager& m_SkeletonAnimManager;
};

#endif
