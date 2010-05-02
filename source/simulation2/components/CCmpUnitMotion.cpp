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
#include "ICmpUnitMotion.h"

#include "ICmpObstruction.h"
#include "ICmpObstructionManager.h"
#include "ICmpPosition.h"
#include "ICmpPathfinder.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Render.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/FixedVector2D.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

static const entity_pos_t WAYPOINT_ADVANCE_MIN = entity_pos_t::FromInt(CELL_SIZE*4);
static const entity_pos_t WAYPOINT_ADVANCE_MAX = entity_pos_t::FromInt(CELL_SIZE*8);
static const entity_pos_t SHORT_PATH_SEARCH_RANGE = entity_pos_t::FromInt(CELL_SIZE*12);

static const CColor OVERLAY_COLOUR_PATH(1, 1, 1, 1);
static const CColor OVERLAY_COLOUR_PATH_ACTIVE(1, 1, 0, 1);
static const CColor OVERLAY_COLOUR_SHORT_PATH(1, 0, 0, 1);

class CCmpUnitMotion : public ICmpUnitMotion
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update);
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotion)

	bool m_DebugOverlayEnabled;
	std::vector<SOverlayLine> m_DebugOverlayLines;
	std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	// Template state:

	fixed m_Speed; // in units per second
	entity_pos_t m_Radius;

	// Dynamic state:

	bool m_HasTarget; // whether we currently have valid paths and targets
	// These values contain undefined junk if !HasTarget:
	ICmpPathfinder::Path m_Path;
	ICmpPathfinder::Path m_ShortPath;
	entity_pos_t m_ShortTargetX, m_ShortTargetZ;
	ICmpPathfinder::Goal m_FinalGoal;

	enum
	{
		IDLE,
		WALKING,
		STOPPING
	};
	int m_State;

	static std::string GetSchema()
	{
		return
			"<a:help>Provides the unit with the ability to move around the world by itself.</a:help>"
			"<a:example>"
				"<WalkSpeed>7.0</WalkSpeed>"
			"</a:example>"
			"<element name='WalkSpeed' a:help='Basic movement speed (in metres per second)'>"
				"<ref name='positiveDecimal'/>"
			"</element>";
	}

	virtual void Init(const CSimContext& context, const CParamNode& paramNode)
	{
		m_HasTarget = false;

		m_Speed = paramNode.GetChild("WalkSpeed").ToFixed();

		CmpPtr<ICmpObstruction> cmpObstruction(context, GetEntityId());
		if (!cmpObstruction.null())
			m_Radius = cmpObstruction->GetUnitRadius();

		m_State = IDLE;

		m_DebugOverlayEnabled = false;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		serialize.Bool("has target", m_HasTarget);
		if (m_HasTarget)
		{
			// TODO: m_Path
			// TODO: m_FinalTargetAngle
		}

		// TODO: m_State
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

		deserialize.Bool(m_HasTarget);
		if (m_HasTarget)
		{
		}
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Update:
		{
			fixed dt = static_cast<const CMessageUpdate&> (msg).turnLength;

			if (m_State == STOPPING)
			{
				m_State = IDLE;
				CMessageMotionChanged msg(fixed::Zero());
				context.GetComponentManager().PostMessage(GetEntityId(), msg);
			}

			Move(dt);

			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
			break;
		}
		}
	}

	virtual fixed GetSpeed()
	{
		return m_Speed;
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlayEnabled = enabled;
		if (enabled)
		{
			RenderPath(m_Path, m_DebugOverlayLines, OVERLAY_COLOUR_PATH);
			RenderPath(m_ShortPath, m_DebugOverlayShortPathLines, OVERLAY_COLOUR_SHORT_PATH);
		}
	}

	virtual bool MoveToPoint(entity_pos_t x, entity_pos_t z);
	virtual bool MoveToAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool IsInAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);

