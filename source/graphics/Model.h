/* Copyright (C) 2013 Wildfire Games.
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

#include "graphics/Texture.h"
#include "graphics/Material.h"
#include "graphics/MeshManager.h"
#include "graphics/ModelAbstract.h"
#include "ps/Overlay.h"

struct SPropPoint;
class CObjectEntry;
class CSkeletonAnim;
class CSkeletonAnimDef;
class CSkeletonAnimManager;
class CSimulation2;

#define MODELFLAG_CASTSHADOWS		(1<<0)
#define MODELFLAG_NOLOOPANIMATION	(1<<1)
#define MODELFLAG_SILHOUETTE_DISPLAY	(1<<2)
#define MODELFLAG_SILHOUETTE_OCCLUDER	(1<<3)
#define MODELFLAG_IGNORE_LOS		(1<<4)

///////////////////////////////////////////////////////////////////////////////
// CModel: basically, a mesh object - holds the texturing and skinning 
// information for a model in game
class CModel : public CModelAbstract
{
	NONCOPYABLE(CModel);

public:
	struct Prop
	{
		Prop() : m_MinHeight(0.f), m_MaxHeight(0.f), m_Point(0), m_Model(0), m_ObjectEntry(0), m_Hidden(false), m_Selectable(true) {}

		float m_MinHeight;
		float m_MaxHeight;

		/**
		 * Location of the prop point within its parent model, relative to either a bone in the parent model or to the 
		 * parent model's origin. See the documentation for @ref SPropPoint for more details.
		 * @see SPropPoint
		 */
		const SPropPoint* m_Point;

		/**
		 * Pointer to the model associated with this prop. Note that the transform matrix held by this model is the full object-to-world
		 * space transform, taking into account all parent model positioning (see @ref CModel::ValidatePosition for positioning logic).
		 * @see CModel::ValidatePosition
		 */
		CModelAbstract* m_Model;
		CObjectEntry* m_ObjectEntry;

		bool m_Hidden; ///< Should this prop be temporarily removed from rendering?
		bool m_Selectable; /// < should this prop count in the selection size?
	};

