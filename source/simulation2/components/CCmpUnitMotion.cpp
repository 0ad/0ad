/* Copyright (C) 2019 Wildfire Games.
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

// For debugging; units will start going straight to the target
// instead of calling the pathfinder
#define DISABLE_PATHFINDER 0

/**
 * When advancing along the long path, and picking a new waypoint to move
 * towards, we'll pick one that's up to this far from the unit's current
 * position (to minimise the effects of grid-constrained movement)
 */
static const entity_pos_t WAYPOINT_ADVANCE_MAX = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*8);

/**
 * Min/Max range to restrict short path queries to. (Larger ranges are slower,
 * smaller ranges might miss some legitimate routes around large obstacles.)
 */
static const entity_pos_t SHORT_PATH_MIN_SEARCH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*2);
static const entity_pos_t SHORT_PATH_MAX_SEARCH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*9);

/**
 * Minimum distance to goal for a long path request
 */
static const entity_pos_t LONG_PATH_MIN_DIST = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*4);

/**
 * When short-pathing, and the short-range pathfinder failed to return a path,
 * Assume we are at destination if we are closer than this distance to the target
 * And we have no target entity.
 * This is somewhat arbitrary, but setting a too big distance means units might lose sight of their end goal too much;
 */
static const entity_pos_t SHORT_PATH_GOAL_RADIUS = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*2);

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

static const CColor OVERLAY_COLOR_LONG_PATH(1, 1, 1, 1);
static const CColor OVERLAY_COLOR_SHORT_PATH(1, 0, 0, 1);

