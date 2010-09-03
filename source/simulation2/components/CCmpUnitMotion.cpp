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
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

static const entity_pos_t WAYPOINT_ADVANCE_MIN = entity_pos_t::FromInt(CELL_SIZE*4);
static const entity_pos_t WAYPOINT_ADVANCE_MAX = entity_pos_t::FromInt(CELL_SIZE*8);
static const entity_pos_t SHORT_PATH_SEARCH_RANGE = entity_pos_t::FromInt(CELL_SIZE*10);
static const entity_pos_t SHORT_PATH_GOAL_RADIUS = entity_pos_t::FromInt(CELL_SIZE*3/2);

static const CColor OVERLAY_COLOUR_PATH(1, 1, 1, 1);
static const CColor OVERLAY_COLOUR_PATH_ACTIVE(1, 1, 0, 1);
static const CColor OVERLAY_COLOUR_SHORT_PATH(1, 0, 0, 1);
static const CColor OVERLAY_COLOUR_DIRECT_PATH(1, 1, 0, 1);

class CCmpUnitMotion : public ICmpUnitMotion
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update_MotionFormation);
		componentManager.SubscribeToMessageType(MT_Update_MotionUnit);
		componentManager.SubscribeToMessageType(MT_RenderSubmit); // for debug overlays
		componentManager.SubscribeToMessageType(MT_PathResult);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotion)

	bool m_DebugOverlayEnabled;
	std::vector<SOverlayLine> m_DebugOverlayLines;
	std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	// Template state:

	bool m_FormationController;
	fixed m_WalkSpeed; // in metres per second
	fixed m_RunSpeed;
	u8 m_PassClass;
	u8 m_CostClass;

	// Dynamic state:

	entity_pos_t m_Radius;

	enum
	{
		STATE_IDLE, // not moving at all
		STATE_STOPPING, // will go IDLE next turn (this delay fixes animation timings)
		// Units that are members of a formation:
		// ('target' is m_TargetEntity plus m_TargetOffset)
		STATE_FORMATIONMEMBER_DIRECT, // trying to follow a straight line to target
		STATE_FORMATIONMEMBER_PATH_COMPUTING, // waiting for a path to target
		STATE_FORMATIONMEMBER_PATH_FOLLOWING, // following the computed path to target
		// Entities that represent individual units or formation controllers:
		// ('target' is m_FinalGoal)
		STATE_INDIVIDUAL_PATH_COMPUTING, // waiting for a path to target
		STATE_INDIVIDUAL_PATH_FOLLOWING // following the computed path to target
	};
	int m_State;

	u32 m_ExpectedPathTicket; // asynchronous request ID we're waiting for, or 0 if none

	entity_id_t m_TargetEntity;
	entity_pos_t m_TargetOffsetX, m_TargetOffsetZ;

	fixed m_Speed;

	// These values contain undefined junk if !HasTarget:
	ICmpPathfinder::Path m_LongPath;
	ICmpPathfinder::Path m_ShortPath;
	entity_pos_t m_ShortTargetX, m_ShortTargetZ;
	ICmpPathfinder::Goal m_FinalGoal;

	static std::string GetSchema()
	{
		return
			"<a:help>Provides the unit with the ability to move around the world by itself.</a:help>"
			"<a:example>"
				"<WalkSpeed>7.0</WalkSpeed>"
				"<PassabilityClass>default</PassabilityClass>"
				"<CostClass>infantry</CostClass>"
			"</a:example>"
			"<element name='FormationController'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='WalkSpeed' a:help='Basic movement speed (in metres per second)'>"
				"<ref name='positiveDecimal'/>"
			"</element>"
			"<optional>"
				"<element name='Run'>"
					"<interleave>"
						"<element name='Speed'><ref name='positiveDecimal'/></element>"
						"<element name='Range'><ref name='positiveDecimal'/></element>"
						"<element name='RangeMin'><ref name='nonNegativeDecimal'/></element>"
						"<element name='RegenTime'><ref name='positiveDecimal'/></element>"
						"<element name='DecayTime'><ref name='positiveDecimal'/></element>"
					"</interleave>"
				"</element>"
			"</optional>"
			"<element name='PassabilityClass' a:help='Identifies the terrain passability class (values are defined in special/pathfinder.xml)'>"
				"<text/>"
			"</element>"
			"<element name='CostClass' a:help='Identifies the movement speed/cost class (values are defined in special/pathfinder.xml)'>"
				"<text/>"
			"</element>";
	}

	/*
	 * TODO: the running/charging thing needs to be designed and implemented
	 */

	virtual void Init(const CSimContext& context, const CParamNode& paramNode)
	{
		m_FormationController = paramNode.GetChild("FormationController").ToBool();

		m_WalkSpeed = paramNode.GetChild("WalkSpeed").ToFixed();
		m_Speed = m_WalkSpeed;

		if (paramNode.GetChild("Run").IsOk())
		{
			m_RunSpeed = paramNode.GetChild("Run").GetChild("Speed").ToFixed();
		}
		else
		{
			m_RunSpeed = m_WalkSpeed;
		}

		CmpPtr<ICmpObstruction> cmpObstruction(context, GetEntityId());
		if (!cmpObstruction.null())
			m_Radius = cmpObstruction->GetUnitRadius();

		CmpPtr<ICmpPathfinder> cmpPathfinder(context, SYSTEM_ENTITY);
		if (!cmpPathfinder.null())
		{
			m_PassClass = cmpPathfinder->GetPassabilityClass(paramNode.GetChild("PassabilityClass").ToASCIIString());
			m_CostClass = cmpPathfinder->GetCostClass(paramNode.GetChild("CostClass").ToASCIIString());
		}

		m_State = STATE_IDLE;

		m_ExpectedPathTicket = 0;

		m_DebugOverlayEnabled = false;
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
//		serialize.Bool("has target", m_HasTarget);
//		if (m_HasTarget)
		{
			// TODO: m_Path
			// TODO: m_FinalTargetAngle
		}

		// TODO: m_State
	}

	virtual void Deserialize(const CSimContext& context, const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(context, paramNode);

//		deserialize.Bool(m_HasTarget);
//		if (m_HasTarget)
		{
		}
	}

	virtual void HandleMessage(const CSimContext& context, const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Update_MotionFormation:
		{
			if (m_FormationController)
			{
				fixed dt = static_cast<const CMessageUpdate_MotionFormation&> (msg).turnLength;
				Move(dt);
			}
			break;
		}
		case MT_Update_MotionUnit:
		{
			if (!m_FormationController)
			{
				fixed dt = static_cast<const CMessageUpdate_MotionUnit&> (msg).turnLength;
				Move(dt);
			}
			break;
		}
		case MT_RenderSubmit:
		{
			const CMessageRenderSubmit& msgData = static_cast<const CMessageRenderSubmit&> (msg);
			RenderSubmit(msgData.collector);
			break;
		}
		case MT_PathResult:
		{
			const CMessagePathResult& msgData = static_cast<const CMessagePathResult&> (msg);
			PathResult(msgData.ticket, msgData.path);
			break;
		}
		}
	}

	virtual fixed GetWalkSpeed()
	{
		return m_WalkSpeed;
	}

	virtual fixed GetRunSpeed()
	{
		return m_RunSpeed;
	}

	virtual void SetSpeed(fixed speed)
	{
		m_Speed = speed;
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlayEnabled = enabled;
		if (enabled)
		{
			RenderPath(m_LongPath, m_DebugOverlayLines, OVERLAY_COLOUR_PATH);
			RenderPath(m_ShortPath, m_DebugOverlayShortPathLines, OVERLAY_COLOUR_SHORT_PATH);
		}
	}

	virtual bool MoveToPoint(entity_pos_t x, entity_pos_t z);
	virtual bool MoveToAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool IsInAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange);
	virtual void MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z);

	virtual void StopMoving()
	{
		m_State = STATE_STOPPING;
	}

	virtual void SetUnitRadius(fixed radius)
	{
		m_Radius = radius;
	}