public:
	// constructor
	CModel(CSkeletonAnimManager& skeletonAnimManager, CSimulation2& simulation);
	// destructor
	~CModel();


	/// Dynamic cast
	virtual CModel* ToCModel()
	{
		return this;
	}

	// setup model from given geometry
	bool InitModel(const CModelDefPtr& modeldef);
	// update this model's state; 'time' is the absolute time since the start of the animation, in MS
	void UpdateTo(float time);

	// get the model's geometry data
	const CModelDefPtr& GetModelDef() { return m_pModelDef; }

	// set the model's material
	void SetMaterial(const CMaterial &material);
	// set the model's player ID, recursively through props
	void SetPlayerID(player_id_t id);
	// set the models mod color
	virtual void SetShadingColor(const CColor& colour);
	// get the model's material
	CMaterial& GetMaterial() { return m_Material; }

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
	// add object flags, recursively through props
	void AddFlagsRec(int flags);
	// remove shadow casting and receiving, recursively through props
	// TODO: replace with more generic shader define + flags setting
	void RemoveShadowsRec();

	// recurse down tree setting dirty bits
	virtual void SetDirtyRec(int dirtyflags) {
		SetDirty(dirtyflags);
		for (size_t i=0;i<m_Props.size();i++) {
			m_Props[i].m_Model->SetDirtyRec(dirtyflags);
		}
	}

	virtual void SetTerrainDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1)
	{
		for (size_t i = 0; i < m_Props.size(); ++i)
			m_Props[i].m_Model->SetTerrainDirty(i0, j0, i1, j1);
	}

	virtual void SetEntityVariable(const std::string& name, float value)
	{
		for (size_t i = 0; i < m_Props.size(); ++i)
			m_Props[i].m_Model->SetEntityVariable(name, value);
	}

	// --- WORLD/OBJECT SPACE BOUNDS -----------------------------------------------------------------

	/// Overridden to calculate both the world-space and object-space bounds of this model, and stores the result in
	/// m_Bounds and m_ObjectBounds, respectively.
	virtual void CalcBounds();

	/// Returns the object-space bounds for this model, excluding its children.
	const CBoundingBoxAligned& GetObjectBounds()
	{
		RecalculateBoundsIfNecessary();				// recalculates both object-space and world-space bounds if necessary
		return m_ObjectBounds;
	}

	virtual const CBoundingBoxAligned GetWorldBoundsRec();		// reimplemented here

	/// Auxiliary method; calculates object space bounds of this model, based solely on vertex positions, and stores 
	/// the result in m_ObjectBounds. Called by CalcBounds (instead of CalcAnimatedObjectBounds) if it has been determined 
	/// that the object-space bounds are static.
	void CalcStaticObjectBounds();
	
	/// Auxiliary method; calculate object-space bounds encompassing all vertex positions for given animation, and stores 
	/// the result in m_ObjectBounds. Called by CalcBounds (instead of CalcStaticBounds) if it has been determined that the 
	/// object-space bounds need to take animations into account.
	void CalcAnimatedObjectBounds(CSkeletonAnimDef* anim,CBoundingBoxAligned& result);

	// --- SELECTION BOX/BOUNDS ----------------------------------------------------------------------

	/// Reimplemented here since proper models should participate in selection boxes.
	virtual const CBoundingBoxAligned GetObjectSelectionBoundsRec();

	/**
	 * Set transform of this object.
	 *
	 * @note In order to ensure that all child props are updated properly,
	 * you must call ValidatePosition().
	 */
	virtual void SetTransform(const CMatrix3D& transform);

	/**
	 * Return whether this is a skinned/skeletal model. If it is, Get*BoneMatrices()
	 * will return valid non-NULL arrays.
	 */
	bool IsSkinned() { return (m_BoneMatrices != NULL); }

	// return the models bone matrices; 16-byte aligned for SSE reads
	const CMatrix3D* GetAnimatedBoneMatrices() { 
		ENSURE(m_PositionValid);
		return m_BoneMatrices;
	}

	/**
	 * Load raw animation frame animation from given file, and build an
	 * animation specific to this model.
	 * @param pathname animation file to load
	 * @param name animation name (e.g. "idle")
	 * @param speed animation speed as a factor of the default animation speed
	 * @param actionpos offset of 'action' event, in range [0, 1]
	 * @param actionpos2 offset of 'action2' event, in range [0, 1]
	 * @param sound offset of 'sound' event, in range [0, 1]
	 * @return new animation, or NULL on error
	 */
	CSkeletonAnim* BuildAnimation(const VfsPath& pathname, const CStr& name, float speed, float actionpos, float actionpos2, float soundpos);

	/**
	 * Add a prop to the model on the given point.
	 */
	void AddProp(const SPropPoint* point, CModelAbstract* model, CObjectEntry* objectentry, float minHeight = 0.f, float maxHeight = 0.f, bool selectable = true);

	/**
	 * Add a prop to the model on the given point, and treat it as the ammo prop.
	 * The prop will be hidden by default.
	 */
	void AddAmmoProp(const SPropPoint* point, CModelAbstract* model, CObjectEntry* objectentry);

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
	CModelAbstract* FindFirstAmmoProp();

	// return prop list
	std::vector<Prop>& GetProps() { return m_Props; }
	const std::vector<Prop>& GetProps() const { return m_Props; }

	// return a clone of this model
	virtual CModelAbstract* Clone() const;

	/**
	 * Ensure that both the transformation and the bone
	 * matrices are correct for this model and all its props.
	 */
	virtual void ValidatePosition();

	/**
	 * Mark this model's position and bone matrices,
	 * and all props' positions as invalid.
	 */
	virtual void InvalidatePosition();

private:
	// delete anything allocated by the model
	void ReleaseData();
	
	// Needed for terrain aligned props
	CSimulation2& m_Simulation;

	// object flags
	int m_Flags;
	// model's material
	CMaterial m_Material;
	// pointer to the model's raw 3d data
	CModelDefPtr m_pModelDef;
	// object space bounds of model - accounts for bounds of all possible animations
	// that can play on a model. Not always up-to-date - currently CalcBounds()
	// updates it when necessary.
	CBoundingBoxAligned m_ObjectBounds;
	// animation currently playing on this model, if any
	CSkeletonAnim* m_Anim;
	// time (in MS) into the current animation
	float m_AnimTime;
	
	/**
	 * Current state of all bones on this model; null if associated modeldef isn't skeletal.
	 * Props may attach to these bones by means of the SPropPoint::m_BoneIndex field; in this case their
	 * transformation matrix held is relative to the bone transformation (see @ref SPropPoint and 
	 * @ref CModel::ValidatePosition).
	 * 
	 * @see SPropPoint
	 */
	CMatrix3D* m_BoneMatrices;
	// list of current props on model
	std::vector<Prop> m_Props;

	/**
	 * The prop point to which the ammo prop is attached, or NULL if none
	 */
	const SPropPoint* m_AmmoPropPoint;

	/**
	 * If m_AmmoPropPoint is not NULL, then the index in m_Props of the ammo prop
	 */
	size_t m_AmmoLoadedProp;

	// manager object which can load animations for us
	CSkeletonAnimManager& m_SkeletonAnimManager;
};

#endif
