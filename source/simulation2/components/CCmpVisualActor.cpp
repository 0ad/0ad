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

#include "precompiled.h"

#include "simulation2/system/Component.h"
#include "ICmpVisual.h"

#include "ICmpPosition.h"
#include "ICmpRangeManager.h"
#include "ICmpVision.h"
#include "simulation2/MessageTypes.h"

#include "graphics/Frustum.h"
#include "graphics/Model.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "graphics/Unit.h"
#include "graphics/UnitAnimation.h"
#include "graphics/UnitManager.h"
#include "lib/utf8.h"
#include "maths/Matrix3D.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "renderer/Scene.h"

class CCmpVisualActor : public ICmpVisual
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update_Final);
		componentManager.SubscribeToMessageType(MT_Interpolate);
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
	}

	DEFAULT_COMPONENT_ALLOCATOR(VisualActor)

	std::wstring m_ActorName;
	CUnit* m_Unit;

	ICmpRangeManager::ELosVisibility m_Visibility; // only valid between Interpolate and RenderSubmit

	// Current animation state
	float m_AnimRunThreshold; // if non-zero this is the special walk/run mode
	std::string m_AnimName;
	bool m_AnimOnce;
	float m_AnimSpeed;
	std::wstring m_SoundGroup;
	float m_AnimDesync;

	float m_AnimSyncRepeatTime; // 0.0 if not synced

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
				"<element name='Foundation' a:help='Used internally; if present the unit will be rendered as a foundation'>"
					"<empty/>"
				"</element>"
			"</optional>";
	}

	virtual void Init(const CSimContext& context, const CParamNode& paramNode)
	{
		m_Unit = NULL;

		if (!context.HasUnitManager())
			return; // do nothing if graphics are disabled

		// TODO: we should do some fancy animation of under-construction buildings rising from the ground,
		// but for now we'll just use the foundation actor and ignore the normal one
		if (paramNode.GetChild("Foundation").IsOk() && paramNode.GetChild("FoundationActor").IsOk())
			m_ActorName = paramNode.GetChild("FoundationActor").ToString();
		else
			m_ActorName = paramNode.GetChild("Actor").ToString();

		std::set<CStr> selections;
		m_Unit = context.GetUnitManager().CreateUnit(m_ActorName, selections);
		if (!m_Unit)
		{
			// The error will have already been logged
			return;
		}

		m_Unit->SetID(GetEntityId());

		SelectAnimation("idle", false, 0.f, L"");
	}

	virtual void Deinit(const CSimContext& context)
	{
		if (m_Unit)
		{
			context.GetUnitManager().DeleteUnit(m_Unit);
			m_Unit = NULL;
		}
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO: store the actor name, if !debug and it differs from the template

		if (serialize.IsDebug())
		{
			serialize.String("actor", m_ActorName, 0, 256);
		}

		// TODO: store random variation. This ought to be synchronised across saved games
		// and networks, so everyone sees the same thing. Saving the list of selection strings
		// would be awfully inefficient, so actors should be changed to (by default) represent
		// variations with a 16-bit RNG seed (selected randomly when creating new units, or
		// when someone hits the "randomise" button in the map editor), only overridden with
		// a list of strings if it really needs to be a specific variation.

		// TODO: store animation state

		// TODO: store shading colour
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(context, paramNode);
	}

	virtual void HandleMessage(const CSimContext& UNUSED(context), const CMessage& msg, bool UNUSED(global))
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
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);
			Interpolate(msgData.frameTime, msgData.offset);
			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector, msgData.frustum, msgData.culling);
			break;
		}
		case MT_OwnershipChanged:
		{
			const CMessageOwnershipChanged& msgData = static_cast<const CMessageOwnershipChanged&> (msg);
			m_Unit->GetModel().SetPlayerID(msgData.to);
			break;
		}
		}
	}

	virtual CBound GetBounds()
	{
		if (!m_Unit)
			return CBound();
		return m_Unit->GetModel().GetBounds();
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

		// Ensure the prop transforms are correct
		m_Unit->GetModel().ValidatePosition();

		CModel* ammo = m_Unit->GetModel().FindFirstAmmoProp();
		if (!ammo)
			return CVector3D();
		return ammo->GetTransform().GetTranslation();
	}

	virtual void SelectAnimation(std::string name, bool once, float speed, std::wstring soundgroup)
	{
		if (!m_Unit)
			return;

		if (!isfinite(speed) || speed < 0) // JS 'undefined' converts to NaN, which causes Bad Things
			speed = 1.f;

		m_AnimRunThreshold = 0.f;
		m_AnimName = name;
		m_AnimOnce = once;
		m_AnimSpeed = speed;
		m_SoundGroup = soundgroup;
		m_AnimDesync = 0.05f; // TODO: make this an argument
		m_AnimSyncRepeatTime = 0.0f;

		m_Unit->GetAnimation().SetAnimationState(m_AnimName, m_AnimOnce, m_AnimSpeed, m_AnimDesync, false, m_SoundGroup.c_str());
	}

	virtual void SelectMovementAnimation(float runThreshold)
	{
		if (!m_Unit)
			return;

		m_AnimRunThreshold = runThreshold;

		m_Unit->GetAnimation().SetAnimationState("walk", false, 1.f, 0.f, false, L"");
	}

	virtual void SetAnimationSyncRepeat(float repeattime)
	{
		if (!m_Unit)
			return;

		m_AnimSyncRepeatTime = repeattime;

		m_Unit->GetAnimation().SetAnimationSyncRepeat(m_AnimSyncRepeatTime);
	}

	virtual void SetAnimationSyncOffset(float actiontime)
	{
		if (!m_Unit)
			return;

		m_Unit->GetAnimation().SetAnimationSyncOffset(actiontime);
	}

	virtual void SetShadingColour(fixed r, fixed g, fixed b, fixed a)
	{
		if (!m_Unit)
			return;

		m_Unit->GetModel().SetShadingColor(CColor(r.ToFloat(), g.ToFloat(), b.ToFloat(), a.ToFloat()));
	}

	virtual void Hotload(const std::wstring& name)
	{
		if (!m_Unit)
			return;

		if (name != m_ActorName)
			return;

		std::set<CStr> selections;
		CUnit* newUnit = GetSimContext().GetUnitManager().CreateUnit(m_ActorName, selections);

		if (!newUnit)
			return;

		// Save some data from the old unit
		CColor shading = m_Unit->GetModel().GetShadingColor();
		size_t playerID = m_Unit->GetModel().GetPlayerID();

		// Replace with the new unit
		GetSimContext().GetUnitManager().DeleteUnit(m_Unit);
		m_Unit = newUnit;

		m_Unit->SetID(GetEntityId());

		m_Unit->GetAnimation().SetAnimationState(m_AnimName, m_AnimOnce, m_AnimSpeed, m_AnimDesync, false, m_SoundGroup.c_str());

		// We'll lose the exact synchronisation but we should at least make sure it's going at the correct rate
		if (m_AnimSyncRepeatTime != 0.0f)
			m_Unit->GetAnimation().SetAnimationSyncRepeat(m_AnimSyncRepeatTime);

		m_Unit->GetModel().SetShadingColor(shading);

		m_Unit->GetModel().SetPlayerID(playerID);
	}

