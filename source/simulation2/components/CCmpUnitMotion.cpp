/* Copyright (C) 2012 Wildfire Games.
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

#include "simulation2/components/ICmpObstruction.h"
#include "simulation2/components/ICmpObstructionManager.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpPathfinder.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpValueModificationManager.h"
#include "simulation2/helpers/Geometry.h"
#include "simulation2/helpers/Render.h"
#include "simulation2/MessageTypes.h"
#include "simulation2/serialization/SerializeTemplates.h"

#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "maths/FixedVector2D.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

/**
 * When advancing along the long path, and picking a new waypoint to move
 * towards, we'll pick one that's up to this far from the unit's current
 * position (to minimise the effects of grid-constrained movement)
 */
static const entity_pos_t WAYPOINT_ADVANCE_MAX = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*8);

/**
 * When advancing along the long path, we'll pick a new waypoint to move
 * towards if we expect to reach the end of our current short path within
 * this many turns (assuming constant speed and turn length).
 * (This could typically be 1, but we need some tolerance in case speeds
 * or turn lengths change.)
 */
static const int WAYPOINT_ADVANCE_LOOKAHEAD_TURNS = 4;

/**
 * Maximum range to restrict short path queries to. (Larger ranges are slower,
 * smaller ranges might miss some legitimate routes around large obstacles.)
 */
static const entity_pos_t SHORT_PATH_SEARCH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*10);

/**
 * When short-pathing to an intermediate waypoint, we aim for a circle of this radius
 * around the waypoint rather than expecting to reach precisely the waypoint itself
 * (since it might be inside an obstacle).
 */
static const entity_pos_t SHORT_PATH_GOAL_RADIUS = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*3/2);

/**
 * If we are this close to our target entity/point, then think about heading
 * for it in a straight line instead of pathfinding.
 */
static const entity_pos_t DIRECT_PATH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*4);

/**
 * If we're following a target entity,
 * we will recompute our path if the target has moved
 * more than this distance from where we last pathed to.
 */
static const entity_pos_t CHECK_TARGET_MOVEMENT_MIN_DELTA = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*4);

/**
 * If we're following as part of a formation,
 * but can't move to our assigned target point in a straight line,
 * we will recompute our path if the target has moved
 * more than this distance from where we last pathed to.
 */
static const entity_pos_t CHECK_TARGET_MOVEMENT_MIN_DELTA_FORMATION = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*1);

/**
 * If we're following something but it's more than this distance away along
 * our path, then don't bother trying to repath regardless of how much it has
 * moved, until we get this close to the end of our old path.
 */
static const entity_pos_t CHECK_TARGET_MOVEMENT_AT_MAX_DIST = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*16);

/**
 * If we're following something and the angle between the (straight-line) directions to its previous target
 * position and its present target position is greater than a given angle, recompute the path even far away
 * (i.e. even if CHECK_TARGET_MOVEMENT_AT_MAX_DIST condition is not fulfilled). The actual check is done
 * on the cosine of this angle, with a PI/6 angle.
 */
static const fixed CHECK_TARGET_MOVEMENT_MIN_COS = fixed::FromInt(866)/1000;

static const CColor OVERLAY_COLOUR_LONG_PATH(1, 1, 1, 1);
static const CColor OVERLAY_COLOUR_SHORT_PATH(1, 0, 0, 1);

static const entity_pos_t g_GoalDelta = entity_pos_t::FromInt(TERRAIN_TILE_SIZE)/4; // for extending the goal outwards/inwards a little bit