class CCmpUnitMotion : public ICmpUnitMotion
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Update_MotionFormation);
		componentManager.SubscribeToMessageType(MT_Update_MotionUnit);
		componentManager.SubscribeToMessageType(MT_PathResult);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_ValueModification);
		componentManager.SubscribeToMessageType(MT_Deserialized);
	}

	DEFAULT_COMPONENT_ALLOCATOR(UnitMotion)

	bool m_DebugOverlayEnabled;
	std::vector<SOverlayLine> m_DebugOverlayLongPathLines;
	std::vector<SOverlayLine> m_DebugOverlayShortPathLines;

	// Template state:

	bool m_FormationController;

	fixed m_TemplateWalkSpeed, m_TemplateRunMultiplier;
	pass_class_t m_PassClass;
	std::string m_PassClassName;

	// Dynamic state:

	entity_pos_t m_Clearance;

	// cached for efficiency
	fixed m_WalkSpeed, m_RunMultiplier;

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

		PATHSTATE_MAX
	};
	u8 m_PathState;

	u32 m_ExpectedPathTicket; // asynchronous request ID we're waiting for, or 0 if none

	entity_id_t m_TargetEntity;
	CFixedVector2D m_TargetPos;
	CFixedVector2D m_TargetOffset;
	entity_pos_t m_TargetMinRange;
	entity_pos_t m_TargetMaxRange;

	// If the entity moves, it will do so at m_WalkSpeed * m_SpeedMultiplier.
	fixed m_SpeedMultiplier;
	// This caches the resulting speed from m_WalkSpeed * m_SpeedMultiplier for convenience.
	fixed m_Speed;

	// Current mean speed (over the last turn).
	fixed m_CurSpeed;

	// Currently active paths (storing waypoints in reverse order).
	// The last item in each path is the point we're currently heading towards.
	WaypointPath m_LongPath;
	WaypointPath m_ShortPath;

	// Motion planning
	u8 m_Tries; // how many tries we've done to get to our current Final Goal.

	PathGoal m_FinalGoal;

	static std::string GetSchema()
	{
		return
			"<a:help>Provides the unit with the ability to move around the world by itself.</a:help>"
			"<a:example>"
				"<WalkSpeed>7.0</WalkSpeed>"
				"<PassabilityClass>default</PassabilityClass>"
			"</a:example>"
			"<element name='FormationController'>"
				"<data type='boolean'/>"
			"</element>"
			"<element name='WalkSpeed' a:help='Basic movement speed (in metres per second)'>"
				"<ref name='positiveDecimal'/>"
			"</element>"
			"<optional>"
				"<element name='RunMultiplier' a:help='How much faster the unit goes when running (as a multiple of walk speed)'>"
					"<ref name='positiveDecimal'/>"
				"</element>"
			"</optional>"
			"<element name='PassabilityClass' a:help='Identifies the terrain passability class (values are defined in special/pathfinder.xml)'>"
				"<text/>"
			"</element>";
	}

	virtual void Init(const CParamNode& paramNode)
	{
		m_FormationController = paramNode.GetChild("FormationController").ToBool();

		m_Moving = false;
		m_FacePointAfterMove = true;

		m_WalkSpeed = m_TemplateWalkSpeed = m_Speed = paramNode.GetChild("WalkSpeed").ToFixed();
		m_SpeedMultiplier = fixed::FromInt(1);
		m_CurSpeed = fixed::Zero();

		m_RunMultiplier = m_TemplateRunMultiplier = fixed::FromInt(1);
		if (paramNode.GetChild("RunMultiplier").IsOk())
			m_RunMultiplier = m_TemplateRunMultiplier = paramNode.GetChild("RunMultiplier").ToFixed();

		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
		{
			m_PassClassName = paramNode.GetChild("PassabilityClass").ToUTF8();
			m_PassClass = cmpPathfinder->GetPassabilityClass(m_PassClassName);
			m_Clearance = cmpPathfinder->GetClearance(m_PassClass);

			CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
			if (cmpObstruction)
				cmpObstruction->SetUnitClearance(m_Clearance);
		}

		m_State = STATE_IDLE;
		m_PathState = PATHSTATE_NONE;

		m_ExpectedPathTicket = 0;

		m_Tries = 0;

		m_TargetEntity = INVALID_ENTITY;

		m_FinalGoal.type = PathGoal::POINT;

		m_DebugOverlayEnabled = false;
	}

	virtual void Deinit()
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
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

		serialize.NumberFixed_Unbounded("speed multiplier", m_SpeedMultiplier);

		serialize.NumberFixed_Unbounded("current speed", m_CurSpeed);

		serialize.Bool("moving", m_Moving);
		serialize.Bool("facePointAfterMove", m_FacePointAfterMove);

		serialize.NumberU8("tries", m_Tries, 0, 255);

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
			PROFILE("UnitMotion::RenderSubmit");
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
			FALLTHROUGH;
		}
		case MT_OwnershipChanged:
		case MT_Deserialized:
		{
			CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSystemEntity());
			if (!cmpValueModificationManager)
				break;

			m_WalkSpeed = cmpValueModificationManager->ApplyModifications(L"UnitMotion/WalkSpeed", m_TemplateWalkSpeed, GetEntityId());
			m_RunMultiplier = cmpValueModificationManager->ApplyModifications(L"UnitMotion/RunMultiplier", m_TemplateRunMultiplier, GetEntityId());

			// For MT_Deserialize compute m_Speed from the serialized m_SpeedMultiplier.
			// For MT_ValueModification and MT_OwnershipChanged, adjust m_SpeedMultiplier if needed
			// (in case then new m_RunMultiplier value is lower than the old).
			SetSpeedMultiplier(m_SpeedMultiplier);

			break;
		}
		}
	}

	void UpdateMessageSubscriptions()
	{
		bool needRender = m_DebugOverlayEnabled;
		GetSimContext().GetComponentManager().DynamicSubscriptionNonsync(MT_RenderSubmit, this, needRender);
	}

	virtual bool IsMoving() const
	{
		return m_Moving;
	}

	virtual fixed GetSpeedMultiplier() const
	{
		return m_SpeedMultiplier;
	}

	virtual void SetSpeedMultiplier(fixed multiplier)
	{
		m_SpeedMultiplier = std::min(multiplier, m_RunMultiplier);
		m_Speed = m_SpeedMultiplier.Multiply(GetWalkSpeed());
	}

	virtual fixed GetSpeed() const
	{
		return m_Speed;
	}

	virtual fixed GetWalkSpeed() const
	{
		return m_WalkSpeed;
	}

	virtual fixed GetRunMultiplier() const
	{
		return m_RunMultiplier;
	}

	virtual pass_class_t GetPassabilityClass() const
	{
		return m_PassClass;
	}

	virtual std::string GetPassabilityClassName() const
	{
		return m_PassClassName;
	}

	virtual void SetPassabilityClassName(const std::string& passClassName)
	{
		m_PassClassName = passClassName;
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		if (cmpPathfinder)
			m_PassClass = cmpPathfinder->GetPassabilityClass(passClassName);
	}

	virtual fixed GetCurrentSpeed() const
	{
		return m_CurSpeed;
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
	virtual bool IsInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange) const;
	virtual bool MoveToTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);
	virtual bool IsInTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) const;
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

	virtual entity_pos_t GetUnitClearance() const
	{
		return m_Clearance;
	}