private:
	/**
	 * Check whether moving from pos to target is safe (won't hit anything).
	 * If safe, returns true (the caller should do cmpPosition->MoveTo).
	 * Otherwise returns false, and either computes a new path to use on the
	 * next turn or makes the unit stop.
	 */
	bool CheckMovement(CFixedVector2D pos, CFixedVector2D target);

	/**
	 * Do the per-turn movement and other updates
	 */
	void Move(fixed dt);

	void StopAndFaceGoal(CFixedVector2D pos);

	/**
	 * Rotate to face towards the target point, given the current pos
	 */
	void FaceTowardsPoint(CFixedVector2D pos, entity_pos_t x, entity_pos_t z);

	/**
	 * Change between idle/walking states; automatically sends MotionChanged messages when appropriate
	 */
	void SwitchState(int state);

	bool ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t hw, entity_pos_t hh, entity_pos_t circleRadius);

	/**
	 * Recompute the whole path to the current goal.
	 * Returns false on error or if the unit can't move anywhere at all.
	 */
	bool RegeneratePath(CFixedVector2D pos, bool avoidMovingUnits);

	/**
	 * Maybe select a new long waypoint if we're getting too close to the
	 * current one.
	 */
	void MaybePickNextWaypoint(const CFixedVector2D& pos);

	/**
	 * Select a next long waypoint, given the current unit position.
	 * Also recomputes the short path to use that waypoint.
	 * Returns false on error, or if there is no waypoint to pick.
	 */
	bool PickNextWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits);

	/**
	 * Select a new short waypoint as the current target,
	 * which possibly involves first selecting a new long waypoint.
	 * Returns false on error, or if there is no waypoint to pick.
	 */
	bool PickNextShortWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits);

	/**
	 * Convert a path into a renderable list of lines
	 */
	void RenderPath(const ICmpPathfinder::Path& path, std::vector<SOverlayLine>& lines, CColor color);

	void RenderSubmit(SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(UnitMotion)

bool CCmpUnitMotion::CheckMovement(CFixedVector2D pos, CFixedVector2D target)
{
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	NullObstructionFilter filter;

	entity_pos_t delta = entity_pos_t::FromInt(1) / 8;
		// add a small delta so that we don't get so close that the pathfinder thinks
		// we've actually crossed the edge (given minor numerical inaccuracies)
	// TODO: keep this in sync with CCmpPathfinder::ComputeShortPath delta
	// (this value needs to be smaller)
	// TODO: work out what this value should actually be, rather than just guessing

	if (cmpObstructionManager->TestLine(filter, pos.X, pos.Y, target.X, target.Y, m_Radius + delta))
	{
		// Oops, hit something
		// TODO: we ought to wait for obstructions to move away instead of immediately throwing away the whole path
		// TODO: actually a whole proper collision resolution thing needs to be designed and written
		if (!RegeneratePath(pos, true))
		{
			// Oh dear, we can't find the path any more; give up
			StopAndFaceGoal(pos);
			return false;
		}
		// Wait for the next Update before we try moving again
		return false;
	}

	return true;
}

void CCmpUnitMotion::Move(fixed dt)
{
	PROFILE("Move");

	if (!m_HasTarget)
		return;

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null())
		return;

	CFixedVector3D pos3 = cmpPosition->GetPosition();
	CFixedVector2D pos (pos3.X, pos3.Z);

	// We want to move (at most) m_Speed*dt units from pos towards the next waypoint

	while (dt > fixed::Zero())
	{
		CFixedVector2D target(m_ShortTargetX, m_ShortTargetZ);
		CFixedVector2D offset = target - pos;

		// Face towards the target
		entity_angle_t angle = atan2_approx(offset.X, offset.Y);
		cmpPosition->TurnTo(angle);

		// Work out how far we can travel in dt
		fixed maxdist = m_Speed.Multiply(dt);

		// If the target is close, we can move there directly
		fixed offsetLength = offset.Length();
		if (offsetLength <= maxdist)
		{
			if (!CheckMovement(pos, target))
				return;

			pos = target;
			cmpPosition->MoveTo(pos.X, pos.Y);

			// Spend the rest of the time heading towards the next waypoint
			dt = dt - (offset.Length() / m_Speed);
			MaybePickNextWaypoint(pos);
			if (PickNextShortWaypoint(pos, false))
				continue;

			// We ran out of usable waypoints, so stop now
			StopAndFaceGoal(pos);
			return;
		}
		else
		{
			// Not close enough, so just move in the right direction
			offset.Normalize(maxdist);
			target = pos + offset;

			if (!CheckMovement(pos, target))
				return;

			pos = target;
			cmpPosition->MoveTo(pos.X, pos.Y);

			MaybePickNextWaypoint(pos);

			return;
		}
	}
}