private:
	void Update(fixed turnLength);
	void Interpolate(float frameTime, float frameOffset);
	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling);
};

REGISTER_COMPONENT_TYPE(VisualActor)

void CCmpVisualActor::Update(fixed turnLength)
{
	if (m_Unit == NULL)
		return;

	// If we're in the special movement mode, select an appropriate animation
	if (m_AnimRunThreshold)
	{
		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null() || !cmpPosition->IsInWorld())
			return;

		float speed = cmpPosition->GetDistanceTravelled().ToFloat() / turnLength.ToFloat();

		if (speed == 0.0f)
			m_Unit->GetAnimation().SetAnimationState("idle", false, 1.f, 0.f, false, L"");
		else if (speed < m_AnimRunThreshold)
			m_Unit->GetAnimation().SetAnimationState("walk", false, speed, 0.f, false, L"");
		else
			m_Unit->GetAnimation().SetAnimationState("run", false, speed, 0.f, false, L"");
	}
}

void CCmpVisualActor::Interpolate(float frameTime, float frameOffset)
{
	if (m_Unit == NULL)
		return;

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null())
		return;

	// Disable rendering of the unit if it has no position
	if (!cmpPosition->IsInWorld())
	{
		m_Visibility = ICmpRangeManager::VIS_HIDDEN;
		return;
	}

	CmpPtr<ICmpRangeManager> cmpRangeManager(GetSimContext(), SYSTEM_ENTITY);
	m_Visibility = cmpRangeManager->GetLosVisibility(GetEntityId(), GetSimContext().GetCurrentDisplayedPlayer());

	// Even if HIDDEN due to LOS, we need to set up the transforms
	// so that projectiles will be launched from the right place

	bool floating = m_Unit->GetObject().m_Base->m_Properties.m_FloatOnWater;

	CMatrix3D transform(cmpPosition->GetInterpolatedTransform(frameOffset, floating));

	m_Unit->GetModel().SetTransform(transform);
	m_Unit->UpdateModel(frameTime);
}

void CCmpVisualActor::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	if (m_Unit == NULL)
		return;

	if (m_Visibility == ICmpRangeManager::VIS_HIDDEN)
		return;

	CModel& model = m_Unit->GetModel();

	model.ValidatePosition();

	if (culling && !frustum.IsBoxVisible(CVector3D(0, 0, 0), model.GetBounds()))
		return;

	if (m_Visibility == ICmpRangeManager::VIS_FOGGED)
		model.SetShadingColor(CColor(0.7f, 0.7f, 0.7f, 1.0f));
	else
		model.SetShadingColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));

	collector.SubmitRecursive(&model);
}