private:
	bool ShouldAvoidMovingUnits() const
	{
		return !m_FormationController;
	}

	bool IsFormationMember() const
	{
		return m_State == STATE_FORMATIONMEMBER_PATH;
	}

	entity_id_t GetGroup() const
	{
		return IsFormationMember() ? m_TargetEntity : GetEntityId();
	}

	bool HasValidPath() const
	{
		return m_PathState == PATHSTATE_FOLLOWING
			|| m_PathState == PATHSTATE_FOLLOWING_REQUESTING_LONG
			|| m_PathState == PATHSTATE_FOLLOWING_REQUESTING_SHORT;
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
		CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(true);

		m_Moving = true;

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
	void PathResult(u32 ticket, const WaypointPath& path);

	/**
	 * Do the per-turn movement and other updates.
	 */
	void Move(fixed dt);

	/**
	 * Decide whether to approximate the given range from a square target as a circle,
	 * rather than as a square.
	 */
	bool ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t circleRadius) const;

	/**
	 * Computes the current location of our target entity (plus offset).
	 * Returns false if no target entity or no valid position.
	 */
	bool ComputeTargetPosition(CFixedVector2D& out) const;

	/**
	 * Attempts to replace the current path with a straight line to the goal,
	 * if this goal is a point, is close enough and the route is not obstructed.
	 */
	bool TryGoingStraightToGoalPoint(const CFixedVector2D& from);

	/**
	 * Attempts to replace the current path with a straight line to the target
	 * entity, if it's close enough and the route is not obstructed.
	 */
	bool TryGoingStraightToTargetEntity(const CFixedVector2D& from);

	/**
	 * Returns whether the target entity has moved more than minDelta since our
	 * last path computations, and we're close enough to it to care.
	 */
	bool CheckTargetMovement(const CFixedVector2D& from, entity_pos_t minDelta);

	/**
	 * Update goal position if moving target
	 */
	void UpdateFinalGoal();

	/**
	 * Returns whether we are close enough to the target to assume it's a good enough
	 * position to stop.
	 */
	bool CloseEnoughFromDestinationToStop(const CFixedVector2D& from) const;

	/**
	 * Returns whether the length of the given path, plus the distance from
	 * 'from' to the first waypoints, it shorter than minDistance.
	 */
	bool PathIsShort(const WaypointPath& path, const CFixedVector2D& from, entity_pos_t minDistance) const;

	/**
	 * Rotate to face towards the target point, given the current pos
	 */
	void FaceTowardsPointFromPos(const CFixedVector2D& pos, entity_pos_t x, entity_pos_t z);

	/**
	 * Returns an appropriate obstruction filter for use with path requests.
	 * noTarget is true only when used inside tryGoingStraightToTargetEntity,
	 * in which case we do not want the target obstruction otherwise it would always fail
	 */
	ControlGroupMovementObstructionFilter GetObstructionFilter(bool noTarget = false) const;

	/**
	 * Start moving to the given goal, from our current position 'from'.
	 * Might go in a straight line immediately, or might start an asynchronous
	 * path request.
	 */
	void BeginPathing(const CFixedVector2D& from, const PathGoal& goal);

	/**
	 * Start an asynchronous long path query.
	 */
	void RequestLongPath(const CFixedVector2D& from, const PathGoal& goal);

	/**
	 * Start an asynchronous short path query.
	 */
	void RequestShortPath(const CFixedVector2D& from, const PathGoal& goal, bool avoidMovingUnits);

	/**
	 * Convert a path into a renderable list of lines
	 */
	void RenderPath(const WaypointPath& path, std::vector<SOverlayLine>& lines, CColor color);

	void RenderSubmit(SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(UnitMotion)

void CCmpUnitMotion::PathResult(u32 ticket, const WaypointPath& path)
{
	// reset our state for sanity.
	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	if (cmpObstruction)
		cmpObstruction->SetMovingFlag(false);

	m_Moving = false;

	// Ignore obsolete path requests
	if (ticket != m_ExpectedPathTicket)
		return;

	m_ExpectedPathTicket = 0; // we don't expect to get this result again

	// Check that we are still able to do something with that path
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
	{
		if (m_PathState == PATHSTATE_WAITING_REQUESTING_LONG || m_PathState == PATHSTATE_WAITING_REQUESTING_SHORT)
			StartFailed();
		else if (m_PathState == PATHSTATE_FOLLOWING_REQUESTING_LONG || m_PathState == PATHSTATE_FOLLOWING_REQUESTING_SHORT)
			StopMoving();
		return;
	}

	if (m_PathState == PATHSTATE_WAITING_REQUESTING_LONG || m_PathState == PATHSTATE_FOLLOWING_REQUESTING_LONG)
	{
		m_LongPath = path;

		// If we are following a path, leave the old m_ShortPath so we can carry on following it
		// until a new short path has been computed
		if (m_PathState == PATHSTATE_WAITING_REQUESTING_LONG)
			m_ShortPath.m_Waypoints.clear();

		// If there's no waypoints then we couldn't get near the target.
		// Sort of hack: Just try going directly to the goal point instead
		// (via the short pathfinder), so if we're stuck and the user clicks
		// close enough to the unit then we can probably get unstuck
		if (m_LongPath.m_Waypoints.empty())
			m_LongPath.m_Waypoints.emplace_back(Waypoint{ m_FinalGoal.x, m_FinalGoal.z });

		if (!HasValidPath())
			StartSucceeded();

		m_PathState = PATHSTATE_FOLLOWING;

		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(true);

		m_Moving = true;
	}
	else if (m_PathState == PATHSTATE_WAITING_REQUESTING_SHORT || m_PathState == PATHSTATE_FOLLOWING_REQUESTING_SHORT)
	{
		m_ShortPath = path;

		// If there's no waypoints then we couldn't get near the target
		if (m_ShortPath.m_Waypoints.empty())
		{
			// If we're globally following a long path, try to remove the next waypoint, it might be obstructed
			// If not, and we are not in a formation, retry
			// unless we are close to our target and we don't have a target entity.
			// This makes sure that units don't clump too much when they are not in a formation and tasked to move.
			if (m_LongPath.m_Waypoints.size() > 1)
				m_LongPath.m_Waypoints.pop_back();
			else if (IsFormationMember())
			{
				m_Moving = false;
				CMessageMotionChanged msg(true, true);
				GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
				return;
			}

			CMessageMotionChanged msg(false, false);
			GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);

			CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
			if (!cmpPosition || !cmpPosition->IsInWorld())
				return;

			CFixedVector2D pos = cmpPosition->GetPosition2D();

			if (CloseEnoughFromDestinationToStop(pos))
			{
				StopMoving();
				MoveSucceeded();

				if (m_FacePointAfterMove)
					FaceTowardsPointFromPos(pos, m_FinalGoal.x, m_FinalGoal.z);
				return;
			}

			UpdateFinalGoal();
			RequestLongPath(pos, m_FinalGoal);
			m_PathState = PATHSTATE_WAITING_REQUESTING_LONG;
			return;
		}

		// else we could, so reset our number of tries.
		m_Tries = 0;

		// Now we've got a short path that we can follow
		if (!HasValidPath())
			StartSucceeded();

		m_PathState = PATHSTATE_FOLLOWING;

		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(true);

		m_Moving = true;
	}
	else
		LOGWARNING("unexpected PathResult (%u %d %d)", GetEntityId(), m_State, m_PathState);
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

		fixed basicSpeed = m_Speed;
		// If in formation, run to keep up; otherwise just walk
		if (IsFormationMember())
			basicSpeed = m_Speed.Multiply(m_RunMultiplier);

		// Find the speed factor of the underlying terrain
		// (We only care about the tile we start on - it doesn't matter if we're moving
		// partially onto a much slower/faster tile)
		// TODO: Terrain-dependent speeds are not currently supported
		fixed terrainSpeed = fixed::FromInt(1);

		fixed maxSpeed = basicSpeed.Multiply(terrainSpeed);

		bool wasObstructed = false;

		// We want to move (at most) maxSpeed*dt units from pos towards the next waypoint

		fixed timeLeft = dt;
		fixed zero = fixed::Zero();

		while (timeLeft > zero)
		{
			// If we ran out of path, we have to stop
			if (m_ShortPath.m_Waypoints.empty() && m_LongPath.m_Waypoints.empty())
				break;

			CFixedVector2D target;
			if (m_ShortPath.m_Waypoints.empty())
				target = CFixedVector2D(m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z);
			else
				target = CFixedVector2D(m_ShortPath.m_Waypoints.back().x, m_ShortPath.m_Waypoints.back().z);

			CFixedVector2D offset = target - pos;

			// Work out how far we can travel in timeLeft
			fixed maxdist = maxSpeed.Multiply(timeLeft);

			// If the target is close, we can move there directly
			fixed offsetLength = offset.Length();
			if (offsetLength <= maxdist)
			{
				if (cmpPathfinder->CheckMovement(GetObstructionFilter(), pos.X, pos.Y, target.X, target.Y, m_Clearance, m_PassClass))
				{
					pos = target;

					// Spend the rest of the time heading towards the next waypoint
					timeLeft = (maxdist - offsetLength) / maxSpeed;

					if (m_ShortPath.m_Waypoints.empty())
						m_LongPath.m_Waypoints.pop_back();
					else
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

				if (cmpPathfinder->CheckMovement(GetObstructionFilter(), pos.X, pos.Y, target.X, target.Y, m_Clearance, m_PassClass))
					pos = target;
				else
					wasObstructed = true; // Error - path was obstructed

				break;
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
			// This is when we might easily get stuck wrongly.

			// check if we've arrived.
			if (CloseEnoughFromDestinationToStop(pos))
			{
				StopMoving();
				MoveSucceeded();

				if (m_FacePointAfterMove)
					FaceTowardsPointFromPos(pos, m_FinalGoal.x, m_FinalGoal.z);

				return;
			}

			// If we still have long waypoints, try and compute a short path
			// This will get us around units, amongst others.
			// However in some cases a long waypoint will be in located in the obstruction of
			// an idle unit. In that case, we need to scrap that waypoint or we might never be able to reach it.
			// I am not sure why this happens but the following code seems to work.
			if (!m_LongPath.m_Waypoints.empty())
			{
				CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
				if (cmpObstructionManager)
				{
					// create a fake obstruction to represent our waypoint.
					ICmpObstructionManager::ObstructionSquare square;
					square.hh = m_Clearance;
					square.hw = m_Clearance;
					square.u = CFixedVector2D(entity_pos_t::FromInt(1),entity_pos_t::FromInt(0));
					square.v = CFixedVector2D(entity_pos_t::FromInt(0),entity_pos_t::FromInt(1));
					square.x = m_LongPath.m_Waypoints.back().x;
					square.z = m_LongPath.m_Waypoints.back().z;
					std::vector<entity_id_t> unitOnGoal;
					// don't ignore moving units as those might be units like us, ie not really moving.
					cmpObstructionManager->GetUnitsOnObstruction(square, unitOnGoal, GetObstructionFilter(), true);
					if (!unitOnGoal.empty())
						m_LongPath.m_Waypoints.pop_back();
				}
				if (!m_LongPath.m_Waypoints.empty())
				{
					PathGoal goal;
					if (m_LongPath.m_Waypoints.size() > 1 || m_FinalGoal.DistanceToPoint(pos) > LONG_PATH_MIN_DIST)
						goal = { PathGoal::POINT, m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z };
					else
					{
						UpdateFinalGoal();
						goal = m_FinalGoal;
						m_LongPath.m_Waypoints.clear();
						CFixedVector2D target = goal.NearestPointOnGoal(pos);
						m_LongPath.m_Waypoints.emplace_back(Waypoint{ target.X, target.Y });
					}
					RequestShortPath(pos, goal, true);
					m_PathState = PATHSTATE_WAITING_REQUESTING_SHORT;
					return;
				}
			}
			// Else, just entirely recompute
			UpdateFinalGoal();
			BeginPathing(pos, m_FinalGoal);

			// potential TODO: We could switch the short-range pathfinder for something else entirely.
			return;
		}

		// We successfully moved along our path, until running out of
		// waypoints or time.

		if (m_PathState == PATHSTATE_FOLLOWING)
		{
			// If we're not currently computing any new paths:
			if (m_LongPath.m_Waypoints.empty() && m_ShortPath.m_Waypoints.empty())
			{
				if (IsFormationMember())
				{
					// We've reached our assigned position. If the controller
					// is idle, send a notification in case it should disband,
					// otherwise continue following the formation next turn.
					CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), m_TargetEntity);
					if (cmpUnitMotion && !cmpUnitMotion->IsMoving())
					{
						CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
						if (cmpObstruction)
							cmpObstruction->SetMovingFlag(false);

						m_Moving = false;
						CMessageMotionChanged msg(false, false);
						GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
					}
				}
				else
				{
					// check if target was reached in case of a moving target
					CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), m_TargetEntity);
					if (cmpUnitMotion && cmpUnitMotion->IsMoving() &&
						MoveToTargetRange(m_TargetEntity, m_TargetMinRange, m_TargetMaxRange))
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

			// If we have a target entity, and we're not miles away from the end of
			// our current path, and the target moved enough, then recompute our
			// whole path
			if (IsFormationMember())
				CheckTargetMovement(pos, CHECK_TARGET_MOVEMENT_MIN_DELTA_FORMATION);
			else
				CheckTargetMovement(pos, CHECK_TARGET_MOVEMENT_MIN_DELTA);
		}
	}
	}
}