class CCmpUnitMotion : public ICmpUnitMotion
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update_MotionFormation);
		componentManager.SubscribeToMessageType(MT_Update_MotionUnit);
		componentManager.SubscribeToMessageType(MT_PathResult);
		componentManager.SubscribeToMessageType(MT_ValueModification);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotion)

	bool m_DebugOverlayEnabled;
	std::vector<SOverlayLine> m_DebugOverlayLongPathLines;
	std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	// Template state:

	bool m_FormationController;
	fixed m_WalkSpeed, m_OriginalWalkSpeed; // in metres per second
	fixed m_RunSpeed, m_OriginalRunSpeed;
	ICmpPathfinder::pass_class_t m_PassClass;
	std::string m_PassClassName;
	ICmpPathfinder::cost_class_t m_CostClass;

	// Dynamic state:

	entity_pos_t m_Radius;
	bool m_Moving;
	bool m_FacePointAfterMove;

	enum State
	{
		/*
		 * Not moving at all.
		 */
		STATE_IDLE,

		/*
		 * Not moving at all. Will go to IDLE next turn.
		 * (This one-turn delay is a hack to fix animation timings.)
		 */
		STATE_STOPPING,

		/*
		 * Member of a formation.
		 * Pathing to the target (depending on m_PathState).
		 * Target is m_TargetEntity plus m_TargetOffset.
		 */
		STATE_FORMATIONMEMBER_PATH,

		/*
		 * Individual unit or formation controller.
		 * Pathing to the target (depending on m_PathState).
		 * Target is m_TargetPos, m_TargetMinRange, m_TargetMaxRange;
		 * if m_TargetEntity is not INVALID_ENTITY then m_TargetPos is tracking it.
		 */
		STATE_INDIVIDUAL_PATH,

		STATE_MAX
	};
	u8 m_State;

	enum PathState
	{
		/*
		 * There is no path.
		 * (This should only happen in IDLE and STOPPING.)
		 */
		PATHSTATE_NONE,

		/*
		 * We have an outstanding long path request.
		 * No paths are usable yet, so we can't move anywhere.
		 */
		PATHSTATE_WAITING_REQUESTING_LONG,

		/*
		 * We have an outstanding short path request.
		 * m_LongPath is valid.
		 * m_ShortPath is not yet valid, so we can't move anywhere.
		 */
		PATHSTATE_WAITING_REQUESTING_SHORT,

		/*
		 * We are following our path, and have no path requests.
		 * m_LongPath and m_ShortPath are valid.
		 */
		PATHSTATE_FOLLOWING,

		/*
		 * We are following our path, and have an outstanding long path request.
		 * (This is because our target moved a long way and we need to recompute
		 * the whole path).
		 * m_LongPath and m_ShortPath are valid.
		 */
		PATHSTATE_FOLLOWING_REQUESTING_LONG,

		/*
		 * We are following our path, and have an outstanding short path request.
		 * (This is because our target moved and we've got a new long path
		 * which we need to follow).
		 * m_LongPath is valid; m_ShortPath is valid but obsolete.
		 */
		PATHSTATE_FOLLOWING_REQUESTING_SHORT,

		/*
		 * We are following our path, and have an outstanding short path request
		 * to append to our current path.
		 * (This is because we got near the end of our short path and need
		 * to extend it to continue along the long path).
		 * m_LongPath and m_ShortPath are valid.
		 */
		PATHSTATE_FOLLOWING_REQUESTING_SHORT_APPEND,

		PATHSTATE_MAX
	};
	u8 m_PathState;

	u32 m_ExpectedPathTicket; // asynchronous request ID we're waiting for, or 0 if none

	entity_id_t m_TargetEntity;
	CFixedVector2D m_TargetPos;
	CFixedVector2D m_TargetOffset;
	entity_pos_t m_TargetMinRange;
	entity_pos_t m_TargetMaxRange;

	fixed m_Speed;

	// Current mean speed (over the last turn).
	fixed m_CurSpeed;

	// Currently active paths (storing waypoints in reverse order).
	// The last item in each path is the point we're currently heading towards.
	ICmpPathfinder::Path m_LongPath;
	ICmpPathfinder::Path m_ShortPath;

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

	virtual void Init(const CParamNode& paramNode)
	{
		m_FormationController = paramNode.GetChild("FormationController").ToBool();

		m_Moving = false;
		m_FacePointAfterMove = true;

		m_WalkSpeed = m_OriginalWalkSpeed = paramNode.GetChild("WalkSpeed").ToFixed();
		m_Speed = m_WalkSpeed;
		m_CurSpeed = fixed::Zero();

		if (paramNode.GetChild("Run").IsOk())
			m_RunSpeed = m_OriginalRunSpeed = paramNode.GetChild("Run").GetChild("Speed").ToFixed();
		else
			m_RunSpeed = m_OriginalRunSpeed = m_WalkSpeed;

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
		{
			m_PassClassName = paramNode.GetChild("PassabilityClass").ToUTF8();
			m_PassClass = cmpPathfinder->GetPassabilityClass(m_PassClassName);
			m_CostClass = cmpPathfinder->GetCostClass(paramNode.GetChild("CostClass").ToUTF8());
		}

		CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
		if (cmpObstruction)
			m_Radius = cmpObstruction->GetUnitRadius();

		m_State = STATE_IDLE;
		m_PathState = PATHSTATE_NONE;

		m_ExpectedPathTicket = 0;

		m_TargetEntity = INVALID_ENTITY;

		m_FinalGoal.type = ICmpPathfinder::Goal::POINT;

		m_DebugOverlayEnabled = false;
	}

	virtual void Deinit()
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.NumberFixed_Unbounded("radius", m_Radius);

		serialize.NumberU8("state", m_State, 0, STATE_MAX-1);
		serialize.NumberU8("path state", m_PathState, 0, PATHSTATE_MAX-1);

		serialize.StringASCII("pass class", m_PassClassName, 0, 64);

		serialize.NumberU32_Unbounded("ticket", m_ExpectedPathTicket);

		serialize.NumberU32_Unbounded("target entity", m_TargetEntity);
		serialize.NumberFixed_Unbounded("target pos x", m_TargetPos.X);
		serialize.NumberFixed_Unbounded("target pos y", m_TargetPos.Y);
		serialize.NumberFixed_Unbounded("target offset x", m_TargetOffset.X);
		serialize.NumberFixed_Unbounded("target offset y", m_TargetOffset.Y);
		serialize.NumberFixed_Unbounded("target min range", m_TargetMinRange);
		serialize.NumberFixed_Unbounded("target max range", m_TargetMaxRange);

		serialize.NumberFixed_Unbounded("speed", m_Speed);

		serialize.Bool("moving", m_Moving);
		serialize.Bool("facePointAfterMove", m_FacePointAfterMove);

		SerializeVector<SerializeWaypoint>()(serialize, "long path", m_LongPath.m_Waypoints);
		SerializeVector<SerializeWaypoint>()(serialize, "short path", m_ShortPath.m_Waypoints);

		SerializeGoal()(serialize, "goal", m_FinalGoal);
	}

	virtual void Serialize(ISerializer& serialize)
	{
		SerializeCommon(serialize);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		SerializeCommon(deserialize);

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
			m_PassClass = cmpPathfinder->GetPassabilityClass(m_PassClassName);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
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
			PROFILE3("UnitMotion::RenderSubmit");
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
		case MT_ValueModification:
		{
			const CMessageValueModification& msgData = static_cast<const CMessageValueModification&> (msg); 
			if (msgData.component != L"UnitMotion")
				break;

			CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSimContext(), SYSTEM_ENTITY);

			fixed newWalkSpeed = cmpValueModificationManager->ApplyModifications(L"UnitMotion/WalkSpeed", m_OriginalWalkSpeed, GetEntityId());
			fixed newRunSpeed = cmpValueModificationManager->ApplyModifications(L"UnitMotion/Run/Speed", m_OriginalRunSpeed, GetEntityId());

			// update m_Speed (the actual speed) if set to one of the variables
			if (m_Speed == m_WalkSpeed)
				m_Speed = newWalkSpeed;
			else if (m_Speed == m_RunSpeed)
				m_Speed = newRunSpeed;

			m_WalkSpeed = newWalkSpeed;
			m_RunSpeed = newRunSpeed;
		}
		}
	}

	void UpdateMessageSubscriptions()
	{
		bool needRender = m_DebugOverlayEnabled;
		GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, needRender);
	}

	virtual bool IsMoving()
	{
		return m_Moving;
	}

	virtual fixed GetWalkSpeed()
	{
		return m_WalkSpeed;
	}

	virtual fixed GetRunSpeed()
	{
		return m_RunSpeed;
	}

	virtual ICmpPathfinder::pass_class_t GetPassabilityClass()
	{
		return m_PassClass;
	}

	virtual std::string GetPassabilityClassName()
	{
		return m_PassClassName;
	}

	virtual void SetPassabilityClassName(std::string passClassName)
	{
		m_PassClassName = passClassName;
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
			m_PassClass = cmpPathfinder->GetPassabilityClass(passClassName);
	}

	virtual fixed GetCurrentSpeed()
	{
		return m_CurSpeed;
	}

	virtual void SetSpeed(fixed speed)
	{
		m_Speed = speed;
	}

	virtual void SetFacePointAfterMove(bool facePointAfterMove)
	{
		m_FacePointAfterMove = facePointAfterMove;
	}

	virtual void SetDebugOverlay(bool enabled)
	{
		m_DebugOverlayEnabled = enabled;
		UpdateMessageSubscriptions();
	}

	virtual bool MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool IsInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool MoveToTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool IsInTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);
	virtual void MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z);

	virtual void FaceTowardsPoint(entity_pos_t x, entity_pos_t z);

	virtual void StopMoving()
	{
		m_Moving = false;
		m_ExpectedPathTicket = 0;
		m_State = STATE_STOPPING;
		m_PathState = PATHSTATE_NONE;
		m_LongPath.m_Waypoints.clear();
		m_ShortPath.m_Waypoints.clear();
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

	bool IsFormationMember()
	{
		return m_State == STATE_FORMATIONMEMBER_PATH;
	}

	void StartFailed()
	{
		StopMoving();
		m_State = STATE_IDLE; // don't go through the STOPPING state since we never even started

		CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(false);

		CMessageMotionChanged msg(true, true);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	void MoveFailed()
	{
		StopMoving();

		CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(false);

		CMessageMotionChanged msg(false, true);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	void StartSucceeded()
	{
		CMessageMotionChanged msg(true, false);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	void MoveSucceeded()
	{
		m_Moving = false;

		CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(false);

		// No longer moving, so speed is 0.
		m_CurSpeed = fixed::Zero();

		CMessageMotionChanged msg(false, false);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	bool MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange, entity_id_t target);

	/**
	 * Handle the result of an asynchronous path query.
	 */
	void PathResult(u32 ticket, const ICmpPathfinder::Path& path);

	/**
	 * Do the per-turn movement and other updates.
	 */
	void Move(fixed dt);

	/**
	 * Decide whether to approximate the given range from a square target as a circle,
	 * rather than as a square.
	 */
	bool ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t hw, entity_pos_t hh, entity_pos_t circleRadius);

	/**
	 * Computes the current location of our target entity (plus offset).
	 * Returns false if no target entity or no valid position.
	 */
	bool ComputeTargetPosition(CFixedVector2D& out);

	/**
	 * Attempts to replace the current path with a straight line to the target
	 * entity, if it's close enough and the route is not obstructed.
	 */
	bool TryGoingStraightToTargetEntity(CFixedVector2D from);

	/**
	 * Returns whether the target entity has moved more than minDelta since our
	 * last path computations, and we're close enough to it to care.
	 */
	bool CheckTargetMovement(CFixedVector2D from, entity_pos_t minDelta);

	/**
	 * Returns whether the length of the given path, plus the distance from
	 * 'from' to the first waypoints, it shorter than minDistance.
	 */
	bool PathIsShort(const ICmpPathfinder::Path& path, CFixedVector2D from, entity_pos_t minDistance);

	/**
	 * Rotate to face towards the target point, given the current pos
	 */
	void FaceTowardsPointFromPos(CFixedVector2D pos, entity_pos_t x, entity_pos_t z);

	/**
	 * Returns an appropriate obstruction filter for use with path requests.
	 */
	ControlGroupMovementObstructionFilter GetObstructionFilter(bool forceAvoidMovingUnits = false);

	/**
	 * Start moving to the given goal, from our current position 'from'.
	 * Might go in a straight line immediately, or might start an asynchronous
	 * path request.
	 */
	void BeginPathing(CFixedVector2D from, const ICmpPathfinder::Goal& goal);

	/**
	 * Start an asynchronous long path query.
	 */
	void RequestLongPath(CFixedVector2D from, const ICmpPathfinder::Goal& goal);

	/**
	 * Start an asynchronous short path query.
	 */
	void RequestShortPath(CFixedVector2D from, const ICmpPathfinder::Goal& goal, bool avoidMovingUnits);

	/**
	 * Select a next long waypoint, given the current unit position.
	 * Also recomputes the short path to use that waypoint.
	 * Returns false on error, or if there is no waypoint to pick.
	 */
	bool PickNextLongWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits);

	/**
	 * Convert a path into a renderable list of lines
	 */
	void RenderPath(const ICmpPathfinder::Path& path, std::vector<SOverlayLine>& lines, CColor color);

	void RenderSubmit(SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(UnitMotion)

void CCmpUnitMotion::PathResult(u32 ticket, const ICmpPathfinder::Path& path)
{
	// Ignore obsolete path requests
	if (ticket != m_ExpectedPathTicket)
		return;

	m_ExpectedPathTicket = 0; // we don't expect to get this result again

	if (m_PathState == PATHSTATE_WAITING_REQUESTING_LONG)
	{
		m_LongPath = path;
		m_ShortPath.m_Waypoints.clear();

		// If there's no waypoints then we couldn't get near the target.
		// Sort of hack: Just try going directly to the goal point instead
		// (via the short pathfinder), so if we're stuck and the user clicks
		// close enough to the unit then we can probably get unstuck
		if (m_LongPath.m_Waypoints.empty())
		{
			ICmpPathfinder::Waypoint wp = { m_FinalGoal.x, m_FinalGoal.z };
			m_LongPath.m_Waypoints.push_back(wp);
		}

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
		{
			StartFailed();
			return;
		}

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		if (!PickNextLongWaypoint(pos, ShouldAvoidMovingUnits()))
		{
			StartFailed();
			return;
		}

		// We started a short path request to the next long path waypoint
		m_PathState = PATHSTATE_WAITING_REQUESTING_SHORT;
	}
	else if (m_PathState == PATHSTATE_WAITING_REQUESTING_SHORT)
	{
		m_ShortPath = path;

		// If there's no waypoints then we couldn't get near the target
		if (m_ShortPath.m_Waypoints.empty())
		{
			if (!IsFormationMember())
			{
				StartFailed();
				return;
			}
			else
			{
				m_Moving = false;
				CMessageMotionChanged msg(true, true);
				GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
			}
		}

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
		{
			StartFailed();
			return;
		}

		// Now we've got a short path that we can follow
		m_PathState = PATHSTATE_FOLLOWING;

		StartSucceeded();
	}
	else if (m_PathState == PATHSTATE_FOLLOWING_REQUESTING_LONG)
	{
		m_LongPath = path;
		// Leave the old m_ShortPath - we'll carry on following it until the
		// new short path has been computed

		// If there's no waypoints then we couldn't get near the target.
		// Sort of hack: Just try going directly to the goal point instead
		// (via the short pathfinder), so if we're stuck and the user clicks
		// close enough to the unit then we can probably get unstuck
		if (m_LongPath.m_Waypoints.empty())
		{
			ICmpPathfinder::Waypoint wp = { m_FinalGoal.x, m_FinalGoal.z };
			m_LongPath.m_Waypoints.push_back(wp);
		}

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
		{
			StopMoving();
			return;
		}

		CFixedVector2D pos = cmpPosition->GetPosition2D();

		if (!PickNextLongWaypoint(pos, ShouldAvoidMovingUnits()))
		{
			StopMoving();
			return;
		}

		// We started a short path request to the next long path waypoint
		m_PathState = PATHSTATE_FOLLOWING_REQUESTING_SHORT;

		// (TODO: is this entirely safe? We might continue moving along our
		// old path while this request is active, so it'll be slightly incorrect
		// by the time the request has completed)
	}
	else if (m_PathState == PATHSTATE_FOLLOWING_REQUESTING_SHORT)
	{
		// Replace the current path with the new one
		m_ShortPath = path;

		// If there's no waypoints then we couldn't get near the target
		if (m_ShortPath.m_Waypoints.empty())
		{
			// We should stop moving (unless we're in a formation, in which
			// case we should continue following it)
			if (!IsFormationMember())
			{
				MoveFailed();
				return;
			}
			else
			{
				m_Moving = false;
				CMessageMotionChanged msg(false, true);
				GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
			}
		}

		m_PathState = PATHSTATE_FOLLOWING;
	}
	else if (m_PathState == PATHSTATE_FOLLOWING_REQUESTING_SHORT_APPEND)
	{
		// Append the new path onto our current one
		m_ShortPath.m_Waypoints.insert(m_ShortPath.m_Waypoints.begin(), path.m_Waypoints.begin(), path.m_Waypoints.end());

		// If there's no waypoints then we couldn't get near the target
		// from the last intermediate long-path waypoint. But we can still
		// continue using the remainder of our current short path. So just
		// discard the now-useless long path.
		if (path.m_Waypoints.empty())
			m_LongPath.m_Waypoints.clear();

		m_PathState = PATHSTATE_FOLLOWING;
	}
	else
	{
		LOGWARNING(L"unexpected PathResult (%u %d %d)", GetEntityId(), m_State, m_PathState);
	}
}

void CCmpUnitMotion::Move(fixed dt)
{
	PROFILE("Move");

	if (m_State == STATE_STOPPING)
	{
		m_State = STATE_IDLE;
		MoveSucceeded();
		return;
	}

	if (m_State == STATE_IDLE)
		return;

	switch (m_PathState)
	{
	case PATHSTATE_NONE:
	{
		// If we're not pathing, do nothing
		return;
	}

	case PATHSTATE_WAITING_REQUESTING_LONG:
	case PATHSTATE_WAITING_REQUESTING_SHORT:
	{
		// If we're waiting for a path and don't have one yet, do nothing
		return;
	}

	case PATHSTATE_FOLLOWING:
	case PATHSTATE_FOLLOWING_REQUESTING_SHORT:
	case PATHSTATE_FOLLOWING_REQUESTING_SHORT_APPEND:
	case PATHSTATE_FOLLOWING_REQUESTING_LONG:
	{
		// TODO: there's some asymmetry here when units look at other
		// units' positions - the result will depend on the order of execution.
		// Maybe we should split the updates into multiple phases to minimise
		// that problem.

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (!cmpPathfinder)
			return;

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return;

		CFixedVector2D initialPos = cmpPosition->GetPosition2D();

		// If we're chasing a potentially-moving unit and are currently close
		// enough to its current position, and we can head in a straight line
		// to it, then throw away our current path and go straight to it
		if (m_PathState == PATHSTATE_FOLLOWING)
			TryGoingStraightToTargetEntity(initialPos);

		// Keep track of the current unit's position during the update
		CFixedVector2D pos = initialPos;

		// If in formation, run to keep up; otherwise just walk
		// (TODO: support stamina, charging, etc)
		fixed basicSpeed;
		if (IsFormationMember())
			basicSpeed = GetRunSpeed();
		else
			basicSpeed = m_Speed; // (typically but not always WalkSpeed)

		// Find the speed factor of the underlying terrain
		// (We only care about the tile we start on - it doesn't matter if we're moving
		// partially onto a much slower/faster tile)
		fixed terrainSpeed = cmpPathfinder->GetMovementSpeed(pos.X, pos.Y, m_CostClass);

		fixed maxSpeed = basicSpeed.Multiply(terrainSpeed);

		bool wasObstructed = false;

		// We want to move (at most) maxSpeed*dt units from pos towards the next waypoint

		fixed timeLeft = dt;
		fixed zero = fixed::Zero();
		
		while (timeLeft > zero)
		{
			// If we ran out of short path, we have to stop
			if (m_ShortPath.m_Waypoints.empty())
				break;

			CFixedVector2D target(m_ShortPath.m_Waypoints.back().x, m_ShortPath.m_Waypoints.back().z);
			CFixedVector2D offset = target - pos;

			// Work out how far we can travel in timeLeft
			fixed maxdist = maxSpeed.Multiply(timeLeft);

			// If the target is close, we can move there directly
			fixed offsetLength = offset.Length();
			if (offsetLength <= maxdist)
			{
				if (cmpPathfinder->CheckMovement(GetObstructionFilter(), pos.X, pos.Y, target.X, target.Y, m_Radius, m_PassClass))
				{
					pos = target;

					// Spend the rest of the time heading towards the next waypoint
					timeLeft = timeLeft - (offsetLength / maxSpeed);

					m_ShortPath.m_Waypoints.pop_back();
					continue;
				}
				else
				{
					// Error - path was obstructed
					wasObstructed = true;
					break;
				}
			}
			else
			{
				// Not close enough, so just move in the right direction
				offset.Normalize(maxdist);
				target = pos + offset;

				if (cmpPathfinder->CheckMovement(GetObstructionFilter(), pos.X, pos.Y, target.X, target.Y, m_Radius, m_PassClass))
				{
					pos = target;
					break;
				}
				else
				{
					// Error - path was obstructed
					wasObstructed = true;
					break;
				}
			}
		}

		// Update the Position component after our movement (if we actually moved anywhere)
		if (pos != initialPos)
		{
			CFixedVector2D offset = pos - initialPos;
			
			// Face towards the target
			entity_angle_t angle = atan2_approx(offset.X, offset.Y);
			cmpPosition->MoveAndTurnTo(pos.X,pos.Y, angle);

			// Calculate the mean speed over this past turn.
			m_CurSpeed = cmpPosition->GetDistanceTravelled() / dt;
		}
		
		if (wasObstructed)
		{
			// Oops, we hit something (very likely another unit).
			// Stop, and recompute the whole path.
			// TODO: if the target has UnitMotion and is higher priority,
			// we should wait a little bit.
			
			m_CurSpeed = zero;
			RequestLongPath(pos, m_FinalGoal);
			m_PathState = PATHSTATE_WAITING_REQUESTING_LONG;

			return;
		}

		// We successfully moved along our path, until running out of
		// waypoints or time.

		if (m_PathState == PATHSTATE_FOLLOWING)
		{
			// If we're not currently computing any new paths:

			// If we are close to reaching the end of the short path
			// (or have reached it already), try to extend it

			entity_pos_t minDistance = basicSpeed.Multiply(dt) * WAYPOINT_ADVANCE_LOOKAHEAD_TURNS;
			if (PathIsShort(m_ShortPath, pos, minDistance))
			{
				// Start the path extension from the end of this short path
				// (or our current position if no short path)
				CFixedVector2D from = pos;
				if (!m_ShortPath.m_Waypoints.empty())
					from = CFixedVector2D(m_ShortPath.m_Waypoints[0].x, m_ShortPath.m_Waypoints[0].z);

				if (PickNextLongWaypoint(from, ShouldAvoidMovingUnits()))
				{
					m_PathState = PATHSTATE_FOLLOWING_REQUESTING_SHORT_APPEND;
				}
				else
				{
					// Failed (there were no long waypoints left).
					// If there's still some short path then continue following
					// it, else we've finished moving.
					if (m_ShortPath.m_Waypoints.empty())
					{
						if (IsFormationMember())
						{
							// We've reached our assigned position. If the controller
							// is idle, send a notification in case it should disband,
							// otherwise continue following the formation next turn.
							CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), m_TargetEntity);
							if (cmpUnitMotion && !cmpUnitMotion->IsMoving())
							{
								m_Moving = false;
								CMessageMotionChanged msg(false, false);
								GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
							}
						}
						else
						{
							// check if target was reached in case of a moving target
							CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), m_TargetEntity);
							if 
							(
								cmpUnitMotion && cmpUnitMotion->IsMoving() &&
								MoveToTargetRange(m_TargetEntity, m_TargetMinRange, m_TargetMaxRange)
							)
								return;

							// Not in formation, so just finish moving
							StopMoving();
							m_State = STATE_IDLE;
							MoveSucceeded();

							if (m_FacePointAfterMove)
								FaceTowardsPointFromPos(pos, m_FinalGoal.x, m_FinalGoal.z);
							// TODO: if the goal was a square building, we ought to point towards the
							// nearest point on the square, not towards its center
						}
					}
				}
			}
		}

		// If we have a target entity, and we're not miles away from the end of
		// our current path, and the target moved enough, then recompute our
		// whole path
		if (m_PathState == PATHSTATE_FOLLOWING)
		{
			if (IsFormationMember())
				CheckTargetMovement(pos, CHECK_TARGET_MOVEMENT_MIN_DELTA_FORMATION);
			else
				CheckTargetMovement(pos, CHECK_TARGET_MOVEMENT_MIN_DELTA);
		}
	}
	}
}

