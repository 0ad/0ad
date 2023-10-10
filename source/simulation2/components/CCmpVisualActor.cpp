/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpVisual.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/serialization/SerializedTypes.h"

#include "ICmpFootprint.h"
#include "ICmpIdentity.h"
#include "ICmpMirage.h"
#include "ICmpOwnership.h"
#include "ICmpPosition.h"
#include "ICmpTemplateManager.h"
#include "ICmpTerrain.h"
#include "ICmpUnitMotion.h"
#include "ICmpUnitRenderer.h"
#include "ICmpValueModificationManager.h"
#include "ICmpVisibility.h"
#include "ICmpSound.h"

#include "graphics/Decal.h"
#include "graphics/Model.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "graphics/Unit.h"
#include "graphics/UnitAnimation.h"
#include "graphics/UnitManager.h"
#include "maths/BoundingSphere.h"
#include "maths/Frustum.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "ps/GameSetup/Config.h"
#include "renderer/Scene.h"

class CCmpVisualActor final : public ICmpVisual
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_InterpolatedPositionChanged);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_ValueModification);
		componentManager.SubscribeToMessageType(MT_Create);
		componentManager.SubscribeToMessageType(MT_Destroy);
	}

	DEFAULT_COMPONENT_ALLOCATOR(VisualActor)