private:
	bool ShouldAvoidMovingUnits()
	{
		return !m_FormationController;
	}

	void SendMessageStartFailed()
	{
		CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), GetEntityId());
		if (!cmpObstruction.null())
			cmpObstruction->SetMovingFlag(false);

		CMessageMotionChanged msg(true, true);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	void SendMessageStartSucceeded()
	{
		CMessageMotionChanged msg(true, false);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	void SendMessageEndSucceeded()
	{
		CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), GetEntityId());
		if (!cmpObstruction.null())
			cmpObstruction->SetMovingFlag(false);

		CMessageMotionChanged msg(false, false);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	/**
	 * Do the per-turn movement and other updates
	 */
	void Move(fixed dt);

	void StopAndFaceGoal(CFixedVector2D pos);

	/**
	 * Rotate to face towards the target point, given the current pos
	 */
	void FaceTowardsPoint(CFixedVector2D pos, entity_pos_t x, entity_pos_t z);

	bool ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t hw, entity_pos_t hh, entity_pos_t circleRadius);

	/**
	 * Recompute the whole path to the current goal.
	 * Returns false on error or if the unit can't move anywhere at all.
	 */
	void RegeneratePath(CFixedVector2D pos);

	void RegenerateShortPath(CFixedVector2D pos, const ICmpPathfinder::Goal& goal, bool avoidMovingUnits);

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

	/**
	 * Handle the result of an asynchronous path query
	 */
	void PathResult(u32 ticket, const ICmpPathfinder::Path& path);
};