bool CCmpUnitMotion::ComputeTargetPosition(CFixedVector2D& out)
{
	if (m_TargetEntity == INVALID_ENTITY)
		return false;

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), m_TargetEntity);
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	if (m_TargetOffset.IsZero())
	{
		// No offset, just return the position directly
		out = cmpPosition->GetPosition2D();
	}
	else
	{
		// There is an offset, so compute it relative to orientation
		entity_angle_t angle = cmpPosition->GetRotation().Y;
		CFixedVector2D offset = m_TargetOffset.Rotate(angle);
		out = cmpPosition->GetPosition2D() + offset;
	}
	return true;
}

bool CCmpUnitMotion::TryGoingStraightToTargetEntity(CFixedVector2D from)
{
	CFixedVector2D targetPos;
	if (!ComputeTargetPosition(targetPos))
		return false;

	// Fail if the target is too far away
	if ((targetPos - from).CompareLength(DIRECT_PATH_RANGE) > 0)
		return false;

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return false;

	// Move the goal to match the target entity's new position
	ICmpPathfinder::Goal goal = m_FinalGoal;
	goal.x = targetPos.X;
	goal.z = targetPos.Y;
	// (we ignore changes to the target's rotation, since only buildings are
	// square and buildings don't move)

	// Find the point on the goal shape that we should head towards
	CFixedVector2D goalPos = cmpPathfinder->GetNearestPointOnGoal(from, goal);

	// Check if there's any collisions on that route
	if (!cmpPathfinder->CheckMovement(GetObstructionFilter(), from.X, from.Y, goalPos.X, goalPos.Y, m_Radius, m_PassClass))
		return false;

	// That route is okay, so update our path
	m_FinalGoal = goal;
	m_LongPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.clear();
	ICmpPathfinder::Waypoint wp = { goalPos.X, goalPos.Y };
	m_ShortPath.m_Waypoints.push_back(wp);

	return true;
}

