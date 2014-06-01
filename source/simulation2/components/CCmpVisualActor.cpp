/* Copyright (C) 2014 Wildfire Games.
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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpVisual.h"

#include "simulation2/MessageTypes.h"

#include "ICmpFootprint.h"
#include "ICmpUnitRenderer.h"
#include "ICmpOwnership.h"
#include "ICmpPosition.h"
#include "ICmpTemplateManager.h"
#include "ICmpTerrain.h"
#include "ICmpUnitMotion.h"
#include "ICmpValueModificationManager.h"
#include "ICmpVision.h"

#include "graphics/Decal.h"
#include "graphics/Frustum.h"
#include "graphics/Model.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "graphics/Unit.h"
#include "graphics/UnitAnimation.h"
#include "graphics/UnitManager.h"
#include "maths/BoundingSphere.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/GameSetup/Config.h"
#include "renderer/Scene.h"

class CCmpVisualActor : public ICmpVisual
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update_Final);
		componentManager.SubscribeToMessageType(MT_InterpolatedPositionChanged);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_ValueModification);
		componentManager.SubscribeToMessageType(MT_TerrainChanged);
		componentManager.SubscribeToMessageType(MT_Destroy);
	}

	DEFAULT_COMPONENT_ALLOCATOR(VisualActor)

private:
	std::wstring m_BaseActorName, m_ActorName;
	bool m_IsFoundationActor;
	CUnit* m_Unit;

	fixed m_R, m_G, m_B; // shading colour

	std::map<std::string, std::string> m_AnimOverride;

	// Current animation state
	fixed m_AnimRunThreshold; // if non-zero this is the special walk/run mode
	std::string m_AnimName;
	bool m_AnimOnce;
	fixed m_AnimSpeed;
	std::wstring m_SoundGroup;
	fixed m_AnimDesync;
	fixed m_AnimSyncRepeatTime; // 0.0 if not synced

	u32 m_Seed; // seed used for random variations

	bool m_ConstructionPreview;

	bool m_VisibleInAtlasOnly;
	bool m_IsActorOnly;	// an in-world entity should not have this or it might not be rendered.

	ICmpUnitRenderer::tag_t m_ModelTag;

public:
	static std::string GetSchema()
	{
		return
			"<a:help>Display the unit using the engine's actor system.</a:help>"
			"<a:example>"
				"<Actor>units/hellenes/infantry_spearman_b.xml</Actor>"
			"</a:example>"
			"<a:example>"
				"<Actor>structures/hellenes/barracks.xml</Actor>"
				"<FoundationActor>structures/fndn_4x4.xml</FoundationActor>"
			"</a:example>"
			"<element name='Actor' a:help='Filename of the actor to be used for this unit'>"
				"<text/>"
			"</element>"
			"<optional>"
				"<element name='FoundationActor' a:help='Filename of the actor to be used the foundation while this unit is being constructed'>"
					"<text/>"
				"</element>"
			"</optional>"
			"<optional>"
				"<element name='Foundation' a:help='Used internally; if present, the unit will be rendered as a foundation'>"
					"<empty/>"
				"</element>"
			"</optional>"
			"<optional>"
				"<element name='ConstructionPreview' a:help='If present, the unit should have a construction preview'>"
					"<empty/>"
				"</element>"
			"</optional>"
			"<optional>"
				"<element name='DisableShadows' a:help='Used internally; if present, shadows will be disabled'>"
					"<empty/>"
				"</element>"
			"</optional>"
			"<optional>"
				"<element name='ActorOnly' a:help='Used internally; if present, the unit will only be rendered if the user has high enough graphical settings.'>"
					"<empty/>"
				"</element>"
			"</optional>"
			"<element name='SilhouetteDisplay'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='SilhouetteOccluder'>"
				"<data type='boolean'/>"
			"</element>"
			"<optional>"
				"<element name='SelectionShape'>"
					"<choice>"
						"<element name='Bounds' a:help='Determines the selection box based on the model bounds'>"
							"<empty/>"
						"</element>"
						"<element name='Footprint' a:help='Determines the selection box based on the entity Footprint component'>"
							"<empty/>"
						"</element>"
						"<element name='Box' a:help='Sets the selection shape to a box of specified dimensions'>"
							"<attribute name='width'>"
								"<ref name='positiveDecimal' />"
							"</attribute>"
							"<attribute name='height'>"
								"<ref name='positiveDecimal' />"
							"</attribute>"
							"<attribute name='depth'>"
								"<ref name='positiveDecimal' />"
							"</attribute>"
						"</element>"
						"<element name='Cylinder' a:help='Sets the selection shape to a cylinder of specified dimensions'>"
							"<attribute name='radius'>"
								"<ref name='positiveDecimal' />"
							"</attribute>"
							"<attribute name='height'>"
								"<ref name='positiveDecimal' />"
							"</attribute>"
						"</element>"
					"</choice>"
				"</element>"
			"</optional>"
			"<element name='VisibleInAtlasOnly'>"
				"<data type='boolean'/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		m_Unit = NULL;
		m_R = m_G = m_B = fixed::FromInt(1);

		m_ConstructionPreview = paramNode.GetChild("ConstructionPreview").IsOk();

		m_Seed = GetEntityId();

		m_IsFoundationActor = paramNode.GetChild("Foundation").IsOk() && paramNode.GetChild("FoundationActor").IsOk();
		if (m_IsFoundationActor)
			m_BaseActorName = m_ActorName = paramNode.GetChild("FoundationActor").ToString();
		else
			m_BaseActorName = m_ActorName = paramNode.GetChild("Actor").ToString();

		m_VisibleInAtlasOnly = paramNode.GetChild("VisibleInAtlasOnly").ToBool();
		m_IsActorOnly = paramNode.GetChild("ActorOnly").IsOk();
		
		InitModel(paramNode);

		// We need to select animation even if graphics are disabled, as this modifies serialized state
		SelectAnimation("idle", false, fixed::FromInt(1), L"");
	}

	virtual void Deinit()
	{
		if (m_Unit)
		{
			GetSimContext().GetUnitManager().DeleteUnit(m_Unit);
			m_Unit = NULL;
		}
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.NumberFixed_Unbounded("r", m_R);
		serialize.NumberFixed_Unbounded("g", m_G);
		serialize.NumberFixed_Unbounded("b", m_B);

		serialize.NumberFixed_Unbounded("anim run threshold", m_AnimRunThreshold);
		serialize.StringASCII("anim name", m_AnimName, 0, 256);
		serialize.Bool("anim once", m_AnimOnce);
		serialize.NumberFixed_Unbounded("anim speed", m_AnimSpeed);
		serialize.String("sound group", m_SoundGroup, 0, 256);
		serialize.NumberFixed_Unbounded("anim desync", m_AnimDesync);
		serialize.NumberFixed_Unbounded("anim sync repeat time", m_AnimSyncRepeatTime);

		serialize.NumberU32_Unbounded("seed", m_Seed);
		// TODO: variation/selection strings
		serialize.String("actor", m_ActorName, 0, 256);

		// TODO: store actor variables?
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO: store the actor name, if !debug and it differs from the template

		if (serialize.IsDebug())
		{
			serialize.String("base actor", m_BaseActorName, 0, 256);
		}

		SerializeCommon(serialize);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		u32 oldSeed = GetActorSeed();

		SerializeCommon(deserialize);

		// If we serialized a different seed or different actor, reload actor
		if (oldSeed != GetActorSeed() || m_BaseActorName != m_ActorName)
			ReloadActor();

		fixed repeattime = m_AnimSyncRepeatTime; // save because SelectAnimation overwrites it

		if (m_AnimRunThreshold.IsZero())
			SelectAnimation(m_AnimName, m_AnimOnce, m_AnimSpeed, m_SoundGroup);
		else
			SelectMovementAnimation(m_AnimRunThreshold);

		SetAnimationSyncRepeat(repeattime);

		if (m_Unit)
		{
			CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
			if (cmpOwnership)
				m_Unit->GetModel().SetPlayerID(cmpOwnership->GetOwner());
		}
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		// Quick exit for running in non-graphical mode
		if (m_Unit == NULL)
			return;

		switch (msg.GetType())
		{
		case MT_Update_Final:
		{
			const CMessageUpdate_Final& msgData = static_cast<const CMessageUpdate_Final&> (msg);
			Update(msgData.turnLength);
			break;
		}
		case MT_OwnershipChanged:
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			m_Unit->GetModel().SetPlayerID(msgData.to);
			break;
		}
		case MT_TerrainChanged:
		{
			const CMessageTerrainChanged& msgData = static_cast<const CMessageTerrainChanged&> (msg);
			m_Unit->GetModel().SetTerrainDirty(msgData.i0, msgData.j0, msgData.i1, msgData.j1);
			break;
		}
		case MT_ValueModification:
		{
			const CMessageValueModification& msgData = static_cast<const CMessageValueModification&> (msg);
			if (msgData.component != L"VisualActor")
				break;
			CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSystemEntity());
			std::wstring newActorName;
			if (m_IsFoundationActor)
				newActorName = cmpValueModificationManager->ApplyModifications(L"VisualActor/FoundationActor", m_BaseActorName, GetEntityId());
			else
				newActorName = cmpValueModificationManager->ApplyModifications(L"VisualActor/Actor", m_BaseActorName, GetEntityId());
			if (newActorName != m_ActorName)
			{
				m_ActorName = newActorName;
				ReloadActor();
			}
			break;
		}
		case MT_InterpolatedPositionChanged:
		{
			const CMessageInterpolatedPositionChanged& msgData = static_cast<const CMessageInterpolatedPositionChanged&> (msg);
			if (m_ModelTag.valid())
			{
				CmpPtr<ICmpUnitRenderer> cmpModelRenderer(GetSystemEntity());
				cmpModelRenderer->UpdateUnitPos(m_ModelTag, msgData.inWorld, msgData.pos0, msgData.pos1);
			}
			break;
		}
		case MT_Destroy:
		{
			if (m_ModelTag.valid())
			{
				CmpPtr<ICmpUnitRenderer> cmpModelRenderer(GetSystemEntity());
				cmpModelRenderer->RemoveUnit(m_ModelTag);
				m_ModelTag = ICmpUnitRenderer::tag_t();
			}
			break;
		}
		}
	}

	virtual CBoundingBoxAligned GetBounds()
	{
		if (!m_Unit)
			return CBoundingBoxAligned::EMPTY;
		return m_Unit->GetModel().GetWorldBounds();
	}

	virtual CUnit* GetUnit()
	{
		return m_Unit;
	}

	virtual CBoundingBoxOriented GetSelectionBox()
	{
		if (!m_Unit)
			return CBoundingBoxOriented::EMPTY;
		return m_Unit->GetModel().GetSelectionBox();
	}

	virtual CVector3D GetPosition()
	{
		if (!m_Unit)
			return CVector3D(0, 0, 0);
		return m_Unit->GetModel().GetTransform().GetTranslation();
	}

	virtual std::wstring GetActorShortName()
	{
		if (!m_Unit)
			return L"";
		return m_Unit->GetObject().m_Base->m_ShortName;
	}

	virtual std::wstring GetProjectileActor()
	{
		if (!m_Unit)
			return L"";
		return m_Unit->GetObject().m_ProjectileModelName;
	}

	virtual CVector3D GetProjectileLaunchPoint()
	{
		if (!m_Unit)
			return CVector3D();

		if (m_Unit->GetModel().ToCModel())
		{
			// Ensure the prop transforms are correct
			CmpPtr<ICmpUnitRenderer> cmpUnitRenderer(GetSystemEntity());
			CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
			if (cmpUnitRenderer && cmpPosition)
			{
				float frameOffset = cmpUnitRenderer->GetFrameOffset();
				CMatrix3D transform(cmpPosition->GetInterpolatedTransform(frameOffset));
				m_Unit->GetModel().SetTransform(transform);
				m_Unit->GetModel().ValidatePosition();
			}

			CModelAbstract* ammo = m_Unit->GetModel().ToCModel()->FindFirstAmmoProp();
			if (ammo)
				return ammo->GetTransform().GetTranslation();
		}

		return CVector3D();
	}

	virtual void SelectAnimation(std::string name, bool once, fixed speed, std::wstring soundgroup)
	{
		m_AnimRunThreshold = fixed::Zero();
		m_AnimName = name;
		m_AnimOnce = once;
		m_AnimSpeed = speed;
		m_SoundGroup = soundgroup;
		m_AnimDesync = fixed::FromInt(1)/20; // TODO: make this an argument
		m_AnimSyncRepeatTime = fixed::Zero();

		if (m_Unit)
		{
			m_Unit->SetEntitySelection(m_AnimName);
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->SetAnimationState(m_AnimName, m_AnimOnce, m_AnimSpeed.ToFloat(), m_AnimDesync.ToFloat(), m_SoundGroup.c_str());
		}
	}

	virtual void ReplaceMoveAnimation(std::string name, std::string replace)
	{
		m_AnimOverride[name] = replace;
	}

	virtual void ResetMoveAnimation(std::string name)
	{
		std::map<std::string, std::string>::const_iterator it = m_AnimOverride.find(name);
		if (it != m_AnimOverride.end())
			m_AnimOverride.erase(name);
	}

	virtual void SetUnitEntitySelection(const CStr& selection)
	{
		if (m_Unit)
		{
			m_Unit->SetEntitySelection(selection);
		}
	}

	virtual void SelectMovementAnimation(fixed runThreshold)
	{
		m_AnimRunThreshold = runThreshold;

		if (m_Unit)
		{
			m_Unit->SetEntitySelection("walk");
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->SetAnimationState("walk", false, 1.f, 0.f, L"");
		}
	}

	virtual void SetAnimationSyncRepeat(fixed repeattime)
	{
		m_AnimSyncRepeatTime = repeattime;

		if (m_Unit)
		{
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->SetAnimationSyncRepeat(m_AnimSyncRepeatTime.ToFloat());
		}
	}

	virtual void SetAnimationSyncOffset(fixed actiontime)
	{
		if (m_Unit)
		{
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->SetAnimationSyncOffset(actiontime.ToFloat());
		}
	}

	virtual void SetShadingColour(fixed r, fixed g, fixed b, fixed a)
	{
		m_R = r;
		m_G = g;
		m_B = b;
		UNUSED2(a); // TODO: why is this even an argument?

		if (m_Unit)
		{
			CModelAbstract& model = m_Unit->GetModel();
			model.SetShadingColor(CColor(m_R.ToFloat(), m_G.ToFloat(), m_B.ToFloat(), 1.0f));
		}
	}

	virtual void SetVariable(std::string name, float value)
	{
		if (m_Unit)
		{
			m_Unit->GetModel().SetEntityVariable(name, value);
		}
	}

	virtual u32 GetActorSeed()
	{
		return m_Seed;
	}
	
	virtual void SetActorSeed(u32 seed)
	{
		if (seed == m_Seed)
			return;

		m_Seed = seed;
		ReloadActor();
	}

	virtual bool HasConstructionPreview()
	{
		return m_ConstructionPreview;
	}

	virtual void Hotload(const VfsPath& name)
	{
		if (!m_Unit)
			return;

		if (name != m_ActorName)
			return;

		ReloadActor();
	}

private:
	/// Helper function shared by component init and actor reloading
	void InitModel(const CParamNode& paramNode);

	/// Helper method; initializes the model selection shape descriptor from XML. Factored out for readability of @ref Init.
	void InitSelectionShapeDescriptor(const CParamNode& paramNode);

	void ReloadActor();

	void Update(fixed turnLength);
	void UpdateVisibility();
};

REGISTER_COMPONENT_TYPE(VisualActor)

// ------------------------------------------------------------------------------------------------------------------

void CCmpVisualActor::InitModel(const CParamNode& paramNode)
{
	if (!GetSimContext().HasUnitManager())
		return;

	std::set<CStr> selections;
	std::wstring actorName = m_ActorName;
	if (actorName.find(L".xml") == std::wstring::npos)
		actorName += L".xml";
	m_Unit = GetSimContext().GetUnitManager().CreateUnit(actorName, GetActorSeed(), selections);
	if (!m_Unit)
		return;

	CModelAbstract& model = m_Unit->GetModel();
	if (model.ToCModel())
	{
		u32 modelFlags = 0;

		if (paramNode.GetChild("SilhouetteDisplay").ToBool())
			modelFlags |= MODELFLAG_SILHOUETTE_DISPLAY;

		if (paramNode.GetChild("SilhouetteOccluder").ToBool())
			modelFlags |= MODELFLAG_SILHOUETTE_OCCLUDER;

		CmpPtr<ICmpVision> cmpVision(GetEntityHandle());
		if (cmpVision && cmpVision->GetAlwaysVisible())
			modelFlags |= MODELFLAG_IGNORE_LOS;

		model.ToCModel()->AddFlagsRec(modelFlags);
	}

	if (paramNode.GetChild("DisableShadows").IsOk())
	{
		if (model.ToCModel())
			model.ToCModel()->RemoveShadowsRec();
		else if (model.ToCModelDecal())
			model.ToCModelDecal()->RemoveShadows();
	}

	// Initialize the model's selection shape descriptor. This currently relies on the component initialization order; the
	// Footprint component must be initialized before this component (VisualActor) to support the ability to use the footprint
	// shape for the selection box (instead of the default recursive bounding box). See TypeList.h for the order in
	// which components are initialized; if for whatever reason you need to get rid of this dependency, you can always just
	// initialize the selection shape descriptor on-demand.
	InitSelectionShapeDescriptor(paramNode);

	m_Unit->SetID(GetEntityId());

	bool floating = m_Unit->GetObject().m_Base->m_Properties.m_FloatOnWater;
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (cmpPosition)
		cmpPosition->SetActorFloating(floating);

	if (!m_ModelTag.valid())
	{
		CmpPtr<ICmpUnitRenderer> cmpModelRenderer(GetSystemEntity());
		if (cmpModelRenderer)
		{
			// TODO: this should account for all possible props, animations, etc,
			// else we might accidentally cull the unit when it should be visible
			CBoundingBoxAligned bounds = m_Unit->GetModel().GetWorldBoundsRec();
			CBoundingSphere boundSphere = CBoundingSphere::FromSweptBox(bounds);

			int flags = 0;
			if (m_IsActorOnly)
				flags |= ICmpUnitRenderer::ACTOR_ONLY;
			if (m_VisibleInAtlasOnly)
				flags |= ICmpUnitRenderer::VISIBLE_IN_ATLAS_ONLY;

			m_ModelTag = cmpModelRenderer->AddUnit(GetEntityHandle(), m_Unit, boundSphere, flags);
		}
	}
}

void CCmpVisualActor::InitSelectionShapeDescriptor(const CParamNode& paramNode)
{
	// by default, we don't need a custom selection shape and we can just keep the default behaviour
	CModelAbstract::CustomSelectionShape* shapeDescriptor = NULL;

	const CParamNode& shapeNode = paramNode.GetChild("SelectionShape");
	if (shapeNode.IsOk())
	{
		if (shapeNode.GetChild("Bounds").IsOk())
		{
			// default; no need to take action
		}
		else if (shapeNode.GetChild("Footprint").IsOk())
		{
			CmpPtr<ICmpFootprint> cmpFootprint(GetEntityHandle());
			if (cmpFootprint)
			{
				ICmpFootprint::EShape fpShape;				// fp stands for "footprint"
				entity_pos_t fpSize0, fpSize1, fpHeight;	// fp stands for "footprint"
				cmpFootprint->GetShape(fpShape, fpSize0, fpSize1, fpHeight);

				float size0 = fpSize0.ToFloat();
				float size1 = fpSize1.ToFloat();

				// TODO: we should properly distinguish between CIRCLE and SQUARE footprint shapes here, but since cylinders 
				// aren't implemented yet and are almost indistinguishable from boxes for small enough sizes anyway, 
				// we'll just use boxes for either case. However, for circular footprints the size0 and size1 values both 
				// represent the radius, so we do have to adjust them to match the size1 and size0's of square footprints 
				// (which represent the full width and depth).
				if (fpShape == ICmpFootprint::CIRCLE)
				{
					size0 *= 2;
					size1 *= 2;
				}

				shapeDescriptor = new CModelAbstract::CustomSelectionShape;
				shapeDescriptor->m_Type = CModelAbstract::CustomSelectionShape::BOX;
				shapeDescriptor->m_Size0 = size0;
				shapeDescriptor->m_Size1 = size1;
				shapeDescriptor->m_Height = fpHeight.ToFloat();
			}
			else
			{
				LOGERROR(L"[VisualActor] Cannot apply footprint-based SelectionShape; Footprint component not initialized.");
			}
		}
		else if (shapeNode.GetChild("Box").IsOk())
		{
			// TODO: we might need to support the ability to specify a different box center in the future
			shapeDescriptor = new CModelAbstract::CustomSelectionShape;
			shapeDescriptor->m_Type = CModelAbstract::CustomSelectionShape::BOX;
			shapeDescriptor->m_Size0 = shapeNode.GetChild("Box").GetChild("@width").ToFixed().ToFloat();
			shapeDescriptor->m_Size1 = shapeNode.GetChild("Box").GetChild("@depth").ToFixed().ToFloat();
			shapeDescriptor->m_Height = shapeNode.GetChild("Box").GetChild("@height").ToFixed().ToFloat();
		}
		else if (shapeNode.GetChild("Cylinder").IsOk())
		{
			LOGWARNING(L"[VisualActor] TODO: Cylinder selection shapes are not yet implemented; defaulting to recursive bounding boxes");
		}
		else
		{
			// shouldn't happen by virtue of validation against schema
			LOGERROR(L"[VisualActor] No selection shape specified");
		}
	}

	ENSURE(m_Unit);
	// the model is now responsible for cleaning up the descriptor
	m_Unit->GetModel().SetCustomSelectionShape(shapeDescriptor);
}

void CCmpVisualActor::ReloadActor()
{
	if (!m_Unit)
		return;

	// Save some data from the old unit
	CColor shading = m_Unit->GetModel().GetShadingColor();
	player_id_t playerID = m_Unit->GetModel().GetPlayerID();

	// Replace with the new unit
	GetSimContext().GetUnitManager().DeleteUnit(m_Unit);

	// HACK: selection shape needs template data, but rather than storing all that data
	//	in the component, we load the template here and pass it into a helper function
	CmpPtr<ICmpTemplateManager> cmpTemplateManager(GetSystemEntity());
	const CParamNode* node = cmpTemplateManager->LoadLatestTemplate(GetEntityId());
	ENSURE(node && node->GetChild("VisualActor").IsOk());

	InitModel(node->GetChild("VisualActor"));

	m_Unit->SetEntitySelection(m_AnimName);
	if (m_Unit->GetAnimation())
		m_Unit->GetAnimation()->SetAnimationState(m_AnimName, m_AnimOnce, m_AnimSpeed.ToFloat(), m_AnimDesync.ToFloat(), m_SoundGroup.c_str());

	// We'll lose the exact synchronisation but we should at least make sure it's going at the correct rate
	if (!m_AnimSyncRepeatTime.IsZero())
		if (m_Unit->GetAnimation())
			m_Unit->GetAnimation()->SetAnimationSyncRepeat(m_AnimSyncRepeatTime.ToFloat());

	m_Unit->GetModel().SetShadingColor(shading);

	m_Unit->GetModel().SetPlayerID(playerID);

	if (m_ModelTag.valid())
	{
		CmpPtr<ICmpUnitRenderer> cmpModelRenderer(GetSystemEntity());
		CBoundingBoxAligned bounds = m_Unit->GetModel().GetWorldBoundsRec();
		CBoundingSphere boundSphere = CBoundingSphere::FromSweptBox(bounds);
		cmpModelRenderer->UpdateUnit(m_ModelTag, m_Unit, boundSphere);
	}
}

void CCmpVisualActor::Update(fixed UNUSED(turnLength))
{
	if (m_Unit == NULL)
		return;

	// If we're in the special movement mode, select an appropriate animation
	if (!m_AnimRunThreshold.IsZero())
	{
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return;

		CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetEntityHandle());
		if (!cmpUnitMotion)
			return;

		float speed = cmpUnitMotion->GetCurrentSpeed().ToFloat();

		std::string name;
		if (speed == 0.0f)
			name = "idle";
		else
			name = (speed < m_AnimRunThreshold.ToFloat()) ? "walk" : "run";

		std::map<std::string, std::string>::const_iterator it = m_AnimOverride.find(name);
		if (it != m_AnimOverride.end())
			name = it->second;

		m_Unit->SetEntitySelection(name);
		if (speed == 0.0f)
		{
			m_Unit->SetEntitySelection(name);
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->SetAnimationState(name, false, 1.f, 0.f, L"");
		}
		else
		{
			m_Unit->SetEntitySelection(name);
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->SetAnimationState(name, false, speed, 0.f, L"");
		}
	}
}
