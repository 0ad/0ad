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
#include "ICmpUnitRenderer.h"

#include "simulation2/MessageTypes.h"

#include "ICmpPosition.h"
#include "ICmpRangeManager.h"
#include "ICmpSelectable.h"
#include "ICmpVision.h"

#include "graphics/Frustum.h"
#include "graphics/ModelAbstract.h"
#include "graphics/ObjectEntry.h"
#include "graphics/Overlay.h"
#include "graphics/Unit.h"
#include "maths/BoundingSphere.h"
#include "maths/Matrix3D.h"
#include "renderer/Scene.h"

#include "tools/atlas/GameInterface/GameLoop.h"

/**
 * Efficiently(ish) renders all the units in the world.
 *
 * The class maintains a list of all units that currently exist, and the data
 * needed for frustum-culling them. To minimise the amount of work done per
 * frame (despite a unit's interpolated position changing every frame), the
 * culling data is only updated once per turn: we store the position at the
 * start of the turn, and the position at the end of the turn, and assume the
 * unit might be anywhere between those two points (linearly).
 *
 * (Note this is a slightly invalid assumption: units don't always move linearly,
 * since their interpolated position depends on terrain and water. But over a
 * single turn it's probably going to be a good enough approximation, and will
 * only break for units that both start and end the turn off-screen.)
 *
 * We want to ignore rotation entirely, since it's a complex function of
 * interpolated position and terrain. So we store a bounding sphere, which
 * is rotation-independent, instead of a bounding box.
 */
class CCmpUnitRenderer : public ICmpUnitRenderer
{
public:
	struct SUnit
	{
		CEntityHandle entity;

		CUnit* actor;

		int flags;

		/**
		 * m_FrameNumber from when the model's transform was last updated.
		 * This is used to avoid recomputing it multiple times per frame
		 * if a model is visible in multiple cull groups.
		 */
		int lastTransformFrame;

		/**
		 * Worst-case bounding shape, relative to position. Needs to account
		 * for all possible animations, orientations, etc.
		 */
		CBoundingSphere boundsApprox;

		/**
		 * Cached LOS visibility status.
		 */
		ICmpRangeManager::ELosVisibility visibility;
		bool visibilityDirty;

		/**
		 * Whether the unit has a valid position. If false, pos0 and pos1
		 * are meaningless.
		 */
		bool inWorld;

		/**
		 * World-space positions to interpolate between.
		 */
		CVector3D pos0;
		CVector3D pos1;

		/**
		 * Bounds encompassing the unit's bounds when it is anywhere between
		 * pos0 and pos1.
		 */
		CBoundingSphere sweptBounds;

		/**
		 * For debug overlay.
		 */
		bool culled;
	};

	std::vector<SUnit> m_Units;
	std::vector<tag_t> m_UnitTagsFree;

	int m_FrameNumber;
	float m_FrameOffset;

	bool m_EnableDebugOverlays;
	std::vector<SOverlaySphere> m_DebugSpheres;

	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_TurnStart);
		componentManager.SubscribeToMessageType(MT_Interpolate);
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitRenderer)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_FrameNumber = 0;
		m_FrameOffset = 0.0f;
		m_EnableDebugOverlays = false;
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_TurnStart:
		{
			TurnStart();
			break;
		}
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);
			Interpolate(msgData.deltaSimTime, msgData.offset);
			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector, msgData.frustum, msgData.culling);
			break;
		}
		}
	}

	SUnit* LookupUnit(tag_t tag)
	{
		if (tag.n < 1 || tag.n - 1 >= m_Units.size())
			return NULL;
		return &m_Units[tag.n - 1];
	}

	virtual tag_t AddUnit(CEntityHandle entity, CUnit* actor, const CBoundingSphere& boundsApprox, int flags)
	{
		ENSURE(actor != NULL);

		tag_t tag;
		if (!m_UnitTagsFree.empty())
		{
			tag = m_UnitTagsFree.back();
			m_UnitTagsFree.pop_back();
		}
		else
		{
			m_Units.push_back(SUnit());
			tag.n = m_Units.size();
		}

		SUnit* unit = LookupUnit(tag);
		unit->entity = entity;
		unit->actor = actor;
		unit->lastTransformFrame = -1;
		unit->flags = flags;
		unit->boundsApprox = boundsApprox;
		unit->inWorld = false;
		unit->visibilityDirty = true;
		unit->pos0 = unit->pos1 = CVector3D();

		return tag;
	}

	virtual void RemoveUnit(tag_t tag)
	{
		SUnit* unit = LookupUnit(tag);
		unit->actor = NULL;
		unit->inWorld = false;
		m_UnitTagsFree.push_back(tag);
	}

	void RecomputeSweptBounds(SUnit* unit)
	{
		// Compute the bounding sphere of the capsule formed by
		// sweeping boundsApprox from pos0 to pos1
		CVector3D mid = (unit->pos0 + unit->pos1) * 0.5f + unit->boundsApprox.GetCenter();
		float radius = (unit->pos1 - unit->pos0).Length() * 0.5f + unit->boundsApprox.GetRadius();
		unit->sweptBounds = CBoundingSphere(mid, radius);
	}

	virtual void UpdateUnit(tag_t tag, CUnit* actor, const CBoundingSphere& boundsApprox)
	{
		SUnit* unit = LookupUnit(tag);
		unit->actor = actor;
		unit->boundsApprox = boundsApprox;
		RecomputeSweptBounds(unit);
	}

	virtual void UpdateUnitPos(tag_t tag, bool inWorld, const CVector3D& pos0, const CVector3D& pos1)
	{
		SUnit* unit = LookupUnit(tag);
		unit->inWorld = inWorld;
		unit->pos0 = pos0;
		unit->pos1 = pos1;
		unit->visibilityDirty = true;
		RecomputeSweptBounds(unit);
	}

	void TurnStart();
	void Interpolate(float frameTime, float frameOffset);
	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling);

	void UpdateVisibility(SUnit& unit);

	virtual float GetFrameOffset()
	{
		return m_FrameOffset;
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_EnableDebugOverlays = enabled;
	}
};