bool CCmpUnitMotion::ComputeTargetPosition(CFixedVector2D& out) const
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

bool CCmpUnitMotion::TryGoingStraightToGoalPoint(const CFixedVector2D& from)
{
	// Make sure the goal is a point (and not a point-like target like a formation controller)
	if (m_FinalGoal.type != PathGoal::POINT || m_TargetEntity != INVALID_ENTITY)
		return false;

	// Fail if the goal is too far away
	CFixedVector2D goalPos(m_FinalGoal.x, m_FinalGoal.z);
	if ((goalPos - from).CompareLength(DIRECT_PATH_RANGE) > 0)
		return false;

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return false;

	// Check if there's any collisions on that route
	if (!cmpPathfinder->CheckMovement(GetObstructionFilter(), from.X, from.Y, goalPos.X, goalPos.Y, m_Clearance, m_PassClass))
		return false;

	// That route is okay, so update our path
	m_LongPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.emplace_back(Waypoint{ goalPos.X, goalPos.Y });

	return true;
}

bool CCmpUnitMotion::TryGoingStraightToTargetEntity(const CFixedVector2D& from)
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
	PathGoal goal = m_FinalGoal;
	goal.x = targetPos.X;
	goal.z = targetPos.Y;
	// (we ignore changes to the target's rotation, since only buildings are
	// square and buildings don't move)

	// Find the point on the goal shape that we should head towards
	CFixedVector2D goalPos = goal.NearestPointOnGoal(from);

	// Check if there's any collisions on that route
	if (!cmpPathfinder->CheckMovement(GetObstructionFilter(true), from.X, from.Y, goalPos.X, goalPos.Y, m_Clearance, m_PassClass))
		return false;

	// That route is okay, so update our path
	m_FinalGoal = goal;
	m_LongPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.emplace_back(Waypoint{ goalPos.X, goalPos.Y });

	return true;
}

