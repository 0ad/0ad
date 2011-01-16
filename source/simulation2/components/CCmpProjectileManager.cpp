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
#include "ICmpProjectileManager.h"

#include "ICmpPosition.h"
#include "ICmpRangeManager.h"
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
const static float PROJECTILE_DECAY_TIME = 60.f;

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
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& UNUSED(serialize))
	{
		// Because this is just graphical effects, and because it's all non-deterministic floating point,
		// we don't do any serialization here.
		// (That means projectiles will vanish if you save/load - is that okay?)
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& UNUSED(deserialize))
	{
		Init(paramNode);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
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
		}
	}

	virtual void LaunchProjectileAtEntity(entity_id_t source, entity_id_t target, fixed speed, fixed gravity)
	{
		LaunchProjectile(source, CFixedVector3D(), target, speed, gravity);
	}

	virtual void LaunchProjectileAtPoint(entity_id_t source, CFixedVector3D target, fixed speed, fixed gravity)
	{
		LaunchProjectile(source, target, INVALID_ENTITY, speed, gravity);
	}

private:
	struct Projectile
	{
		CUnit* unit;
		CVector3D pos;
		CVector3D target;
		entity_id_t targetEnt; // INVALID_ENTITY if the target is just a point
		float timeLeft;
		float horizSpeed;
		float gravity;
	};

	std::vector<Projectile> m_Projectiles;

	void LaunchProjectile(entity_id_t source, CFixedVector3D targetPoint, entity_id_t targetEnt, fixed speed, fixed gravity);

	void AdvanceProjectile(Projectile& projectile, float dt, float frameOffset);

	void Interpolate(float frameTime, float frameOffset);

	void RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling);
};

REGISTER_COMPONENT_TYPE(ProjectileManager)

void CCmpProjectileManager::LaunchProjectile(entity_id_t source, CFixedVector3D targetPoint, entity_id_t targetEnt, fixed speed, fixed gravity)
{
	if (!GetSimContext().HasUnitManager())
		return; // do nothing if graphics are disabled

	CmpPtr<ICmpVisual> sourceVisual(GetSimContext(), source);
	if (sourceVisual.null())
		return;

	std::wstring name = sourceVisual->GetProjectileActor();
	if (name.empty())
	{
		// If the actor was actually loaded, complain that it doesn't have a projectile
		if (!sourceVisual->GetActorShortName().empty())
			LOGERROR(L"Unit with actor '%ls' launched a projectile but has no actor on 'projectile' attachpoint", sourceVisual->GetActorShortName().c_str());
		return;
	}

	std::set<CStr> selections;

	Projectile projectile;
	projectile.unit = GetSimContext().GetUnitManager().CreateUnit(name, selections);
	if (!projectile.unit)
	{
		// The error will have already been logged
		return;
	}

	CVector3D sourceVec(sourceVisual->GetProjectileLaunchPoint());
	if (!sourceVec)
	{
		// If there's no explicit launch point, take a guess based on the entity position

		CmpPtr<ICmpPosition> sourcePos(GetSimContext(), source);
		if (sourcePos.null())
			return;

		sourceVec = sourcePos->GetPosition();
		sourceVec.Y += 3.f;
	}

	CVector3D targetVec;

	if (targetEnt == INVALID_ENTITY)
	{
		targetVec = CVector3D(targetPoint);
	}
	else
	{
		CmpPtr<ICmpPosition> targetPos(GetSimContext(), targetEnt);
		if (targetPos.null())
			return;

		targetVec = CVector3D(targetPos->GetPosition());
	}

	projectile.pos = sourceVec;
	projectile.target = targetVec;
	projectile.targetEnt = targetEnt;

	CVector3D offset = projectile.target - projectile.pos;
	float horizDistance = hypot(offset.X, offset.Z);

	projectile.horizSpeed = speed.ToFloat();
	projectile.timeLeft = horizDistance / projectile.horizSpeed;

	projectile.gravity = gravity.ToFloat();

	m_Projectiles.push_back(projectile);
}