bool CCmpUnitMotion::CheckTargetMovement(CFixedVector2D from, entity_pos_t minDelta)
{
	CFixedVector2D targetPos;
	if (!ComputeTargetPosition(targetPos))
		return false;

	// Fail unless the target has moved enough
	CFixedVector2D oldTargetPos(m_FinalGoal.x, m_FinalGoal.z);
	if ((targetPos - oldTargetPos).CompareLength(minDelta) < 0)
		return false;

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;
	CFixedVector2D pos = cmpPosition->GetPosition2D();
	CFixedVector2D oldDir = (oldTargetPos - pos);
	CFixedVector2D newDir = (targetPos - pos);
	oldDir.Normalize();
	newDir.Normalize();

	// Fail unless we're close enough to the target to care about its movement
	// and the angle between the (straight-line) directions of the previous and new target positions is small
	if (oldDir.Dot(newDir) > CHECK_TARGET_MOVEMENT_MIN_COS && !PathIsShort(m_LongPath, from, CHECK_TARGET_MOVEMENT_AT_MAX_DIST))
		return false;

	// Fail if the target is no longer visible to this entity's owner
	// (in which case we'll continue moving to its last known location,
	// unless it comes back into view before we reach that location)
	CmpPtr<ICmpOwnership> cmpOwnership(GetEntityHandle());
	if (cmpOwnership)
	{
		CmpPtr<ICmpRangeManager> cmpRangeManager(GetSystemEntity());
		if (cmpRangeManager)
		{
			if (cmpRangeManager->GetLosVisibility(m_TargetEntity, cmpOwnership->GetOwner()) == ICmpRangeManager::VIS_HIDDEN)
				return false;
		}
	}

	// The target moved and we need to update our current path;
	// change the goal here and expect our caller to start the path request
	m_FinalGoal.x = targetPos.X;
	m_FinalGoal.z = targetPos.Y;
	RequestLongPath(from, m_FinalGoal);
	m_PathState = PATHSTATE_FOLLOWING_REQUESTING_LONG;

	return true;
}