bool CCmpUnitMotion::CheckTargetMovement(const CFixedVector2D& from, entity_pos_t minDelta)
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
		if (cmpRangeManager && cmpRangeManager->GetLosVisibility(m_TargetEntity, cmpOwnership->GetOwner()) == ICmpRangeManager::VIS_HIDDEN)
			return false;
	}

	// The target moved and we need to update our current path;
	// change the goal here and expect our caller to start the path request
	m_FinalGoal.x = targetPos.X;
	m_FinalGoal.z = targetPos.Y;
	RequestLongPath(from, m_FinalGoal);
	m_PathState = PATHSTATE_FOLLOWING_REQUESTING_LONG;

	return true;
}

void CCmpUnitMotion::UpdateFinalGoal()
{
	if (m_TargetEntity == INVALID_ENTITY)
		return;
	CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), m_TargetEntity);
	if (!cmpUnitMotion)
		return;
	if (IsFormationMember())
		return;
	CFixedVector2D targetPos;
	if (!ComputeTargetPosition(targetPos))
		return;
	m_FinalGoal.x = targetPos.X;
	m_FinalGoal.z = targetPos.Y;
}

bool CCmpUnitMotion::CloseEnoughFromDestinationToStop(const CFixedVector2D& from) const
{
	if (m_TargetEntity != INVALID_ENTITY || m_FinalGoal.DistanceToPoint(from) > SHORT_PATH_GOAL_RADIUS)
		return false;
	return true;
}