REGISTER_COMPONENT_TYPE(UnitMotion)

void CCmpUnitMotion::PathResult(u32 ticket, const ICmpPathfinder::Path& path)
{
	// Ignore obsolete path requests
	if (ticket != m_ExpectedPathTicket)
		return;

	m_ExpectedPathTicket = 0; // we don't expect to get this result again

	if (m_State == STATE_FORMATIONMEMBER_PATH_COMPUTING)
	{
		m_LongPath.m_Waypoints.clear();
		m_ShortPath = path;

		if (m_DebugOverlayEnabled)
			RenderPath(m_ShortPath, m_DebugOverlayShortPathLines, OVERLAY_COLOUR_SHORT_PATH);

		if (m_ShortPath.m_Waypoints.empty())
		{
			// Can't find any path - switch back to DIRECT so we try again later
			// when the formation's moved further on.
			// TODO: We should probably wait a while before trying again
			m_State = STATE_FORMATIONMEMBER_DIRECT;
			return;
		}

		// Now we're moving along the path

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null() || !cmpPosition->IsInWorld())
			return;

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		m_State = STATE_FORMATIONMEMBER_PATH_FOLLOWING;
		SendMessageStartSucceeded();

		PickNextShortWaypoint(pos, ShouldAvoidMovingUnits());

		return;
	}
	else if (m_State == STATE_INDIVIDUAL_PATH_COMPUTING)
	{
		m_LongPath = path;
		m_ShortPath.m_Waypoints.clear();

		if (m_DebugOverlayEnabled)
			RenderPath(m_LongPath, m_DebugOverlayLines, OVERLAY_COLOUR_PATH);

		// If there's no waypoints then we've stopped already, otherwise move to the first one
		if (m_LongPath.m_Waypoints.empty())
		{
			m_State = STATE_IDLE;
			SendMessageStartFailed();
		}
		else
		{
			CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
			if (cmpPosition.null() || !cmpPosition->IsInWorld())
				return;

			CFixedVector2D pos = cmpPosition->GetPosition2D();

			if (PickNextShortWaypoint(pos, ShouldAvoidMovingUnits()))
			{
				m_State = STATE_INDIVIDUAL_PATH_FOLLOWING;
				SendMessageStartSucceeded();
			}
			else
			{
				m_State = STATE_IDLE;
				SendMessageStartFailed();
			}
		}
	}
	else
	{
		LOGWARNING(L"unexpected PathResult (%d %d)", GetEntityId(), m_State);
	}
}