bool CCmpUnitMotion::PathIsShort(const ICmpPathfinder::Path& path, CFixedVector2D from, entity_pos_t minDistance)
{
	entity_pos_t distLeft = minDistance;

	for (ssize_t i = (ssize_t)path.m_Waypoints.size()-1; i >= 0; --i)
	{
		// Check if the next path segment is longer than the requested minimum
		CFixedVector2D waypoint(path.m_Waypoints[i].x, path.m_Waypoints[i].z);
		CFixedVector2D delta = waypoint - from;
		if (delta.CompareLength(distLeft) > 0)
			return false;

		// Still short enough - prepare to check the next segment
		distLeft -= delta.Length();
		from = waypoint;
	}

	// Reached the end of the path before exceeding minDistance
	return true;
}

void CCmpUnitMotion::FaceTowardsPoint(entity_pos_t x, entity_pos_t z)
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;

	CFixedVector2D pos = cmpPosition->GetPosition2D();
	FaceTowardsPointFromPos(pos, x, z);
}

void CCmpUnitMotion::FaceTowardsPointFromPos(CFixedVector2D pos, entity_pos_t x, entity_pos_t z)
{
	CFixedVector2D target(x, z);
	CFixedVector2D offset = target - pos;
	if (!offset.IsZero())
	{
		entity_angle_t angle = atan2_approx(offset.X, offset.Y);

		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition)
			return;
		cmpPosition->TurnTo(angle);
	}
}