bool CCmpUnitMotion::PathIsShort(const WaypointPath& path, const CFixedVector2D& from, entity_pos_t minDistance) const
{
	CFixedVector2D prev = from;
	entity_pos_t distLeft = minDistance;

	for (ssize_t i = (ssize_t)path.m_Waypoints.size()-1; i >= 0; --i)
	{
		// Check if the next path segment is longer than the requested minimum
		CFixedVector2D waypoint(path.m_Waypoints[i].x, path.m_Waypoints[i].z);
		CFixedVector2D delta = waypoint - prev;
		if (delta.CompareLength(distLeft) > 0)
			return false;

		// Still short enough - prepare to check the next segment
		distLeft -= delta.Length();
		prev = waypoint;
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

void CCmpUnitMotion::FaceTowardsPointFromPos(const CFixedVector2D& pos, entity_pos_t x, entity_pos_t z)
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

ControlGroupMovementObstructionFilter CCmpUnitMotion::GetObstructionFilter(bool noTarget) const
{
	entity_id_t group = noTarget ? m_TargetEntity : GetGroup();
	return ControlGroupMovementObstructionFilter(ShouldAvoidMovingUnits(), group);
}



void CCmpUnitMotion::BeginPathing(const CFixedVector2D& from, const PathGoal& goal)
{
	// reset our state for sanity.
	m_ExpectedPathTicket = 0;

	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	if (cmpObstruction)
		cmpObstruction->SetMovingFlag(false);

	m_Moving = false;

	m_PathState = PATHSTATE_NONE;

#if DISABLE_PATHFINDER
	{
		CmpPtr<ICmpPathfinder> cmpPathfinder (GetSimContext(), SYSTEM_ENTITY);
		CFixedVector2D goalPos = m_FinalGoal.NearestPointOnGoal(from);
		m_LongPath.m_Waypoints.clear();
		m_ShortPath.m_Waypoints.clear();
		m_ShortPath.m_Waypoints.emplace_back(Waypoint{ goalPos.X, goalPos.Y });
		m_PathState = PATHSTATE_FOLLOWING;
		return;
	}
#endif

	// If we're aiming at a target entity and it's close and we can reach
	// it in a straight line, then we'll just go along the straight line
	// instead of computing a path.
	if (TryGoingStraightToTargetEntity(from))
	{
		if (!HasValidPath())
			StartSucceeded();
		m_PathState = PATHSTATE_FOLLOWING;
		return;
	}

	// Same thing applies to non-entity points
	if (TryGoingStraightToGoalPoint(from))
	{
		if (!HasValidPath())
			StartSucceeded();
		m_PathState = PATHSTATE_FOLLOWING;
		return;
	}

	// Otherwise we need to compute a path.

	// If it's close then just do a short path, not a long path
	// TODO: If it's close on the opposite side of a river then we really
	// need a long path, so we shouldn't simply check linear distance
	// the check is arbitrary but should be a reasonably small distance.
	if (goal.DistanceToPoint(from) < LONG_PATH_MIN_DIST)
	{
		// add our final goal as a long range waypoint so we don't forget
		// where we are going if the short-range pathfinder returns
		// an aborted path.
		m_LongPath.m_Waypoints.clear();
		CFixedVector2D target = m_FinalGoal.NearestPointOnGoal(from);
		m_LongPath.m_Waypoints.emplace_back(Waypoint{ target.X, target.Y });
		m_PathState = PATHSTATE_WAITING_REQUESTING_SHORT;
		RequestShortPath(from, goal, true);
	}
	else
	{
		m_PathState = PATHSTATE_WAITING_REQUESTING_LONG;
		RequestLongPath(from, goal);
	}
}

void CCmpUnitMotion::RequestLongPath(const CFixedVector2D& from, const PathGoal& goal)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	// this is by how much our waypoints will be apart at most.
	// this value here seems sensible enough.
	PathGoal improvedGoal = goal;
	improvedGoal.maxdist = SHORT_PATH_MIN_SEARCH_RANGE - entity_pos_t::FromInt(1);

	cmpPathfinder->SetDebugPath(from.X, from.Y, improvedGoal, m_PassClass);

	m_ExpectedPathTicket = cmpPathfinder->ComputePathAsync(from.X, from.Y, improvedGoal, m_PassClass, GetEntityId());
}

void CCmpUnitMotion::RequestShortPath(const CFixedVector2D &from, const PathGoal& goal, bool avoidMovingUnits)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	// wrapping around on m_Tries isn't really a problem so don't check for overflow.
	fixed searchRange = std::max(SHORT_PATH_MIN_SEARCH_RANGE * ++m_Tries, goal.DistanceToPoint(from));
	if (goal.type != PathGoal::POINT && searchRange < goal.hw && searchRange < SHORT_PATH_MIN_SEARCH_RANGE * 2)
		searchRange = std::min(goal.hw, SHORT_PATH_MIN_SEARCH_RANGE * 2);
	if (searchRange > SHORT_PATH_MAX_SEARCH_RANGE)
		searchRange = SHORT_PATH_MAX_SEARCH_RANGE;

	m_ExpectedPathTicket = cmpPathfinder->ComputeShortPathAsync(from.X, from.Y, m_Clearance, searchRange, goal, m_PassClass, avoidMovingUnits, GetGroup(), GetEntityId());
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

	PathGoal goal;
	goal.x = x;
	goal.z = z;

	if (minRange.IsZero() && maxRange.IsZero())
	{
		// Non-ranged movement:

		// Head directly for the goal
		goal.type = PathGoal::POINT;
	}
	else
	{
		// Ranged movement:

		entity_pos_t distance = (pos - CFixedVector2D(x, z)).Length();

		if (distance < minRange)
		{
			// Too close to target - move outwards to a circle
			// that's slightly larger than the min range
			goal.type = PathGoal::INVERTED_CIRCLE;
			goal.hw = minRange + Pathfinding::GOAL_DELTA;
		}
		else if (maxRange >= entity_pos_t::Zero() && distance > maxRange)
		{
			// Too far from target - move inwards to a circle
			// that's slightly smaller than the max range
			goal.type = PathGoal::CIRCLE;
			goal.hw = maxRange - Pathfinding::GOAL_DELTA;

			// If maxRange was abnormally small,
			// collapse the circle into a point
			if (goal.hw <= entity_pos_t::Zero())
				goal.type = PathGoal::POINT;
		}
		else
		{
			// We're already in range - no need to move anywhere
			if (m_FacePointAfterMove)
				FaceTowardsPointFromPos(pos, x, z);
			return false;
		}
	}

	m_State = STATE_INDIVIDUAL_PATH;
	m_TargetEntity = target;
	m_TargetOffset = CFixedVector2D();
	m_TargetMinRange = minRange;
	m_TargetMaxRange = maxRange;
	m_FinalGoal = goal;
	m_Tries = 0;

	BeginPathing(pos, goal);

	return true;
}