void CCmpUnitMotion::StopAndFaceGoal(CFixedVector2D pos)
{
	SwitchState(IDLE);
	FaceTowardsPoint(pos, m_FinalGoal.x, m_FinalGoal.z);

	// TODO: if the goal was a square building, we ought to point towards the
	// nearest point on the square, not towards its center
}

void CCmpUnitMotion::FaceTowardsPoint(CFixedVector2D pos, entity_pos_t x, entity_pos_t z)
{
	CFixedVector2D target(x, z);
	CFixedVector2D offset = target - pos;
	if (!offset.IsZero())
	{
		entity_angle_t angle = atan2_approx(offset.X, offset.Y);

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return;
		cmpPosition->TurnTo(angle);
	}
}

void CCmpUnitMotion::SwitchState(int state)
{
	debug_assert(state == IDLE || state == WALKING);

	if (state == IDLE)
		m_HasTarget = false;

	// IDLE -> IDLE -- no change
	// IDLE -> WALKING -- send a MotionChanged(speed) message
	// WALKING -> IDLE -- set to STOPPING, so we'll send MotionChanged(0) in the next Update
	// WALKING -> WALKING -- send a MotionChanged(speed) message
	// STOPPING -> IDLE -- stay in STOPPING
	// STOPPING -> WALKING -- set to WALKING, send MotionChanged(speed)

	if (state == WALKING)
	{
		CMessageMotionChanged msg(m_Speed);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	if (m_State == IDLE && state == WALKING)
	{
		m_State = WALKING;
		return;
	}

	if (m_State == WALKING && state == IDLE)
	{
		m_State = STOPPING;
		return;
	}

	if (m_State == STOPPING && state == IDLE)
	{
		return;
	}

	if (m_State == STOPPING && state == WALKING)
	{
		m_State = WALKING;
		return;
	}
}

bool CCmpUnitMotion::MoveToPoint(entity_pos_t x, entity_pos_t z)
{
	PROFILE("MoveToPoint");

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector3D pos3 = cmpPosition->GetPosition();
	CFixedVector2D pos (pos3.X, pos3.Z);

	// Reset any current movement
	m_HasTarget = false;

	ICmpPathfinder::Goal goal;

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	ICmpObstructionManager::ObstructionSquare obstruction;
	if (cmpObstructionManager->FindMostImportantObstruction(x, z, m_Radius, obstruction))
	{
		// If we're aiming inside a building, then aim for the outline of the building instead
		// TODO: if we're aiming at a unit then maybe a circle would look nicer?

		goal.type = ICmpPathfinder::Goal::SQUARE;
		goal.x = obstruction.x;
		goal.z = obstruction.z;
		goal.u = obstruction.u;
		goal.v = obstruction.v;
		entity_pos_t delta = entity_pos_t::FromInt(1) / 4; // nudge the goal outwards so it doesn't intersect the building itself
		goal.hw = obstruction.hw + m_Radius + delta;
		goal.hh = obstruction.hh + m_Radius + delta;
	}
	else
	{
		// Unobstructed - head directly for the goal
		goal.type = ICmpPathfinder::Goal::POINT;
		goal.x = x;
		goal.z = z;
	}


	m_FinalGoal = goal;
	if (!RegeneratePath(pos, false))
		return false;

	SwitchState(WALKING);
	return true;
}

bool CCmpUnitMotion::ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t hw, entity_pos_t hh, entity_pos_t circleRadius)
{
	// Given a square, plus a target range we should reach, the shape at that distance
	// is a round-cornered square which we can approximate as either a circle or as a square.
	// Choose the shape that will minimise the worst-case error:

	// For a square, error is (sqrt(2)-1) * range at the corners
	entity_pos_t errSquare = (entity_pos_t::FromInt(4142)/10000).Multiply(range);

	// For a circle, error is radius-hw at the sides and radius-hh at the top/bottom
	entity_pos_t errCircle = circleRadius - std::min(hw, hh);

	return (errCircle < errSquare);
}