void CCmpUnitMotion::Move(fixed dt)
{
	PROFILE("Move");

	switch (m_State)
	{
	case STATE_STOPPING:
		m_State = STATE_IDLE;
		SendMessageEndSucceeded();
		return;

	case STATE_IDLE:
	case STATE_FORMATIONMEMBER_PATH_COMPUTING:
	case STATE_INDIVIDUAL_PATH_COMPUTING:
		return;

	case STATE_FORMATIONMEMBER_DIRECT:
	case STATE_FORMATIONMEMBER_PATH_FOLLOWING:
	{
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
		if (cmpPathfinder.null())
			return;

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null() || !cmpPosition->IsInWorld())
			return;

		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), m_TargetEntity);
		if (cmpTargetPosition.null() || !cmpTargetPosition->IsInWorld())
		{
			// If the target has disappeared, give up on it
			StopMoving();
			return;
		}

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		// We want to move (at most) m_Speed*dt units from pos towards the next waypoint

		entity_angle_t targetAngle = cmpTargetPosition->GetRotation().Y;
		CFixedVector2D targetOffset = CFixedVector2D(m_TargetOffsetX, m_TargetOffsetZ).Rotate(targetAngle);

		while (dt > fixed::Zero())
		{
			CFixedVector2D target;
			if (m_State == STATE_FORMATIONMEMBER_DIRECT)
			{
				target = cmpTargetPosition->GetPosition2D() + targetOffset;

				if (m_DebugOverlayEnabled)
				{
					// Draw a path from our current location to the target
					ICmpPathfinder::Path dummyPath;
					ICmpPathfinder::Waypoint dummyWaypoint;
					dummyWaypoint.x = pos.X;
					dummyWaypoint.z = pos.Y;
					dummyPath.m_Waypoints.push_back(dummyWaypoint);
					dummyWaypoint.x = target.X;
					dummyWaypoint.z = target.Y;
					dummyPath.m_Waypoints.push_back(dummyWaypoint);
					RenderPath(dummyPath, m_DebugOverlayShortPathLines, OVERLAY_COLOUR_DIRECT_PATH);
				}
			}
			else // m_State == STATE_FORMATIONMEMBER_PATH_FOLLOWING
			{
				target = CFixedVector2D(m_ShortTargetX, m_ShortTargetZ);
			}

			CFixedVector2D offset = target - pos;

			// Face towards the target
			if (!offset.IsZero())
			{
				entity_angle_t angle = atan2_approx(offset.X, offset.Y);
				cmpPosition->TurnTo(angle);
			}

			// Find the speed factor of the underlying terrain
			// (We only care about the tile we start on - it doesn't matter if we're moving
			// partially onto a much slower/faster tile)
			fixed terrainSpeed = cmpPathfinder->GetMovementSpeed(pos.X, pos.Y, m_CostClass);

			fixed maxSpeed = GetRunSpeed().Multiply(terrainSpeed); // TODO: fall back to walk speed if tired

			// Work out how far we can travel in dt
			fixed maxdist = maxSpeed.Multiply(dt);

			// If the target is close, we can move there directly
			fixed offsetLength = offset.Length();
			if (offsetLength <= maxdist)
			{
				ControlGroupObstructionFilter filter(true, m_TargetEntity);
				if (cmpPathfinder->CheckMovement(filter, pos.X, pos.Y, target.X, target.Y, m_Radius, m_PassClass))
				{
					// We can move directly with no obstructions

					pos = target;
					cmpPosition->MoveTo(pos.X, pos.Y);

					if (m_State == STATE_FORMATIONMEMBER_DIRECT)
					{
						// We've caught up with the target. Match the target direction
						// (but stay in TARGET_ENTITY_DIRECT mode since
						// the target might move by the next turn and we should continue
						// following it)
						cmpPosition->TurnTo(targetAngle);
						return;
					}
					else // m_State == STATE_FORMATIONMEMBER_PATH_FOLLOWING
					{
						// We were heading towards a waypoint on an obstruction-avoiding path.
						// Spend the rest of the time heading towards the next waypoint
						dt = dt - (offsetLength / maxSpeed);
						if (PickNextShortWaypoint(pos, false))
						{
							// Got a new waypoint; use it in the next iteration
							continue;
						}

						// We reached the end of the waypoints, so try going direct again
						m_State = STATE_FORMATIONMEMBER_DIRECT;
						continue;
					}
				}
			}
			else
			{
				// Not close enough, so just move in the right direction
				offset.Normalize(maxdist);
				target = pos + offset;

				ControlGroupObstructionFilter filter(true, m_TargetEntity);
				if (cmpPathfinder->CheckMovement(filter, pos.X, pos.Y, target.X, target.Y, m_Radius, m_PassClass))
				{
					pos = target;
					cmpPosition->MoveTo(pos.X, pos.Y);

					return;
				}
			}

			// Our movement was blocked before we reached the target/waypoint.
			// Compute a new short path to the target, which we'll use on the next turn.

			// TODO: if the target is a long way away, we should compute a long path
			// or leave the formation or something.

			m_State = STATE_FORMATIONMEMBER_PATH_COMPUTING;

			ICmpPathfinder::Goal goal;
			goal.type = ICmpPathfinder::Goal::POINT;
			target = cmpTargetPosition->GetPosition2D() + targetOffset;
			goal.x = target.X;
			goal.z = target.Y;

			RegenerateShortPath(pos, goal, true);

			return;
		}

		return;
	}

	case STATE_INDIVIDUAL_PATH_FOLLOWING:
	{
		CmpPtr<ICmpPathfinder> cmpPathfinder (GetSimContext(), SYSTEM_ENTITY);
		if (cmpPathfinder.null())
			return;

		CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
		if (cmpPosition.null())
			return;

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		// We want to move (at most) m_Speed*dt units from pos towards the next waypoint

		while (dt > fixed::Zero())
		{
			CFixedVector2D target(m_ShortTargetX, m_ShortTargetZ);
			CFixedVector2D offset = target - pos;

			// Face towards the target
			entity_angle_t angle = atan2_approx(offset.X, offset.Y);
			cmpPosition->TurnTo(angle);

			// Find the speed factor of the underlying terrain
			// (We only care about the tile we start on - it doesn't matter if we're moving
			// partially onto a much slower/faster tile)
			fixed terrainSpeed = cmpPathfinder->GetMovementSpeed(pos.X, pos.Y, m_CostClass);

			fixed maxSpeed = m_Speed.Multiply(terrainSpeed); // TODO: running?

			// Work out how far we can travel in dt
			fixed maxdist = maxSpeed.Multiply(dt);

			// If the target is close, we can move there directly
			fixed offsetLength = offset.Length();
			if (offsetLength <= maxdist)
			{
				ControlGroupObstructionFilter filter(ShouldAvoidMovingUnits(), GetEntityId());
				if (cmpPathfinder->CheckMovement(filter, pos.X, pos.Y, target.X, target.Y, m_Radius, m_PassClass))
				{
					pos = target;
					cmpPosition->MoveTo(pos.X, pos.Y);

					// Spend the rest of the time heading towards the next waypoint
					dt = dt - (offsetLength / maxSpeed);
					MaybePickNextWaypoint(pos);
					if (PickNextShortWaypoint(pos, ShouldAvoidMovingUnits()))
						continue;

					// We ran out of usable waypoints, so stop now
					StopAndFaceGoal(pos);

					return;
				}
			}
			else
			{
				// Not close enough, so just move in the right direction
				offset.Normalize(maxdist);
				target = pos + offset;

				ControlGroupObstructionFilter filter(ShouldAvoidMovingUnits(), GetEntityId());
				if (cmpPathfinder->CheckMovement(filter, pos.X, pos.Y, target.X, target.Y, m_Radius, m_PassClass))
				{
					pos = target;
					cmpPosition->MoveTo(pos.X, pos.Y);

					MaybePickNextWaypoint(pos);

					return;
				}
			}

			// We got blocked
			// Try finding a new path
			m_State = STATE_INDIVIDUAL_PATH_COMPUTING;
			RegeneratePath(pos);
			return;
		}
	}
	}
}