private:
	std::wstring m_BaseActorName, m_ActorName;
	bool m_IsFoundationActor;

	// Not initialized in non-visual mode
	CUnit* m_Unit;
	CModelAbstract::CustomSelectionShape* m_ShapeDescriptor = nullptr;

	fixed m_R, m_G, m_B; // shading color

	// Current animation state
	std::string m_AnimName;
	bool m_AnimOnce;
	fixed m_AnimSpeed;
	std::wstring m_SoundGroup;
	fixed m_AnimDesync;
	fixed m_AnimSyncRepeatTime; // 0.0 if not synced
	fixed m_AnimSyncOffsetTime;

	std::map<CStr, CStr> m_VariantSelections;

	u32 m_Seed; // seed used for random variations

	bool m_ConstructionPreview;

	bool m_VisibleInAtlasOnly;
	bool m_IsActorOnly;	// an in-world entity should not have this or it might not be rendered.

	bool m_SilhouetteDisplay;
	bool m_SilhouetteOccluder;
	bool m_DisableShadows;

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
								"<data type='decimal'>"
									"<param name='minExclusive'>0.0</param>"
								"</data>"
							"</attribute>"
							"<attribute name='height'>"
								"<data type='decimal'>"
									"<param name='minExclusive'>0.0</param>"
								"</data>"
							"</attribute>"
							"<attribute name='depth'>"
								"<data type='decimal'>"
									"<param name='minExclusive'>0.0</param>"
								"</data>"
							"</attribute>"
						"</element>"
						"<element name='Cylinder' a:help='Sets the selection shape to a cylinder of specified dimensions'>"
							"<attribute name='radius'>"
								"<data type='decimal'>"
									"<param name='minExclusive'>0.0</param>"
								"</data>"
							"</attribute>"
							"<attribute name='height'>"
								"<data type='decimal'>"
									"<param name='minExclusive'>0.0</param>"
								"</data>"
							"</attribute>"
						"</element>"
					"</choice>"
				"</element>"
			"</optional>"
			"<element name='VisibleInAtlasOnly'>"
				"<data type='boolean'/>"
			"</element>";
	}

	void Init(const CParamNode& paramNode) override
	{
		m_Unit = NULL;
		m_R = m_G = m_B = fixed::FromInt(1);

		m_ConstructionPreview = paramNode.GetChild("ConstructionPreview").IsOk();

		m_Seed = GetEntityId();

		m_IsFoundationActor = paramNode.GetChild("Foundation").IsOk() && paramNode.GetChild("FoundationActor").IsOk();

		m_BaseActorName = paramNode.GetChild(m_IsFoundationActor ? "FoundationActor" : "Actor").ToWString();
		ParseActorName(m_BaseActorName);

		m_VisibleInAtlasOnly = paramNode.GetChild("VisibleInAtlasOnly").ToBool();
		m_IsActorOnly = paramNode.GetChild("ActorOnly").IsOk();

		m_SilhouetteDisplay = paramNode.GetChild("SilhouetteDisplay").ToBool();
		m_SilhouetteOccluder = paramNode.GetChild("SilhouetteOccluder").ToBool();
		m_DisableShadows = paramNode.GetChild("DisableShadows").ToBool();

		// Initialize the model's selection shape descriptor. This currently relies on the component initialization order; the
		// Footprint component must be initialized before this component (VisualActor) to support the ability to use the footprint
		// shape for the selection box (instead of the default recursive bounding box). See TypeList.h for the order in
		// which components are initialized; if for whatever reason you need to get rid of this dependency, you can always just
		// initialize the selection shape descriptor on-demand.
		InitSelectionShapeDescriptor(paramNode);
	}

	void Deinit() override
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

		serialize.StringASCII("anim name", m_AnimName, 0, 256);
		serialize.Bool("anim once", m_AnimOnce);
		serialize.NumberFixed_Unbounded("anim speed", m_AnimSpeed);
		serialize.String("sound group", m_SoundGroup, 0, 256);
		serialize.NumberFixed_Unbounded("anim desync", m_AnimDesync);
		serialize.NumberFixed_Unbounded("anim sync repeat time", m_AnimSyncRepeatTime);
		serialize.NumberFixed_Unbounded("anim sync offset time", m_AnimSyncOffsetTime);

		Serializer(serialize, "variation", m_VariantSelections);

		serialize.NumberU32_Unbounded("seed", m_Seed);
		serialize.String("actor", m_ActorName, 0, 256);

		// TODO: store actor variables?
	}

	void Serialize(ISerializer& serialize) override
	{
		// TODO: store the actor name, if !debug and it differs from the template

		if (serialize.IsDebug())
		{
			serialize.String("base actor", m_BaseActorName, 0, 256);
		}

		SerializeCommon(serialize);
	}

	void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize) override
	{
		Init(paramNode);

		u32 oldSeed = GetActorSeed();

		SerializeCommon(deserialize);

		InitModel();

		// If we serialized a different seed or different actor, reload actor
		if (oldSeed != GetActorSeed() || m_BaseActorName != m_ActorName)
			ReloadActor();
		else
			ReloadUnitAnimation();

		if (m_Unit)
		{
			CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
			if (cmpOwnership)
				m_Unit->GetModel().SetPlayerID(cmpOwnership->GetOwner());
		}
	}

	void HandleMessage(const CMessage& msg, bool UNUSED(global)) override
	{
		switch (msg.GetType())
		{
		case MT_OwnershipChanged:
		{
			RecomputeActorName();

			if (!m_Unit)
				break;

			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			m_Unit->GetModel().SetPlayerID(msgData.to);

			break;
		}
		case MT_ValueModification:
		{
			// Mirages don't respond to technology modifications.
			CmpPtr<ICmpMirage> cmpMirage(GetEntityHandle());
			if (cmpMirage)
				return;

			const CMessageValueModification& msgData = static_cast<const CMessageValueModification&> (msg);
			if (msgData.component != L"VisualActor")
				break;

			RecomputeActorName();

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
		case MT_Create:
		{
			InitModel();

			SelectAnimation("idle");
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

	CBoundingBoxAligned GetBounds() const override
	{
		if (!m_Unit)
			return CBoundingBoxAligned::EMPTY;
		return m_Unit->GetModel().GetWorldBounds();
	}

	CUnit* GetUnit() override
	{
		return m_Unit;
	}

	CBoundingBoxOriented GetSelectionBox() const override
	{
		if (!m_Unit)
			return CBoundingBoxOriented::EMPTY;
		return m_Unit->GetModel().GetSelectionBox();
	}

	CVector3D GetPosition() const override
	{
		if (!m_Unit)
			return CVector3D(0, 0, 0);
		return m_Unit->GetModel().GetTransform().GetTranslation();
	}

	std::wstring GetProjectileActor() const override
	{
		if (!m_Unit)
			return L"";
		return m_Unit->GetObject().m_ProjectileModelName;
	}

	CFixedVector3D GetProjectileLaunchPoint() const override
	{
		if (!m_Unit)
			return CFixedVector3D();

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
			{
				CVector3D vector = ammo->GetTransform().GetTranslation();
				return CFixedVector3D(fixed::FromFloat(vector.X), fixed::FromFloat(vector.Y), fixed::FromFloat(vector.Z));
			}
		}

		return CFixedVector3D();
	}

	void SetVariant(const CStr& key, const CStr& selection) override
	{
		if (m_VariantSelections[key] == selection)
			return;

		m_VariantSelections[key] = selection;

		if (m_Unit)
		{
			m_Unit->SetEntitySelection(key, selection);
			if (m_Unit->GetAnimation())
				m_Unit->GetAnimation()->ReloadAnimation();
		}
	}

	std::string GetAnimationName() const override
	{
		return m_AnimName;
	}

	void SelectAnimation(const std::string& name, bool once = false, fixed speed = fixed::FromInt(1)) override
	{
		m_AnimName = name;
		m_AnimOnce = once;
		m_AnimSpeed = speed;
		m_SoundGroup = L"";
		m_AnimDesync = fixed::FromInt(1)/20; // TODO: make this an argument
		m_AnimSyncRepeatTime = fixed::Zero();
		m_AnimSyncOffsetTime = fixed::Zero();

		SetVariant("animation", m_AnimName);

		CmpPtr<ICmpSound> cmpSound(GetEntityHandle());
		if (cmpSound)
			m_SoundGroup = cmpSound->GetSoundGroup(wstring_from_utf8(m_AnimName));

		if (!m_Unit || !m_Unit->GetAnimation() || !m_Unit->GetID())
			return;

		m_Unit->GetAnimation()->SetAnimationState(m_AnimName, m_AnimOnce, m_AnimSpeed.ToFloat(), m_AnimDesync.ToFloat(), m_SoundGroup.c_str());	
	}

	void SelectMovementAnimation(const std::string& name, fixed speed) override
	{
		ENSURE(name == "idle" || name == "walk" || name == "run");
		if (m_AnimName != "idle" && m_AnimName != "walk" && m_AnimName != "run")
			return;
		if (m_AnimName == name && speed == m_AnimSpeed)
			return;
		SelectAnimation(name, false, speed);
	}

	void SetAnimationSyncRepeat(fixed repeattime) override
	{
		m_AnimSyncRepeatTime = repeattime;

		if (m_Unit && m_Unit->GetAnimation())
			m_Unit->GetAnimation()->SetAnimationSyncRepeat(m_AnimSyncRepeatTime.ToFloat());
	}

	void SetAnimationSyncOffset(fixed actiontime) override
	{
		m_AnimSyncOffsetTime = actiontime;

		if (m_Unit && m_Unit->GetAnimation())
			m_Unit->GetAnimation()->SetAnimationSyncOffset(m_AnimSyncOffsetTime.ToFloat());
	}

	void SetShadingColor(fixed r, fixed g, fixed b, fixed a) override
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

	void SetVariable(const std::string& name, float value) override
	{
		if (m_Unit)
			m_Unit->GetModel().SetEntityVariable(name, value);
	}

	u32 GetActorSeed() const override
	{
		return m_Seed;
	}

	void SetActorSeed(u32 seed) override
	{
		if (seed == m_Seed)
			return;

		m_Seed = seed;
		ReloadActor();
	}

	void RecomputeActorName() override
	{
		CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSystemEntity());
		std::wstring newActorName;
		if (m_IsFoundationActor)
			newActorName = cmpValueModificationManager->ApplyModifications(L"VisualActor/FoundationActor", m_BaseActorName, GetEntityId());
		else
			newActorName = cmpValueModificationManager->ApplyModifications(L"VisualActor/Actor", m_BaseActorName, GetEntityId());

		if (newActorName != m_ActorName)
		{
			ParseActorName(newActorName);
			ReloadActor();
		}
	}

	bool HasConstructionPreview() const override
	{
		return m_ConstructionPreview;
	}

	void Hotload(const VfsPath& name) override
	{
		if (!m_Unit)
			return;

		if (!name.empty() && name != m_ActorName)
			return;

		ReloadActor();
	}

private:
	// Replace {phenotype} with the correct value in m_ActorName
	void ParseActorName(std::wstring base);

	/// Helper function shared by component init and actor reloading
	void InitModel();

	/// Helper method; initializes the model selection shape descriptor from XML. Factored out for readability of @ref Init.
	void InitSelectionShapeDescriptor(const CParamNode& paramNode);

	// ReloadActor is used when the actor or seed changes.
	void ReloadActor();
	// ReloadUnitAnimation is used for a minimal reloading upon deserialization, when the actor and seed are identical.
	// It is also used by ReloadActor.
	void ReloadUnitAnimation();
};

REGISTER_COMPONENT_TYPE(VisualActor)

// ------------------------------------------------------------------------------------------------------------------

void CCmpVisualActor::ParseActorName(std::wstring base)
{
	CmpPtr<ICmpIdentity> cmpIdentity(GetEntityHandle());
	const std::wstring pattern = L"{phenotype}";
	if (cmpIdentity)
	{
		size_t pos = base.find(pattern);
		while (pos != std::string::npos)
		{
			base.replace(pos, pattern.size(),  cmpIdentity->GetPhenotype());
			pos = base.find(pattern, pos + pattern.size());
		}
	}

	m_ActorName = base;
}

void CCmpVisualActor::InitModel()
{
	if (!GetSimContext().HasUnitManager())
		return;

	std::wstring actorName = m_ActorName;
	if (actorName.find(L".xml") == std::wstring::npos)
		actorName += L".xml";
	m_Unit = GetSimContext().GetUnitManager().CreateUnit(actorName, GetEntityId(), GetActorSeed());
	if (!m_Unit)
		return;

	CModelAbstract& model = m_Unit->GetModel();
	if (model.ToCModel())
	{
		u32 modelFlags = 0;

		if (m_SilhouetteDisplay)
			modelFlags |= ModelFlag::SILHOUETTE_DISPLAY;

		if (m_SilhouetteOccluder)
			modelFlags |= ModelFlag::SILHOUETTE_OCCLUDER;

		CmpPtr<ICmpVisibility> cmpVisibility(GetEntityHandle());
		if (cmpVisibility && cmpVisibility->GetAlwaysVisible())
			modelFlags |= ModelFlag::IGNORE_LOS;

		model.ToCModel()->AddFlagsRec(modelFlags);
	}

	if (m_DisableShadows)
	{
		if (model.ToCModel())
			model.ToCModel()->RemoveShadowsRec();
		else if (model.ToCModelDecal())
			model.ToCModelDecal()->RemoveShadows();
	}

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

	// the model is now responsible for cleaning up the descriptor
	if (m_ShapeDescriptor != nullptr)
		m_Unit->GetModel().SetCustomSelectionShape(m_ShapeDescriptor);
}

void CCmpVisualActor::InitSelectionShapeDescriptor(const CParamNode& paramNode)
{
	// by default, we don't need a custom selection shape and we can just keep the default behaviour
	m_ShapeDescriptor = nullptr;

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

				m_ShapeDescriptor = new CModelAbstract::CustomSelectionShape;
				m_ShapeDescriptor->m_Type = CModelAbstract::CustomSelectionShape::BOX;
				m_ShapeDescriptor->m_Size0 = size0;
				m_ShapeDescriptor->m_Size1 = size1;
				m_ShapeDescriptor->m_Height = fpHeight.ToFloat();
			}
			else
			{
				LOGERROR("[VisualActor] Cannot apply footprint-based SelectionShape; Footprint component not initialized.");
			}
		}
		else if (shapeNode.GetChild("Box").IsOk())
		{
			// TODO: we might need to support the ability to specify a different box center in the future
			m_ShapeDescriptor = new CModelAbstract::CustomSelectionShape;
			m_ShapeDescriptor->m_Type = CModelAbstract::CustomSelectionShape::BOX;
			m_ShapeDescriptor->m_Size0 = shapeNode.GetChild("Box").GetChild("@width").ToFixed().ToFloat();
			m_ShapeDescriptor->m_Size1 = shapeNode.GetChild("Box").GetChild("@depth").ToFixed().ToFloat();
			m_ShapeDescriptor->m_Height = shapeNode.GetChild("Box").GetChild("@height").ToFixed().ToFloat();
		}
		else if (shapeNode.GetChild("Cylinder").IsOk())
		{
			LOGWARNING("[VisualActor] TODO: Cylinder selection shapes are not yet implemented; defaulting to recursive bounding boxes");
		}
		else
		{
			// shouldn't happen by virtue of validation against schema
			LOGERROR("[VisualActor] No selection shape specified");
		}
	}
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

	InitSelectionShapeDescriptor(node->GetChild("VisualActor"));

	InitModel();

	if (!m_Unit)
	{
		if (m_ModelTag.valid())
		{
			CmpPtr<ICmpUnitRenderer> cmpModelRenderer(GetSystemEntity());
			if (cmpModelRenderer)
				cmpModelRenderer->RemoveUnit(m_ModelTag);
			m_ModelTag = ICmpUnitRenderer::tag_t{};
		}
		return;
	}

	ReloadUnitAnimation();

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

void CCmpVisualActor::ReloadUnitAnimation()
{
	if (!m_Unit)
		return;

	m_Unit->SetEntitySelection(m_VariantSelections);

	if (!m_Unit->GetAnimation())
		return;

	m_Unit->GetAnimation()->SetAnimationState(m_AnimName, m_AnimOnce, m_AnimSpeed.ToFloat(), m_AnimDesync.ToFloat(), m_SoundGroup.c_str());

	// We'll lose the exact synchronisation but we should at least make sure it's going at the correct rate
	if (!m_AnimSyncRepeatTime.IsZero())
		m_Unit->GetAnimation()->SetAnimationSyncRepeat(m_AnimSyncRepeatTime.ToFloat());
	if (!m_AnimSyncOffsetTime.IsZero())
		m_Unit->GetAnimation()->SetAnimationSyncOffset(m_AnimSyncOffsetTime.ToFloat());
}