void CCmpUnitRenderer::TurnStart()
{
	PROFILE3("UnitRenderer::TurnStart");

	// Assume units have stopped moving after the previous turn. If that assumption is not
	// correct, we will get a UpdateUnitPos to tell us about its movement in the new turn.
	for (size_t i = 0; i < m_Units.size(); i++)
	{
		SUnit& unit = m_Units[i];
		unit.pos0 = unit.pos1;
		unit.sweptBounds = CBoundingSphere(unit.pos1, unit.boundsApprox.GetRadius());

		// Visibility must be recomputed on the first frame during this turn
		unit.visibilityDirty = true;
	}
}

void CCmpUnitRenderer::Interpolate(float frameTime, float frameOffset)
{
	PROFILE3("UnitRenderer::Interpolate");

	++m_FrameNumber;
	m_FrameOffset = frameOffset;

	// TODO: we shouldn't update all the animations etc for units that are off-screen
	// (but need to be careful about e.g. sounds triggered by animations of off-screen
	// units)
	for (size_t i = 0; i < m_Units.size(); i++)
	{
		SUnit& unit = m_Units[i];
		if (unit.actor)
			unit.actor->UpdateModel(frameTime);
	}

	m_DebugSpheres.clear();
	if (m_EnableDebugOverlays)
	{
		for (size_t i = 0; i < m_Units.size(); i++)
		{
			SUnit& unit = m_Units[i];
			if (!(unit.actor && unit.inWorld))
				continue;

			SOverlaySphere sphere;
			sphere.m_Center = unit.sweptBounds.GetCenter();
			sphere.m_Radius = unit.sweptBounds.GetRadius();
			if (unit.culled)
				sphere.m_Color = CColor(1.0f, 0.5f, 0.5f, 0.5f);
			else
				sphere.m_Color = CColor(0.5f, 0.5f, 1.0f, 0.5f);

			m_DebugSpheres.push_back(sphere);
		}
	}
}

void CCmpUnitRenderer::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	// TODO: need a coarse culling pass based on some kind of spatial data
	// structure - that's the main point of this design. Once we've got a
	// rough list of possibly-visible units, then we can do the more precise
	// culling. (And once it's cheap enough, we can do multiple culling passes
	// per frame - one for shadow generation, one for water reflections, etc.)

	PROFILE3("UnitRenderer::RenderSubmit");

	for (size_t i = 0; i < m_Units.size(); ++i)
	{
		SUnit& unit = m_Units[i];

		unit.culled = true;

		if (!(unit.actor && unit.inWorld))
			continue;

		if (!g_AtlasGameLoop->running && !g_RenderActors && (unit.flags & ACTOR_ONLY))
			continue;

		if (!g_AtlasGameLoop->running && (unit.flags & VISIBLE_IN_ATLAS_ONLY))
			continue;

		if (culling && !frustum.IsSphereVisible(unit.sweptBounds.GetCenter(), unit.sweptBounds.GetRadius()))
			continue;

		if (unit.visibilityDirty)
			UpdateVisibility(unit);
		if (unit.visibility == ICmpRangeManager::VIS_HIDDEN)
			continue;

		unit.culled = false;

		CModelAbstract& unitModel = unit.actor->GetModel();

		if (unit.lastTransformFrame != m_FrameNumber)
		{
			CmpPtr<ICmpPosition> cmpPosition(unit.entity);
			if (!cmpPosition)
				continue;

			CMatrix3D transform(cmpPosition->GetInterpolatedTransform(m_FrameOffset));

			unitModel.SetTransform(transform);

			unit.lastTransformFrame = m_FrameNumber;
		}

		if (culling && !frustum.IsBoxVisible(CVector3D(0, 0, 0), unitModel.GetWorldBoundsRec()))
			continue;

		collector.SubmitRecursive(&unitModel);
	}

	for (size_t i = 0; i < m_DebugSpheres.size(); ++i)
		collector.Submit(&m_DebugSpheres[i]);
}

void CCmpUnitRenderer::UpdateVisibility(SUnit& unit)
{
	if (unit.inWorld)
	{
		// The 'always visible' flag means we should always render the unit
		// (regardless of whether the LOS system thinks it's visible)
		CmpPtr<ICmpVision> cmpVision(unit.entity);
		if (cmpVision && cmpVision->GetAlwaysVisible())
			unit.visibility = ICmpRangeManager::VIS_VISIBLE;
		else
		{
			CmpPtr<ICmpRangeManager> cmpRangeManager(GetSystemEntity());
			unit.visibility = cmpRangeManager->GetLosVisibility(unit.entity,
					GetSimContext().GetCurrentDisplayedPlayer());
		}
	}
	else
		unit.visibility = ICmpRangeManager::VIS_HIDDEN;

	// Change the visibility of the visual actor's selectable if it has one.
	CmpPtr<ICmpSelectable> cmpSelectable(unit.entity);
	if (cmpSelectable)
		cmpSelectable->SetVisibility(unit.visibility != ICmpRangeManager::VIS_HIDDEN);

	unit.visibilityDirty = false;
}

REGISTER_COMPONENT_TYPE(UnitRenderer)