ControlGroupMovementObstructionFilter CCmpUnitMotion::GetObstructionFilter(bool forceAvoidMovingUnits)
{
	entity_id_t group;
	if (IsFormationMember())
		group = m_TargetEntity;
	else
		group = GetEntityId();

	return ControlGroupMovementObstructionFilter(forceAvoidMovingUnits || ShouldAvoidMovingUnits(), group);
}



void CCmpUnitMotion::BeginPathing(CFixedVector2D from, const ICmpPathfinder::Goal& goal)
{
	// Cancel any pending path requests
	m_ExpectedPathTicket = 0;

	// Update the unit's movement status.
	m_Moving = true;

	// Set our 'moving' flag, so other units pathfinding now will ignore us
	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	if (cmpObstruction)
		cmpObstruction->SetMovingFlag(true);

	// If we're aiming at a target entity and it's close and we can reach
	// it in a straight line, then we'll just go along the straight line
	// instead of computing a path.
	if (TryGoingStraightToTargetEntity(from))
	{
		m_PathState = PATHSTATE_FOLLOWING;
		return;
	}

	// TODO: should go straight to non-entity points too

	// Otherwise we need to compute a path.

	// TODO: if it's close then just do a short path, not a long path
	// (But if it's close on the opposite side of a river then we really
	// need a long path, so we can't simply check linear distance)

	m_PathState = PATHSTATE_WAITING_REQUESTING_LONG;
	RequestLongPath(from, goal);
}