bool CCmpUnitMotion::MoveToAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	PROFILE("MoveToAttackRange");

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector3D pos3 = cmpPosition->GetPosition();
	CFixedVector2D pos (pos3.X, pos3.Z);

	// Reset any current movement
	m_HasTarget = false;

	ICmpPathfinder::Goal goal;

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	ICmpObstructionManager::tag_t tag = 0;

	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), target);
	if (!cmpObstruction.null())
		tag = cmpObstruction->GetObstruction();

	/*
	 * If we're starting outside the maxRange, we need to move closer in.
	 * If we're starting inside the minRange, we need to move further out.
	 * These ranges are measured from the center of this entity to the edge of the target;
	 * we add the goal range onto the size of the target shape to get the goal shape.
	 * (Then we extend it outwards/inwards by a little bit to be sure we'll end up
	 * within the right range, in case of minor numerical inaccuracies.)
	 *
	 * There's a bit of a problem with large square targets:
	 * the pathfinder only lets us move to goals that are squares, but the points an equal
	 * distance from the target make a rounded square shape instead.
	 *
	 * When moving closer, we could shrink the goal radius to 1/sqrt(2) so the goal shape fits entirely
	 * within the desired rounded square, but that gives an unfair advantage to attackers who approach
	 * the target diagonally.
	 *
	 * If the target is small relative to the range (e.g. archers attacking anything),
	 * then we cheat and pretend the target is actually a circle.
	 * (TODO: that probably looks rubbish for things like walls?)
	 *
	 * If the target is large relative to the range (e.g. melee units attacking buildings),
	 * then we multiply maxRange by approx 1/sqrt(2) to guarantee they'll always aim close enough.
	 * (Those units should set minRange to 0 so they'll never be considered *too* close.)
	 */

	const entity_pos_t goalDelta = entity_pos_t::FromInt(CELL_SIZE)/4; // for extending the goal outwards/inwards a little bit

	if (tag)
	{
		ICmpObstructionManager::ObstructionSquare obstruction = cmpObstructionManager->GetObstruction(tag);

		CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
		goal.x = obstruction.x;
		goal.z = obstruction.z;

		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize);

		if (distance < minRange)
		{
			// Too close to the square - need to move away

			entity_pos_t goalDistance = minRange + goalDelta;

			goal.type = ICmpPathfinder::Goal::SQUARE;
			goal.u = obstruction.u;
			goal.v = obstruction.v;
			entity_pos_t delta = std::max(goalDistance, m_Radius + entity_pos_t::FromInt(CELL_SIZE)/16); // ensure it's far enough to not intersect the building itself
			goal.hw = obstruction.hw + delta;
			goal.hh = obstruction.hh + delta;
		}
		else if (distance < maxRange)
		{
			// We're already in range - no need to move anywhere
			FaceTowardsPoint(pos, goal.x, goal.z);
			return false;
		}
		else
		{
			// We might need to move closer:

			// Circumscribe the square
			entity_pos_t circleRadius = halfSize.Length();

			if (ShouldTreatTargetAsCircle(maxRange, obstruction.hw, obstruction.hh, circleRadius))
			{
				// The target is small relative to our range, so pretend it's a circle

				// Note that the distance to the circle will always be less than
				// the distance to the square, so the previous "distance < maxRange"
				// check is still valid (though not sufficient)
				entity_pos_t circleDistance = (pos - CFixedVector2D(obstruction.x, obstruction.z)).Length() - circleRadius;

				if (circleDistance < maxRange)
				{
					// We're already in range - no need to move anywhere
					FaceTowardsPoint(pos, goal.x, goal.z);
					return false;
				}

				entity_pos_t goalDistance = maxRange - goalDelta;

				goal.type = ICmpPathfinder::Goal::CIRCLE;
				goal.hw = circleRadius + goalDistance;
			}
			else
			{
				// The target is large relative to our range, so treat it as a square and
				// get close enough that the diagonals come within range

				entity_pos_t goalDistance = (maxRange - goalDelta)*2 / 3; // multiply by slightly less than 1/sqrt(2)

				goal.type = ICmpPathfinder::Goal::SQUARE;
				goal.u = obstruction.u;
				goal.v = obstruction.v;
				entity_pos_t delta = std::max(goalDistance, m_Radius + entity_pos_t::FromInt(CELL_SIZE)/16); // ensure it's far enough to not intersect the building itself
				goal.hw = obstruction.hw + delta;
				goal.hh = obstruction.hh + delta;
			}
		}
	}
	else
	{
		// The target didn't have an obstruction or obstruction shape, so treat it as a point instead

		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (cmpTargetPosition.null() || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector3D targetPos = cmpTargetPosition->GetPosition();

		entity_pos_t distance = (pos - CFixedVector2D(targetPos.X, targetPos.Z)).Length();

		entity_pos_t goalDistance;
		if (distance < minRange)
		{
			goalDistance = minRange + goalDelta;
		}
		else if (distance > maxRange)
		{
			goalDistance = maxRange - goalDelta;
		}
		else
		{
			// We're already in range - no need to move anywhere
			FaceTowardsPoint(pos, goal.x, goal.z);
			return false;
		}

		// TODO: what happens if goalDistance < 0? (i.e. we probably can never get close enough to the target)

		goal.type = ICmpPathfinder::Goal::CIRCLE;
		goal.x = targetPos.X;
		goal.z = targetPos.Z;
		goal.hw = m_Radius + goalDistance;
	}

	m_FinalGoal = goal;
	if (!RegeneratePath(pos, false))
		return false;

	SwitchState(WALKING);
	return true;
}

