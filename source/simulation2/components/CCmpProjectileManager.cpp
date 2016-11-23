/* Copyright (C) 2015 Wildfire Games.
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
#include "ICmpProjectileManager.h"

#include "ICmpObstruction.h"
#include "ICmpObstructionManager.h"
#include "ICmpPosition.h"
#include "ICmpRangeManager.h"
#include "ICmpTerrain.h"
#include "ICmpVisual.h"
#include "simulation2/MessageTypes.h"

#include "graphics/Frustum.h"
#include "graphics/Model.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/Matrix3D.h"
#include "maths/Quaternion.h"
#include "maths/Vector3D.h"
#include "ps/CLogger.h"
#include "renderer/Scene.h"

// Time (in seconds) before projectiles that stuck in the ground are destroyed
const static float PROJECTILE_DECAY_TIME = 30.f;

class CCmpProjectileManager : public ICmpProjectileManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Interpolate);
		componentManager.SubscribeToMessageType(MT_RenderSubmit);
	}

	DEFAULT_COMPONENT_ALLOCATOR(ProjectileManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_ActorSeed = 0;
		m_NextId = 1;
	}

	virtual void Deinit()
	{
		for (size_t i = 0; i < m_Projectiles.size(); ++i)
			GetSimContext().GetUnitManager().DeleteUnit(m_Projectiles[i].unit);
		m_Projectiles.clear();
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// Because this is just graphical effects, and because it's all non-deterministic floating point,
		// we don't do much serialization here.
		// (That means projectiles will vanish if you save/load - is that okay?)

		// The attack code stores the id so that the projectile gets deleted when it hits the target
		serialize.NumberU32_Unbounded("next id", m_NextId);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		// The attack code stores the id so that the projectile gets deleted when it hits the target
		deserialize.NumberU32_Unbounded("next id", m_NextId);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Interpolate:
		{
			const CMessageInterpolate& msgData = static_cast<const CMessageInterpolate&> (msg);
			Interpolate(msgData.deltaSimTime);
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

	virtual uint32_t LaunchProjectileAtPoint(entity_id_t source, const CFixedVector3D& target, fixed speed, fixed gravity)
	{
		return LaunchProjectile(source, target, speed, gravity);
	}

	virtual void RemoveProjectile(uint32_t);

private:
	struct Projectile
	{
		CUnit* unit;
		CVector3D origin;
		CVector3D pos;
		CVector3D v;
		float time;
		float timeHit;
		float gravity;
		bool stopped;
		uint32_t id;

		CVector3D position(float t)
		{
			float t2 = t;
			if (t2 > timeHit)
				t2 = timeHit + logf(1.f + t2 - timeHit);

			CVector3D ret(origin);
			ret.X += v.X * t2;
			ret.Z += v.Z * t2;
			ret.Y += v.Y * t2 - 0.5f * gravity * t * t;
			return ret;
		}
	};

	std::vector<Projectile> m_Projectiles;

	uint32_t m_ActorSeed;

	uint32_t m_NextId;

	uint32_t LaunchProjectile(entity_id_t source, CFixedVector3D targetPoint, fixed speed, fixed gravity);

	void AdvanceProjectile(Projectile& projectile, float dt);

	void Interpolate(float frameTime);

	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling);
};

REGISTER_COMPONENT_TYPE(ProjectileManager)

uint32_t CCmpProjectileManager::LaunchProjectile(entity_id_t source, CFixedVector3D targetPoint, fixed speed, fixed gravity)
{
	// This is network synced so don't use GUI checks before incrementing or it breaks any non GUI simulations
	uint32_t currentId = m_NextId++;

	if (!GetSimContext().HasUnitManager())
		return currentId; // do nothing if graphics are disabled

	CmpPtr<ICmpVisual> cmpSourceVisual(GetSimContext(), source);
	if (!cmpSourceVisual)
		return currentId;

	std::wstring name = cmpSourceVisual->GetProjectileActor();
	if (name.empty())
	{
		// If the actor was actually loaded, complain that it doesn't have a projectile
		if (!cmpSourceVisual->GetActorShortName().empty())
			LOGERROR("Unit with actor '%s' launched a projectile but has no actor on 'projectile' attachpoint", utf8_from_wstring(cmpSourceVisual->GetActorShortName()));
		return currentId;
	}

	Projectile projectile;
	projectile.id = currentId;
	projectile.time = 0.f;
	projectile.stopped = false;
	projectile.gravity = gravity.ToFloat();

	projectile.origin = cmpSourceVisual->GetProjectileLaunchPoint();
	if (!projectile.origin)
	{
		// If there's no explicit launch point, take a guess based on the entity position
		CmpPtr<ICmpPosition> sourcePos(GetSimContext(), source);
		if (!sourcePos)
			return currentId;
		projectile.origin = sourcePos->GetPosition();
		projectile.origin.Y += 3.f;
	}

	std::set<CStr> selections;
	projectile.unit = GetSimContext().GetUnitManager().CreateUnit(name, m_ActorSeed++, selections);
	if (!projectile.unit) // The error will have already been logged
		return currentId;

	projectile.pos = projectile.origin;
	CVector3D offset(targetPoint);
	offset -= projectile.pos;
	float horizDistance = sqrtf(offset.X*offset.X + offset.Z*offset.Z);
	projectile.timeHit = horizDistance / speed.ToFloat();
	projectile.v = offset * (1.f / projectile.timeHit);

	projectile.v.Y = offset.Y / projectile.timeHit  + 0.5f * projectile.gravity * projectile.timeHit;

	m_Projectiles.push_back(projectile);

	return projectile.id;
}

void CCmpProjectileManager::AdvanceProjectile(Projectile& projectile, float dt)
{
	projectile.time += dt;
	if (projectile.stopped)
		return;

	CVector3D delta;
	if (dt < 0.1f)
		delta = projectile.pos;
	else // For big dt delta is unprecise
		delta = projectile.position(projectile.time - 0.1f);

	projectile.pos = projectile.position(projectile.time);

	delta = projectile.pos - delta;

	// If we've passed the target position and haven't stopped yet,
	// carry on until we reach solid land
	if (projectile.time >= projectile.timeHit)
	{
		CmpPtr<ICmpTerrain> cmpTerrain(GetSystemEntity());
		if (cmpTerrain)
		{
			float h = cmpTerrain->GetExactGroundLevel(projectile.pos.X, projectile.pos.Z);
			if (projectile.pos.Y < h)
			{
				projectile.pos.Y = h; // stick precisely to the terrain
				projectile.stopped = true;
			}
		}
	}

	// Construct a rotation matrix so that (0,1,0) is in the direction of 'delta'

	CVector3D up(0, 1, 0);

	delta.Normalize();
	CVector3D axis = up.Cross(delta);
	if (axis.LengthSquared() < 0.0001f)
		axis = CVector3D(1, 0, 0); // if up & delta are almost collinear, rotate around some other arbitrary axis
	else
		axis.Normalize();

	float angle = acosf(up.Dot(delta));

	CMatrix3D transform;
	CQuaternion quat;
	quat.FromAxisAngle(axis, angle);
	quat.ToMatrix(transform);

	// Then apply the translation
	transform.Translate(projectile.pos);

	// Move the model
	projectile.unit->GetModel().SetTransform(transform);
}

void CCmpProjectileManager::Interpolate(float frameTime)
{
	for (size_t i = 0; i < m_Projectiles.size(); ++i)
	{
		AdvanceProjectile(m_Projectiles[i], frameTime);
	}

	// Remove the ones that have reached their target
	for (size_t i = 0; i < m_Projectiles.size(); )
	{
		// Projectiles hitting targets get removed immediately.
		// Those hitting the ground stay for a while, because it looks pretty.
		if (m_Projectiles[i].stopped)
		{
			if (m_Projectiles[i].time - m_Projectiles[i].timeHit > PROJECTILE_DECAY_TIME)
			{
				// Delete in-place by swapping with the last in the list
				std::swap(m_Projectiles[i], m_Projectiles.back());
				GetSimContext().GetUnitManager().DeleteUnit(m_Projectiles.back().unit);
				m_Projectiles.pop_back();
				continue; // don't increment i
			}
		}

		++i;
	}
}

void CCmpProjectileManager::RemoveProjectile(uint32_t id)
{
	// Scan through the projectile list looking for one with the correct id to remove
	for (size_t i = 0; i < m_Projectiles.size(); i++)
	{
		if (m_Projectiles[i].id == id)
		{
			// Delete in-place by swapping with the last in the list
			std::swap(m_Projectiles[i], m_Projectiles.back());
			GetSimContext().GetUnitManager().DeleteUnit(m_Projectiles.back().unit);
			m_Projectiles.pop_back();
			return;
		}
	}
}

void CCmpProjectileManager::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	CmpPtr<ICmpRangeManager> cmpRangeManager(GetSystemEntity());
	int player = GetSimContext().GetCurrentDisplayedPlayer();
	ICmpRangeManager::CLosQuerier los(cmpRangeManager->GetLosQuerier(player));
	bool losRevealAll = cmpRangeManager->GetLosRevealAll(player);

	for (size_t i = 0; i < m_Projectiles.size(); ++i)
	{
		// Don't display projectiles outside the visible area
		ssize_t posi = (ssize_t)(0.5f + m_Projectiles[i].pos.X / TERRAIN_TILE_SIZE);
		ssize_t posj = (ssize_t)(0.5f + m_Projectiles[i].pos.Z / TERRAIN_TILE_SIZE);
		if (!losRevealAll && !los.IsVisible(posi, posj))
			continue;

		CModelAbstract& model = m_Projectiles[i].unit->GetModel();

		model.ValidatePosition();

		if (culling && !frustum.IsBoxVisible(model.GetWorldBoundsRec()))
			continue;

		// TODO: do something about LOS (copy from CCmpVisualActor)

		collector.SubmitRecursive(&model);
	}
}