void CCmpUnitMotion::RequestLongPath(CFixedVector2D from, const ICmpPathfinder::Goal& goal)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	cmpPathfinder->SetDebugPath(from.X, from.Y, goal, m_PassClass, m_CostClass);

	m_ExpectedPathTicket = cmpPathfinder->ComputePathAsync(from.X, from.Y, goal, m_PassClass, m_CostClass, GetEntityId());
}

void CCmpUnitMotion::RequestShortPath(CFixedVector2D from, const ICmpPathfinder::Goal& goal, bool avoidMovingUnits)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	m_ExpectedPathTicket = cmpPathfinder->ComputeShortPathAsync(from.X, from.Y, m_Radius, SHORT_PATH_SEARCH_RANGE, goal, m_PassClass, avoidMovingUnits, m_TargetEntity, GetEntityId());
}

bool CCmpUnitMotion::PickNextLongWaypoint(const CFixedVector2D& pos, bool avoidMovingUnits)
{
	// If there's no long path, we can't pick the next waypoint from it
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
		if ((w - pos).CompareLength(WAYPOINT_ADVANCE_MAX) > 0)
			break;
		targetX = m_LongPath.m_Waypoints.back().x;
		targetZ = m_LongPath.m_Waypoints.back().z;
		m_LongPath.m_Waypoints.pop_back();
	}

	// Now we need to recompute a short path to the waypoint

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

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return false;

	m_ExpectedPathTicket = cmpPathfinder->ComputeShortPathAsync(pos.X, pos.Y, m_Radius, SHORT_PATH_SEARCH_RANGE, goal, m_PassClass, avoidMovingUnits, GetEntityId(), GetEntityId());

	return true;
}

bool CCmpUnitMotion::MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange)
{
	return MoveToPointRange(x, z, minRange, maxRange, INVALID_ENTITY);
}

bool CCmpUnitMotion::MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange, entity_id_t target)
{
	PROFILE("MoveToPointRange");

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	ICmpPathfinder::Goal goal;

	if (minRange.IsZero() && maxRange.IsZero())
	{
		// Handle the non-ranged mode:

		// Check whether this point is in an obstruction

		CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
		if (!cmpObstructionManager)
			return false;

		ICmpObstructionManager::ObstructionSquare obstruction;
		if (cmpObstructionManager->FindMostImportantObstruction(GetObstructionFilter(true), x, z, m_Radius, obstruction))
		{
			// If we're aiming inside a building, then aim for the outline of the building instead
			// TODO: if we're aiming at a unit then maybe a circle would look nicer?

			goal.type = ICmpPathfinder::Goal::SQUARE;
			goal.x = obstruction.x;
			goal.z = obstruction.z;
			goal.u = obstruction.u;
			goal.v = obstruction.v;
			goal.hw = obstruction.hw + m_Radius + g_GoalDelta; // nudge the goal outwards so it doesn't intersect the building itself
			goal.hh = obstruction.hh + m_Radius + g_GoalDelta;
		}
		else
		{
			// Unobstructed - head directly for the goal
			goal.type = ICmpPathfinder::Goal::POINT;
			goal.x = x;
			goal.z = z;
		}
	}
	else
	{
		entity_pos_t distance = (pos - CFixedVector2D(x, z)).Length();

		entity_pos_t goalDistance;
		if (distance < minRange)
		{
			goalDistance = minRange + g_GoalDelta;
		}
		else if (maxRange >= entity_pos_t::Zero() && distance > maxRange)
		{
			goalDistance = maxRange - g_GoalDelta;
		}
		else
		{
			// We're already in range - no need to move anywhere
			if (m_FacePointAfterMove)
				FaceTowardsPointFromPos(pos, x, z);
			return false;
		}

		// TODO: what happens if goalDistance < 0? (i.e. we probably can never get close enough to the target)

		goal.type = ICmpPathfinder::Goal::CIRCLE;
		goal.x = x;
		goal.z = z;

		// Formerly added m_Radius, but it seems better to go by the mid-point.
		goal.hw = goalDistance;
	}

	m_State = STATE_INDIVIDUAL_PATH;
	m_TargetEntity = target;
	m_TargetOffset = CFixedVector2D();
	m_TargetMinRange = minRange;
	m_TargetMaxRange = maxRange;
	m_FinalGoal = goal;

	BeginPathing(pos, goal);

	return true;
}