bool CCmpUnitMotion::IsInAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	// This function closely mirrors MoveToAttackRange - it needs to return true
	// after that Move has completed

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector3D pos3 = cmpPosition->GetPosition();
	CFixedVector2D pos (pos3.X, pos3.Z);

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	ICmpObstructionManager::tag_t tag = 0;

	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), target);
	if (!cmpObstruction.null())
		tag = cmpObstruction->GetObstruction();

	entity_pos_t distance;

	if (tag)
	{
		ICmpObstructionManager::ObstructionSquare obstruction = cmpObstructionManager->GetObstruction(tag);

		CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize);

		// See if we're too close to the target square
		if (distance < minRange)
			return false;

		// See if we're close enough to the target square
		if (distance <= maxRange)
			return true;

		entity_pos_t circleRadius = halfSize.Length();

		if (ShouldTreatTargetAsCircle(maxRange, obstruction.hw, obstruction.hh, circleRadius))
		{
			// The target is small relative to our range, so pretend it's a circle
			// and see if we're close enough to that

			entity_pos_t circleDistance = (pos - CFixedVector2D(obstruction.x, obstruction.z)).Length() - circleRadius;

			if (circleDistance <= maxRange)
				return true;
		}

		return false;
	}
	else
	{
		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (cmpTargetPosition.null() || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector3D targetPos = cmpTargetPosition->GetPosition();

		entity_pos_t distance = (pos - CFixedVector2D(targetPos.X, targetPos.Z)).Length();

		if (minRange <= distance && distance <= maxRange)
			return true;

		return false;
	}
}

bool CCmpUnitMotion::RegeneratePath(CFixedVector2D pos, bool avoidMovingUnits)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder (GetSimContext(), SYSTEM_ENTITY);
	if (cmpPathfinder.null())
		return false;

	m_Path.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.clear();

	// TODO: if it's close then just do a short path, not a long path
	cmpPathfinder->SetDebugPath(pos.X, pos.Y, m_FinalGoal);
	cmpPathfinder->ComputePath(pos.X, pos.Y, m_FinalGoal, m_Path);

	if (m_DebugOverlayEnabled)
		RenderPath(m_Path, m_DebugOverlayLines, OVERLAY_COLOUR_PATH);

	// If there's no waypoints then we've stopped already, otherwise move to the first one
	if (m_Path.m_Waypoints.empty())
	{
		m_HasTarget = false;
		return false;
	}
	else
	{
		return PickNextShortWaypoint(pos, avoidMovingUnits);
	}
}

void CCmpUnitMotion::MaybePickNextWaypoint(const CFixedVector2D& pos)
{
	if (m_Path.m_Waypoints.empty())
		return;

	CFixedVector2D w(m_Path.m_Waypoints.back().x, m_Path.m_Waypoints.back().z);
	if ((w - pos).Length() < WAYPOINT_ADVANCE_MIN)
		PickNextWaypoint(pos, false); // TODO: handle failures?
}