void CCmpUnitMotion::StopAndFaceGoal(CFixedVector2D pos)
{
	m_ExpectedPathTicket = 0;
	m_State = STATE_STOPPING;
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

bool CCmpUnitMotion::MoveToPoint(entity_pos_t x, entity_pos_t z)
{
	PROFILE("MoveToPoint");

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();
	ICmpPathfinder::Goal goal;

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	ControlGroupObstructionFilter filter(true, GetEntityId());

	ICmpObstructionManager::ObstructionSquare obstruction;
	if (cmpObstructionManager->FindMostImportantObstruction(filter, x, z, m_Radius, obstruction))
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

	m_State = STATE_INDIVIDUAL_PATH_COMPUTING;
	m_FinalGoal = goal;
	RegeneratePath(pos);
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

static const entity_pos_t g_GoalDelta = entity_pos_t::FromInt(CELL_SIZE)/4; // for extending the goal outwards/inwards a little bit

bool CCmpUnitMotion::MoveToAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	PROFILE("MoveToAttackRange");

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	ICmpObstructionManager::tag_t tag;

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

	if (tag.valid())
	{
		ICmpObstructionManager::ObstructionSquare obstruction = cmpObstructionManager->GetObstruction(tag);

		CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
		ICmpPathfinder::Goal goal;
		goal.x = obstruction.x;
		goal.z = obstruction.z;

		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize);

		if (distance < minRange)
		{
			// Too close to the square - need to move away

			entity_pos_t goalDistance = minRange + g_GoalDelta;

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

				entity_pos_t goalDistance = maxRange - g_GoalDelta;

				goal.type = ICmpPathfinder::Goal::CIRCLE;
				goal.hw = circleRadius + goalDistance;
			}
			else
			{
				// The target is large relative to our range, so treat it as a square and
				// get close enough that the diagonals come within range

				entity_pos_t goalDistance = (maxRange - g_GoalDelta)*2 / 3; // multiply by slightly less than 1/sqrt(2)

				goal.type = ICmpPathfinder::Goal::SQUARE;
				goal.u = obstruction.u;
				goal.v = obstruction.v;
				entity_pos_t delta = std::max(goalDistance, m_Radius + entity_pos_t::FromInt(CELL_SIZE)/16); // ensure it's far enough to not intersect the building itself
				goal.hw = obstruction.hw + delta;
				goal.hh = obstruction.hh + delta;
			}
		}

		m_State = STATE_INDIVIDUAL_PATH_COMPUTING;
		m_FinalGoal = goal;
		RegeneratePath(pos);
		return true;
	}
	else
	{
		// The target didn't have an obstruction or obstruction shape, so treat it as a point instead

		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (cmpTargetPosition.null() || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector2D targetPos = cmpTargetPosition->GetPosition2D();

		return MoveToPointRange(targetPos.X, targetPos.Y, minRange, maxRange);
	}
}