bool CCmpUnitMotion::IsInPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange) const
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	bool hasObstruction = false;
	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	ICmpObstructionManager::ObstructionSquare obstruction;
//TODO	if (cmpObstructionManager)
//		hasObstruction = cmpObstructionManager->FindMostImportantObstruction(GetObstructionFilter(), x, z, m_Radius, obstruction);

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
			return false;
		else if (maxRange >= entity_pos_t::Zero() && distance > maxRange)
			return false;
		else
			return true;
	}
}

bool CCmpUnitMotion::ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t circleRadius) const
{
	// Given a square, plus a target range we should reach, the shape at that distance
	// is a round-cornered square which we can approximate as either a circle or as a square.
	// Previously, we used the shape that minimized the worst-case error.
	// However that is unsage in some situations. So let's be less clever and
	// just check if our range is at least three times bigger than the circleradius
	return (range > circleRadius*3);
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

	if (!hasObstruction)
	{
		// The target didn't have an obstruction or obstruction shape, so treat it as a point instead

		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (!cmpTargetPosition || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector2D targetPos = cmpTargetPosition->GetPosition2D();

		return MoveToPointRange(targetPos.X, targetPos.Y, minRange, maxRange);
	}

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

	CFixedVector2D halfSize(obstruction.hw, obstruction.hh);
	PathGoal goal;
	goal.x = obstruction.x;
	goal.z = obstruction.z;

	entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize, true);

	// Compare with previous obstruction
	ICmpObstructionManager::ObstructionSquare previousObstruction;
	cmpObstruction->GetPreviousObstructionSquare(previousObstruction);
	entity_pos_t previousDistance = Geometry::DistanceToSquare(pos - CFixedVector2D(previousObstruction.x, previousObstruction.z), obstruction.u, obstruction.v, halfSize, true);

	bool inside = distance.IsZero() && !Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize).IsZero();
	if ((distance < minRange && previousDistance < minRange) || inside)
	{
		// Too close to the square - need to move away

		// Circumscribe the square
		entity_pos_t circleRadius = halfSize.Length();

		entity_pos_t goalDistance = minRange + Pathfinding::GOAL_DELTA;

		if (ShouldTreatTargetAsCircle(minRange, circleRadius))
		{
			// The target is small relative to our range, so pretend it's a circle
			goal.type = PathGoal::INVERTED_CIRCLE;
			goal.hw = circleRadius + goalDistance;
		}
		else
		{
			goal.type = PathGoal::INVERTED_SQUARE;
			goal.u = obstruction.u;
			goal.v = obstruction.v;
			goal.hw = obstruction.hw + goalDistance;
			goal.hh = obstruction.hh + goalDistance;
		}
	}
	else if (maxRange < entity_pos_t::Zero() || distance < maxRange || previousDistance < maxRange)
	{
		// We're already in range - no need to move anywhere
		FaceTowardsPointFromPos(pos, goal.x, goal.z);
		return false;
	}
	else
	{
		// We might need to move closer:

		// Circumscribe the square
		entity_pos_t circleRadius = halfSize.Length();

		if (ShouldTreatTargetAsCircle(maxRange, circleRadius))
		{
			// The target is small relative to our range, so pretend it's a circle

			// Note that the distance to the circle will always be less than
			// the distance to the square, so the previous "distance < maxRange"
			// check is still valid (though not sufficient)
			entity_pos_t circleDistance = (pos - CFixedVector2D(obstruction.x, obstruction.z)).Length() - circleRadius;
			entity_pos_t previousCircleDistance = (pos - CFixedVector2D(previousObstruction.x, previousObstruction.z)).Length() - circleRadius;

			if (circleDistance < maxRange || previousCircleDistance < maxRange)
			{
				// We're already in range - no need to move anywhere
				if (m_FacePointAfterMove)
					FaceTowardsPointFromPos(pos, goal.x, goal.z);
				return false;
			}

			entity_pos_t goalDistance = maxRange - Pathfinding::GOAL_DELTA;

			goal.type = PathGoal::CIRCLE;
			goal.hw = circleRadius + goalDistance;
		}
		else
		{
			// The target is large relative to our range, so treat it as a square and
			// get close enough that the diagonals come within range

			entity_pos_t goalDistance = (maxRange - Pathfinding::GOAL_DELTA)*2 / 3; // multiply by slightly less than 1/sqrt(2)

			goal.type = PathGoal::SQUARE;
			goal.u = obstruction.u;
			goal.v = obstruction.v;
			entity_pos_t delta = std::max(goalDistance, m_Clearance + entity_pos_t::FromInt(TERRAIN_TILE_SIZE)/16); // ensure it's far enough to not intersect the building itself
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
	m_Tries = 0;

	BeginPathing(pos, goal);

	return true;
}

bool CCmpUnitMotion::IsInTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) const
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
		entity_pos_t distance = Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize, true);

		// Compare with previous obstruction
		ICmpObstructionManager::ObstructionSquare previousObstruction;
		cmpObstruction->GetPreviousObstructionSquare(previousObstruction);
		entity_pos_t previousDistance = Geometry::DistanceToSquare(pos - CFixedVector2D(previousObstruction.x, previousObstruction.z), obstruction.u, obstruction.v, halfSize, true);

		// See if we're too close to the target square
		bool inside = distance.IsZero() && !Geometry::DistanceToSquare(pos - CFixedVector2D(obstruction.x, obstruction.z), obstruction.u, obstruction.v, halfSize).IsZero();
		if ((distance < minRange && previousDistance < minRange) || inside)
			return false;

		// See if we're close enough to the target square
		if (maxRange < entity_pos_t::Zero() || distance <= maxRange || previousDistance <= maxRange)
			return true;

		entity_pos_t circleRadius = halfSize.Length();

		if (ShouldTreatTargetAsCircle(maxRange, circleRadius))
		{
			// The target is small relative to our range, so pretend it's a circle
			// and see if we're close enough to that.
			// Also check circle around previous position.
			entity_pos_t circleDistance = (pos - CFixedVector2D(obstruction.x, obstruction.z)).Length() - circleRadius;
			entity_pos_t previousCircleDistance = (pos - CFixedVector2D(previousObstruction.x, previousObstruction.z)).Length() - circleRadius;

			return circleDistance <= maxRange || previousCircleDistance <= maxRange;
		}

		// take minimal clearance required in MoveToTargetRange into account, multiplying by 3/2 for diagonals
		entity_pos_t maxDist = std::max(maxRange, (m_Clearance + entity_pos_t::FromInt(TERRAIN_TILE_SIZE)/16)*3/2);
		return distance <= maxDist || previousDistance <= maxDist;
	}
	else
	{
		CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), target);
		if (!cmpTargetPosition || !cmpTargetPosition->IsInWorld())
			return false;

		CFixedVector2D targetPos = cmpTargetPosition->GetPreviousPosition2D();
		entity_pos_t distance = (pos - targetPos).Length();

		return minRange <= distance && (maxRange < entity_pos_t::Zero() || distance <= maxRange);
	}
}