bool CCmpUnitMotion::PickNextWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits)
{
	if (m_Path.m_Waypoints.empty())
		return false;

	// First try to get the immediate next waypoint
	entity_pos_t targetX = m_Path.m_Waypoints.back().x;
	entity_pos_t targetZ = m_Path.m_Waypoints.back().z;
	m_Path.m_Waypoints.pop_back();

	// To smooth the motion and avoid grid-constrained movement and allow dynamic obstacle avoidance,
	// try skipping some more waypoints if they're close enough

	while (!m_Path.m_Waypoints.empty())
	{
		CFixedVector2D w(m_Path.m_Waypoints.back().x, m_Path.m_Waypoints.back().z);
		if ((w - pos).Length() > WAYPOINT_ADVANCE_MAX)
			break;
		targetX = m_Path.m_Waypoints.back().x;
		targetZ = m_Path.m_Waypoints.back().z;
		m_Path.m_Waypoints.pop_back();
	}

	// Highlight the targeted waypoint
	if (m_DebugOverlayEnabled)
		m_DebugOverlayLines[m_Path.m_Waypoints.size()].m_Color = OVERLAY_COLOUR_PATH_ACTIVE;

	// Now we need to recompute a short path to the waypoint
	m_ShortPath.m_Waypoints.clear();

	ICmpPathfinder::Goal goal;
	if (m_Path.m_Waypoints.empty())
	{
		// This was the last waypoint - head for the exact goal
		goal = m_FinalGoal;
	}
	else
	{
		// Head for somewhere near the waypoint (but allow some leeway in case it's obstructed)
		goal.type = ICmpPathfinder::Goal::CIRCLE;
		goal.hw = entity_pos_t::FromInt(CELL_SIZE*3/2);
		goal.x = targetX;
		goal.z = targetZ;
	}

	CmpPtr<ICmpPathfinder> cmpPathfinder (GetSimContext(), SYSTEM_ENTITY);
	if (cmpPathfinder.null())
		return false;

	// Set up the filter to avoid/ignore moving units
	NullObstructionFilter filterNull;
	StationaryObstructionFilter filterStationary;
	const IObstructionTestFilter* filter;
	if (avoidMovingUnits)
		filter = &filterNull;
	else
		filter = &filterStationary;

	cmpPathfinder->ComputeShortPath(*filter, pos.X, pos.Y, m_Radius, SHORT_PATH_SEARCH_RANGE, goal, m_ShortPath);

	if (m_DebugOverlayEnabled)
		RenderPath(m_ShortPath, m_DebugOverlayShortPathLines, OVERLAY_COLOUR_SHORT_PATH);

	return true;
}

bool CCmpUnitMotion::PickNextShortWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits)
{
	// If we don't have a short path now
	if (m_ShortPath.m_Waypoints.empty())
	{
		// Try to pick a new long waypoint (which will also recompute the short path)
		if (!PickNextWaypoint(pos, avoidMovingUnits))
			return false; // no waypoints left

		if (m_ShortPath.m_Waypoints.empty())
			return false; // we can't reach the next long waypoint or are already there
	}

	// Head towards the next short waypoint
	m_ShortTargetX = m_ShortPath.m_Waypoints.back().x;
	m_ShortTargetZ = m_ShortPath.m_Waypoints.back().z;
	m_ShortPath.m_Waypoints.pop_back();
	m_HasTarget = true;
	return true;
}

void CCmpUnitMotion::RenderPath(const ICmpPathfinder::Path& path, std::vector<SOverlayLine>& lines, CColor color)
{
	lines.clear();
	std::vector<float> waypointCoords;
	for (size_t i = 0; i < path.m_Waypoints.size(); ++i)
	{
		float x = path.m_Waypoints[i].x.ToFloat();
		float z = path.m_Waypoints[i].z.ToFloat();
		waypointCoords.push_back(x);
		waypointCoords.push_back(z);
		lines.push_back(SOverlayLine());
		lines.back().m_Color = color;
		SimRender::ConstructSquareOnGround(GetSimContext(), x, z, 1.0f, 1.0f, 0.0f, lines.back());
	}
	lines.push_back(SOverlayLine());
	lines.back().m_Color = color;
	SimRender::ConstructLineOnGround(GetSimContext(), waypointCoords, lines.back());

}

void CCmpUnitMotion::RenderSubmit(SceneCollector& collector)
{
	if (!m_DebugOverlayEnabled)
		return;

	for (size_t i = 0; i < m_DebugOverlayLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLines[i]);

	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}