bool CCmpUnitMotion::IsInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange)
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	bool hasObstruction = false;
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	ICmpObstructionManager::ObstructionSquare obstruction;
	if (cmpObstructionManager)
		hasObstruction = cmpObstructionManager->FindMostImportantObstruction(GetObstructionFilter(true), x, z, m_Radius, obstruction);

	if (minRange.IsZero() && maxRange.IsZero() && hasObstruction)
	{
		// Handle the non-ranged mode:
		CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize);

		// See if we're too close to the target square
		if (distance < minRange)
			return false;

		// See if we're close enough to the target square
		if (maxRange < entity_pos_t::Zero() || distance <= maxRange)
			return true;

		return false;
	}
	else
	{
		entity_pos_t distance = (pos - CFixedVector2D(x, z)).Length();

		if (distance < minRange)
		{
			return false;
		}
		else if (maxRange >= entity_pos_t::Zero() && distance > maxRange)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
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

bool CCmpUnitMotion::MoveToTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	PROFILE("MoveToTargetRange");

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager)
		return false;

	bool hasObstruction = false;
	ICmpObstructionManager::ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), target);
	if (cmpObstruction)
		hasObstruction = cmpObstruction->GetObstructionSquare(obstruction);

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

	if (hasObstruction)
	{
		CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
		ICmpPathfinder::Goal goal;
		goal.x = obstruction.x;
		goal.z = obstruction.z;

		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize);

		// compare with previous obstruction
		ICmpObstructionManager::ObstructionSquare previousObstruction;
		cmpObstruction->GetPreviousObstructionSquare(previousObstruction);
		entity_pos_t previousDistance = Geometry::DistanceToSquare(pos - CFixedVector2D(previousObstruction.x, previousObstruction.z), obstruction.u, obstruction.v, halfSize);

		if (distance < minRange && previousDistance < minRange)
		{
			// Too close to the square - need to move away

			// TODO: maybe we should do the ShouldTreatTargetAsCircle thing here?

			entity_pos_t goalDistance = minRange + g_GoalDelta;

			goal.type = ICmpPathfinder::Goal::SQUARE;
			goal.u = obstruction.u;
			goal.v = obstruction.v;
			entity_pos_t delta = std::max(goalDistance, m_Radius + entity_pos_t::FromInt(TERRAIN_TILE_SIZE)/16); // ensure it's far enough to not intersect the building itself
			goal.hw = obstruction.hw + delta;
			goal.hh = obstruction.hh + delta;
		}
		else if (maxRange < entity_pos_t::Zero() || distance < maxRange || previousDistance < maxRange)
		{
			// We're already in range - no need to move anywhere
			if (m_FacePointAfterMove)
				FaceTowardsPointFromPos(pos, goal.x, goal.z);
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
					if (m_FacePointAfterMove)
						FaceTowardsPointFromPos(pos, goal.x, goal.z);
					return false;
				}

				entity_pos_t previousCircleDistance = (pos - CFixedVector2D(previousObstruction.x, previousObstruction.z)).Length() - circleRadius;

				if (previousCircleDistance < maxRange)
				{
					// We're already in range - no need to move anywhere
					if (m_FacePointAfterMove)
						FaceTowardsPointFromPos(pos, goal.x, goal.z);
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
				entity_pos_t delta = std::max(goalDistance, m_Radius + entity_pos_t::FromInt(TERRAIN_TILE_SIZE)/16); // ensure it's far enough to not intersect the building itself
				goal.hw = obstruction.hw + delta;
				goal.hh = obstruction.hh + delta;
			}
		}

		m_State = STATE_INDIVIDUAL_PATH;
		m_TargetEntity = target;
		m_TargetOffset = CFixedVector2D();
		m_TargetMinRange = minRange;
		m_TargetMaxRange = maxRange;
		m_FinalGoal = goal;

		BeginPathing(pos, goal);

		return true;
	}
	else
	{
		// The target didn't have an obstruction or obstruction shape, so treat it as a point instead

		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (!cmpTargetPosition || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector2D targetPos = cmpTargetPosition->GetPosition2D();

		return MoveToPointRange(targetPos.X, targetPos.Y, minRange, maxRange, target);
	}
}

bool CCmpUnitMotion::IsInTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	// This function closely mirrors MoveToTargetRange - it needs to return true
	// after that Move has completed

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	if (!cmpObstructionManager)
		return false;

	bool hasObstruction = false;
	ICmpObstructionManager::ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetSimContext(), target);
	if (cmpObstruction)
		hasObstruction = cmpObstruction->GetObstructionSquare(obstruction);

	if (hasObstruction)
	{
		CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize);

		// compare with previous obstruction
		ICmpObstructionManager::ObstructionSquare previousObstruction;
		cmpObstruction->GetPreviousObstructionSquare(previousObstruction);
		entity_pos_t previousDistance = Geometry::DistanceToSquare(pos - CFixedVector2D(previousObstruction.x, previousObstruction.z), obstruction.u, obstruction.v, halfSize);
		
		// See if we're too close to the target square
		if (distance < minRange && previousDistance < minRange)
			return false;

		// See if we're close enough to the target square
		if (maxRange < entity_pos_t::Zero() || distance <= maxRange || previousDistance <= maxRange)
			return true;

		entity_pos_t circleRadius = halfSize.Length();

		if (ShouldTreatTargetAsCircle(maxRange, obstruction.hw, obstruction.hh, circleRadius))
		{
			// The target is small relative to our range, so pretend it's a circle
			// and see if we're close enough to that

			entity_pos_t circleDistance = (pos - CFixedVector2D(obstruction.x, obstruction.z)).Length() - circleRadius;

			if (circleDistance <= maxRange)
				return true;
			// also check circle around previous position
			circleDistance = (pos - CFixedVector2D(previousObstruction.x, previousObstruction.z)).Length() - circleRadius;

			if (circleDistance <= maxRange)
				return true;
		}

		return false;
	}
	else
	{
		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (!cmpTargetPosition || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector2D targetPos = cmpTargetPosition->GetPreviousPosition2D();

		entity_pos_t distance = (pos - targetPos).Length();

		return minRange <= distance && 
			(maxRange < entity_pos_t::Zero() || distance <= maxRange);
	}
}

void CCmpUnitMotion::MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z)
{
	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), target);
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	ICmpPathfinder::Goal goal;
	goal.type = ICmpPathfinder::Goal::POINT;
	goal.x = pos.X;
	goal.z = pos.Y;

	m_State = STATE_FORMATIONMEMBER_PATH;
	m_TargetEntity = target;
	m_TargetOffset = CFixedVector2D(x, z);
	m_TargetMinRange = entity_pos_t::Zero();
	m_TargetMaxRange = entity_pos_t::Zero();
	m_FinalGoal = goal;

	BeginPathing(pos, goal);
}





void CCmpUnitMotion::RenderPath(const ICmpPathfinder::Path& path, std::vector<SOverlayLine>& lines, CColor color)
{
	bool floating = false;
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (cmpPosition)
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

	RenderPath(m_LongPath, m_DebugOverlayLongPathLines, OVERLAY_COLOUR_LONG_PATH);
	RenderPath(m_ShortPath, m_DebugOverlayShortPathLines, OVERLAY_COLOUR_SHORT_PATH);

	for (size_t i = 0; i < m_DebugOverlayLongPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLongPathLines[i]);

	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}