void CCmpUnitMotion::MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z)
{
	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), target);
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	PathGoal goal;
	goal.type = PathGoal::POINT;
	goal.x = pos.X;
	goal.z = pos.Y;

	m_State = STATE_FORMATIONMEMBER_PATH;
	m_TargetEntity = target;
	m_TargetOffset = CFixedVector2D(x, z);
	m_TargetMinRange = entity_pos_t::Zero();
	m_TargetMaxRange = entity_pos_t::Zero();
	m_FinalGoal = goal;
	m_Tries = 0;

	BeginPathing(pos, goal);
}





void CCmpUnitMotion::RenderPath(const WaypointPath& path, std::vector<SOverlayLine>& lines, CColor color)
{
	bool floating = false;
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (cmpPosition)
		floating = cmpPosition->CanFloat();

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
	float x = cmpPosition->GetPosition2D().X.ToFloat();
	float z = cmpPosition->GetPosition2D().Y.ToFloat();
	waypointCoords.push_back(x);
	waypointCoords.push_back(z);
	lines.push_back(SOverlayLine());
	lines.back().m_Color = color;
	SimRender::ConstructLineOnGround(GetSimContext(), waypointCoords, lines.back(), floating);

}

void CCmpUnitMotion::RenderSubmit(SceneCollector& collector)
{
	if (!m_DebugOverlayEnabled)
		return;

	RenderPath(m_LongPath, m_DebugOverlayLongPathLines, OVERLAY_COLOR_LONG_PATH);
	RenderPath(m_ShortPath, m_DebugOverlayShortPathLines, OVERLAY_COLOR_SHORT_PATH);

	for (size_t i = 0; i < m_DebugOverlayLongPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayLongPathLines[i]);

	for (size_t i = 0; i < m_DebugOverlayShortPathLines.size(); ++i)
		collector.Submit(&m_DebugOverlayShortPathLines[i]);
}