void CCmpProjectileManager::AdvanceProjectile(Projectile& projectile, float dt, float frameOffset)
{
	// Do nothing if we've already reached the target
	if (projectile.timeLeft <= 0)
		return;

	// Clamp dt to the remaining travel time
	if (dt > projectile.timeLeft)
		dt = projectile.timeLeft;

	// Track the target entity (if there is one, and it's still alive)
	if (projectile.targetEnt != INVALID_ENTITY)
	{
		CmpPtr<ICmpPosition> targetPos(GetSimContext(), projectile.targetEnt);
		if (!targetPos.null())
		{
			CMatrix3D t = targetPos->GetInterpolatedTransform(frameOffset, false);
			projectile.target = t.GetTranslation();
			projectile.target.Y += 2.f; // TODO: ought to aim towards a random point in the solid body of the target

			// TODO: if the unit is moving, we should probably aim a bit in front of it
			// so we don't have to curve so much just before reaching it
		}
	}

	CVector3D offset = projectile.target - projectile.pos;

	// Compute the vertical velocity that's needed so we travel in a ballistic curve and
	// reach the target after timeLeft.
	// (This is just a linear approximation to the curve, but it'll converge to hit the target)
	float vh = (projectile.gravity / 2.f) * projectile.timeLeft + offset.Y / projectile.timeLeft;

	// Move an appropriate fraction towards the target
	CVector3D delta (offset.X * dt/projectile.timeLeft, vh * dt, offset.Z * dt/projectile.timeLeft);

	projectile.pos += delta;
	projectile.timeLeft -= dt;

	// If we're really close to the target now, delete the remaining time so that we don't do a little
	// tiny numerically-imprecise movement next frame
	if (projectile.timeLeft < 0.01f)
	{
		projectile.timeLeft = 0;
		// Move exactly to the target, so we hit it precisely even if
		// the framerate is very low
		projectile.pos = projectile.target;
	}

	// Construct a rotation matrix so that (0,1,0) is in the direction of 'delta'

	CMatrix3D transform;
	CVector3D up(0, 1, 0);

	delta.Normalize();
	CVector3D axis = up.Cross(delta);
	if (axis.LengthSquared() < 0.0001f)
		axis = CVector3D(1, 0, 0); // if up & delta are almost collinear, rotate around some other arbitrary axis
	else
		axis.Normalize();

	float angle = acos(up.Dot(delta));
	CQuaternion quat;
	quat.FromAxisAngle(axis, angle);
	quat.ToMatrix(transform);

	// Then apply the translation
	transform.Translate(projectile.pos);

	// Move the model
	projectile.unit->GetModel().SetTransform(transform);
}

void CCmpProjectileManager::Interpolate(float frameTime, float frameOffset)
{
	for (size_t i = 0; i < m_Projectiles.size(); ++i)
	{
		AdvanceProjectile(m_Projectiles[i], frameTime, frameOffset);
	}

	// Remove the ones that have reached their target
	for (size_t i = 0; i < m_Projectiles.size(); )
	{
		// Projectiles hitting targets get removed immediately.
		// Those hitting the ground stay for a while, because it looks pretty.
		if (m_Projectiles[i].timeLeft <= 0.f)
		{
			if (m_Projectiles[i].targetEnt == INVALID_ENTITY && m_Projectiles[i].timeLeft > -PROJECTILE_DECAY_TIME)
			{
				// AdvanceProjectile doesn't update timeLeft if the projectile's
				// already finished, so just update its timer here
				m_Projectiles[i].timeLeft -= frameTime;
			}
			else
			{
				// Delete in-place by swapping with the last in the list
				std::swap(m_Projectiles[i], m_Projectiles.back());
				m_Projectiles.pop_back();
				continue; // don't increment i
			}
		}

		++i;
	}
}

void CCmpProjectileManager::RenderSubmit(SceneCollector& collector, const CFrustum& frustum, bool culling)
{
	CmpPtr<ICmpRangeManager> cmpRangeManager(GetSimContext(), SYSTEM_ENTITY);
	int player = GetSimContext().GetCurrentDisplayedPlayer();
	ICmpRangeManager::CLosQuerier los (cmpRangeManager->GetLosQuerier(player));
	bool losRevealAll = cmpRangeManager->GetLosRevealAll(player);

	for (size_t i = 0; i < m_Projectiles.size(); ++i)
	{
		// Don't display projectiles outside the visible area
		ssize_t posi = (ssize_t)(0.5f + m_Projectiles[i].pos.X / CELL_SIZE);
		ssize_t posj = (ssize_t)(0.5f + m_Projectiles[i].pos.Z / CELL_SIZE);
		if (!losRevealAll && !los.IsVisible(posi, posj))
			continue;

		CModel& model = m_Projectiles[i].unit->GetModel();

		model.ValidatePosition();

		if (culling && !frustum.IsBoxVisible(CVector3D(0, 0, 0), model.GetBounds()))
			continue;

		// TODO: do something about LOS (copy from CCmpVisualActor)

		collector.SubmitRecursive(&model);
	}
}