bool CCmpUnitMotion::MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange)
{
	PROFILE("MoveToPointRange");

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	entity_pos_t distance = (pos - CFixedVector2D(x, z)).Length();

	entity_pos_t goalDistance;
	if (distance < minRange)
	{
		goalDistance = minRange + g_GoalDelta;
	}
	else if (distance > maxRange)
	{
		goalDistance = maxRange - g_GoalDelta;
	}
	else
	{
		// We're already in range - no need to move anywhere
		FaceTowardsPoint(pos, x, z);
		return false;
	}

	// TODO: what happens if goalDistance < 0? (i.e. we probably can never get close enough to the target)

	ICmpPathfinder::Goal goal;
	goal.type = ICmpPathfinder::Goal::CIRCLE;
	goal.x = x;
	goal.z = z;
	goal.hw = m_Radius + goalDistance;

	m_State = STATE_INDIVIDUAL_PATH_COMPUTING;
	m_FinalGoal = goal;
	RegeneratePath(pos);
	return true;
}

bool CCmpUnitMotion::IsInAttackRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	// This function closely mirrors MoveToAttackRange - it needs to return true
	// after that Move has completed

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (cmpPosition.null() || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSimContext(), SYSTEM_ENTITY);
	if (cmpObstructionManager.null())
		return false;

	ICmpObstructionManager::tag_t tag;

	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), target);
	if (!cmpObstruction.null())
		tag = cmpObstruction->GetObstruction();

	entity_pos_t distance;

	if (tag.valid())
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

		CFixedVector2D targetPos = cmpTargetPosition->GetPosition2D();

		entity_pos_t distance = (pos - targetPos).Length();

		if (minRange <= distance && distance <= maxRange)
			return true;

		return false;
	}
}

void CCmpUnitMotion::MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z)
{
	m_ExpectedPathTicket = 0;
	m_State = STATE_FORMATIONMEMBER_DIRECT;

	m_TargetEntity = target;
	m_TargetOffsetX = x;
	m_TargetOffsetZ = z;

	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), GetEntityId());
	if (!cmpObstruction.null())
		cmpObstruction->SetMovingFlag(true);
}



void CCmpUnitMotion::RegeneratePath(CFixedVector2D pos)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
	if (cmpPathfinder.null())
		return;

	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), GetEntityId());
	if (!cmpObstruction.null())
		cmpObstruction->SetMovingFlag(true);

	// TODO: if it's close then just do a short path, not a long path
	cmpPathfinder->SetDebugPath(pos.X, pos.Y, m_FinalGoal, m_PassClass, m_CostClass);
	m_ExpectedPathTicket = cmpPathfinder->ComputePathAsync(pos.X, pos.Y, m_FinalGoal, m_PassClass, m_CostClass, GetEntityId());
}

void CCmpUnitMotion::RegenerateShortPath(CFixedVector2D pos, const ICmpPathfinder::Goal& goal, bool avoidMovingUnits)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
	if (cmpPathfinder.null())
		return;

	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), GetEntityId());
	if (!cmpObstruction.null())
		cmpObstruction->SetMovingFlag(true);

	m_ExpectedPathTicket = cmpPathfinder->ComputeShortPathAsync(pos.X, pos.Y, m_Radius, SHORT_PATH_SEARCH_RANGE, goal, m_PassClass, avoidMovingUnits, m_TargetEntity, GetEntityId());
}

void CCmpUnitMotion::MaybePickNextWaypoint(const CFixedVector2D& pos)
{
	if (m_LongPath.m_Waypoints.empty())
		return;

	CFixedVector2D w(m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z);
	if ((w - pos).Length() < WAYPOINT_ADVANCE_MIN)
		PickNextWaypoint(pos, false); // TODO: handle failures?
}

bool CCmpUnitMotion::PickNextWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits)
{
	if (m_LongPath.m_Waypoints.empty())
		return false;

	// First try to get the immediate next waypoint
	entity_pos_t targetX = m_LongPath.m_Waypoints.back().x;
	entity_pos_t targetZ = m_LongPath.m_Waypoints.back().z;
	m_LongPath.m_Waypoints.pop_back();

	// To smooth the motion and avoid grid-constrained movement and allow dynamic obstacle avoidance,
	// try skipping some more waypoints if they're close enough

	while (!m_LongPath.m_Waypoints.empty())
	{
		CFixedVector2D w(m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z);
		if ((w - pos).Length() > WAYPOINT_ADVANCE_MAX)
			break;
		targetX = m_LongPath.m_Waypoints.back().x;
		targetZ = m_LongPath.m_Waypoints.back().z;
		m_LongPath.m_Waypoints.pop_back();
	}

	// Highlight the targeted waypoint
	if (m_DebugOverlayEnabled)
		m_DebugOverlayLines[m_LongPath.m_Waypoints.size()].m_Color = OVERLAY_COLOUR_PATH_ACTIVE;

	// Now we need to recompute a short path to the waypoint
	m_ShortPath.m_Waypoints.clear();

	ICmpPathfinder::Goal goal;
	if (m_LongPath.m_Waypoints.empty())
	{
		// This was the last waypoint - head for the exact goal
		goal = m_FinalGoal;
	}
	else
	{
		// Head for somewhere near the waypoint (but allow some leeway in case it's obstructed)
		goal.type = ICmpPathfinder::Goal::CIRCLE;
		goal.hw = SHORT_PATH_GOAL_RADIUS;
		goal.x = targetX;
		goal.z = targetZ;
	}

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
	if (cmpPathfinder.null())
		return false;

	// Set up the filter to avoid/ignore moving units
	ControlGroupObstructionFilter filter(avoidMovingUnits, GetEntityId());

	// TODO: this should be async
	cmpPathfinder->ComputeShortPath(filter, pos.X, pos.Y, m_Radius, SHORT_PATH_SEARCH_RANGE, goal, m_PassClass, m_ShortPath);

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
	return true;
}

void CCmpUnitMotion::RenderPath(const ICmpPathfinder::Path& path, std::vector<SOverlayLine>& lines, CColor color)
{
	bool floating = false;
	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), GetEntityId());
	if (!cmpPosition.null())
		floating = cmpPosition->IsFloating();

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
		SimRender::ConstructSquareOnGround(GetSimContext(), x, z, 1.0f, 1.0f, 0.0f, lines.back(), floating);
	}
	lines.push_back(SOverlayLine());
	lines.back().m_Color = color;
	SimRender::ConstructLineOnGround(GetSimContext(), waypointCoords, lines.back(), floating);

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
