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
#include "simulation2/components/ICmpVisual.h"
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
 * Min/Max range to restrict short path queries to. (Larger ranges are slower,
 * Min/Max range to restrict short path queries to. (Larger ranges are (much) slower,
 * smaller ranges might miss some legitimate routes around large obstacles.)
 */
static const entity_pos_t SHORT_PATH_MIN_SEARCH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*3)/2;
static const entity_pos_t SHORT_PATH_MAX_SEARCH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*6);
static const entity_pos_t SHORT_PATH_SEARCH_RANGE_INCREMENT = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*1);

/**
 * When using the short-pathfinder to rejoin a long-path waypoint, aim for a circle of this radius around the waypoint.
 */
static const entity_pos_t SHORT_PATH_LONG_WAYPOINT_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*1);

/**
 * Minimum distance to goal for a long path request
 */
static const entity_pos_t LONG_PATH_MIN_DIST = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*4);

/**
 * If we are this close to our target entity/point, then think about heading
 * for it in a straight line instead of pathfinding.
 */
static const entity_pos_t DIRECT_PATH_RANGE = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*4);

/**
 * To avoid recomputing paths too often, have some leeway for target range checks
 * based on our distance to the target. Increase that incertainty by one navcell
 * for every this many tiles of distance.
 */
static const entity_pos_t TARGET_UNCERTAINTY_MULTIPLIER = entity_pos_t::FromInt(TERRAIN_TILE_SIZE*2);

/**
 * When we fail more than this many path computations in a row, inform other components that the move will fail.
 * Experimentally, this number needs to be somewhat high or moving groups of units will lead to stuck units.
 * However, too high means units will look idle for a long time when they are failing to move.
 * TODO: if UnitMotion could send differentiated "unreachable" and "currently stuck" failing messages,
 * this could probably be lowered.
 * TODO: when unit pushing is implemented, this number can probably be lowered.
 */
static const u8 MAX_FAILED_PATH_COMPUTATIONS = 15;

/**
 * If we have failed path computations this many times and ComputePathToGoal is called,
 * always run a long-path, to avoid getting stuck sometimes (see D1424).
 */
static const u8 MAX_FAILED_PATH_COMPUTATIONS_BEFORE_LONG_PATH = 3;

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

	bool m_FacePointAfterMove;

	// Number of path computations that failed (in a row).
	// When this gets above MAX_FAILED_PATH_COMPUTATIONS, inform other components
	// that the move will likely fail.
	u8 m_FailedPathComputations = 0;

	// If true, PathingUpdateNeeded returns false always.
	// This exists because the goal may be unreachable to the short/long pathfinder.
	// In such cases, we would compute inacceptable paths and PathingUpdateNeeded would trigger every turn.
	// To avoid that, when we know the new path is imperfect, treat it as OK and follow it until the end.
	// When reaching the end, we'll run through HandleObstructedMove and this will be reset.
	bool m_FollowKnownImperfectPath = false;

	struct Ticket {
		u32 m_Ticket = 0; // asynchronous request ID we're waiting for, or 0 if none
		enum Type {
			SHORT_PATH,
			LONG_PATH
		} m_Type = SHORT_PATH; // Pick some default value to avoid UB.

		void clear() { m_Ticket = 0; }
	} m_ExpectedPathTicket;

	struct MoveRequest {
		enum Type {
			NONE,
			POINT,
			ENTITY,
			OFFSET
		} m_Type = NONE;
		entity_id_t m_Entity = INVALID_ENTITY;
		CFixedVector2D m_Position;
		entity_pos_t m_MinRange, m_MaxRange;

		// For readability
		CFixedVector2D GetOffset() const { return m_Position; };

		MoveRequest() = default;
		MoveRequest(CFixedVector2D pos, entity_pos_t minRange, entity_pos_t maxRange) : m_Type(POINT), m_Position(pos), m_MinRange(minRange), m_MaxRange(maxRange) {};
		MoveRequest(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange) : m_Type(ENTITY), m_Entity(target), m_MinRange(minRange), m_MaxRange(maxRange) {};
		MoveRequest(entity_id_t target, CFixedVector2D offset) : m_Type(OFFSET), m_Entity(target), m_Position(offset) {};
	} m_MoveRequest;

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

		m_DebugOverlayEnabled = false;
	}

	virtual void Deinit()
	{
	}

	template<typename S>
	void SerializeCommon(S& serialize)
	{
		serialize.StringASCII("pass class", m_PassClassName, 0, 64);

		serialize.NumberU32_Unbounded("ticket", m_ExpectedPathTicket.m_Ticket);
		SerializeU8_Enum<Ticket::Type, Ticket::Type::LONG_PATH>()(serialize, "ticket type", m_ExpectedPathTicket.m_Type);

		serialize.NumberU8("failed path computations", m_FailedPathComputations, 0, 255);
		serialize.Bool("followknownimperfectpath", m_FollowKnownImperfectPath);

		SerializeU8_Enum<MoveRequest::Type, MoveRequest::Type::OFFSET>()(serialize, "target type", m_MoveRequest.m_Type);
		serialize.NumberU32_Unbounded("target entity", m_MoveRequest.m_Entity);
		serialize.NumberFixed_Unbounded("target pos x", m_MoveRequest.m_Position.X);
		serialize.NumberFixed_Unbounded("target pos y", m_MoveRequest.m_Position.Y);
		serialize.NumberFixed_Unbounded("target min range", m_MoveRequest.m_MinRange);
		serialize.NumberFixed_Unbounded("target max range", m_MoveRequest.m_MaxRange);

		serialize.NumberFixed_Unbounded("speed multiplier", m_SpeedMultiplier);

		serialize.NumberFixed_Unbounded("current speed", m_CurSpeed);

		serialize.Bool("facePointAfterMove", m_FacePointAfterMove);

		SerializeVector<SerializeWaypoint>()(serialize, "long path", m_LongPath.m_Waypoints);
		SerializeVector<SerializeWaypoint>()(serialize, "short path", m_ShortPath.m_Waypoints);
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

	virtual bool IsMoveRequested() const
	{
		return m_MoveRequest.m_Type != MoveRequest::NONE;
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

	virtual bool MoveToPointRange(entity_pos_t x, entity_pos_t z, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return MoveTo(MoveRequest(CFixedVector2D(x, z), minRange, maxRange));
	}

	virtual bool MoveToTargetRange(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
	{
		return MoveTo(MoveRequest(target, minRange, maxRange));
	}

	virtual void MoveToFormationOffset(entity_id_t target, entity_pos_t x, entity_pos_t z)
	{
		MoveTo(MoveRequest(target, CFixedVector2D(x, z)));
	}

	virtual void FaceTowardsPoint(entity_pos_t x, entity_pos_t z);

	/**
	 * Clears the current MoveRequest - the unit will stop and no longer try and move.
	 * This should never be called from UnitMotion, since MoveToX orders are given
	 * by other components - these components should also decide when to stop.
	 */
	virtual void StopMoving()
	{
		if (m_FacePointAfterMove)
		{
			CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
			if (cmpPosition && cmpPosition->IsInWorld())
			{
				CFixedVector2D targetPos;
				if (ComputeTargetPosition(targetPos))
					FaceTowardsPointFromPos(cmpPosition->GetPosition2D(), targetPos.X, targetPos.Y);
			}
		}

		m_MoveRequest = MoveRequest();
		m_ExpectedPathTicket.clear();
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
		// TODO: this really shouldn't be what we are checking for.
		return m_MoveRequest.m_Type == MoveRequest::OFFSET;
	}

	entity_id_t GetGroup() const
	{
		return IsFormationMember() ? m_MoveRequest.m_Entity : GetEntityId();
	}

	/**
	 * Warns other components that our current movement will likely fail (e.g. we won't be able to reach our target)
	 * This should only be called before the actual movement in a given turn, or units might both move and try to do things
	 * on the same turn, leading to gliding units.
	 */
	void MoveFailed()
	{
		CMessageMotionUpdate msg(CMessageMotionUpdate::LIKELY_FAILURE);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	/**
	 * Warns other components that our current movement is likely over (i.e. we probably reached our destination)
	 * This should only be called before the actual movement in a given turn, or units might both move and try to do things
	 * on the same turn, leading to gliding units.
	 */
	void MoveSucceeded()
	{
		CMessageMotionUpdate msg(CMessageMotionUpdate::LIKELY_SUCCESS);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	/**
	 * Increment the number of failed path computations and notify other components if required.
	 */
	void IncrementFailedPathComputationAndMaybeNotify()
	{
		m_FailedPathComputations++;
		if (m_FailedPathComputations >= MAX_FAILED_PATH_COMPUTATIONS)
		{
			MoveFailed();
			m_FailedPathComputations = 0;
		}
	}

	/**
	 * If path would take us farther away from the goal than pos currently is, return false, else return true.
	 */
	bool RejectFartherPaths(const PathGoal& goal, const WaypointPath& path, const CFixedVector2D& pos) const;

	/**
	 * If there are 2 waypoints of more remaining in longPath, return SHORT_PATH_LONG_WAYPOINT_RANGE.
	 * Otherwise the pathing should be exact.
	 */
	entity_pos_t ShortPathWaypointRange(const WaypointPath& longPath) const
	{
		return longPath.m_Waypoints.size() >= 2 ? SHORT_PATH_LONG_WAYPOINT_RANGE : entity_pos_t::Zero();
	}

	bool InShortPathRange(const PathGoal& goal, const CFixedVector2D& pos) const
	{
		return goal.DistanceToPoint(pos) < LONG_PATH_MIN_DIST;
	}

	/**
	 * Handle the result of an asynchronous path query.
	 */
	void PathResult(u32 ticket, const WaypointPath& path);

	/**
	 * Do the per-turn movement and other updates.
	 */
	void Move(fixed dt);

	/**
	 * Returns true if we are possibly at our destination.
	 * Since the concept of being at destination is dependent on why the move was requested,
	 * UnitMotion can only ever hint about this, hence the conditional tone.
	 */
	bool PossiblyAtDestination() const;

	/**
	 * Process the move the unit will do this turn.
	 * This does not send actually change the position.
	 * @returns true if the move was obstructed.
	 */
	bool PerformMove(fixed dt, WaypointPath& shortPath, WaypointPath& longPath, CFixedVector2D& pos) const;

	/**
	 * Update other components on our speed.
	 * (For performance, this should try to avoid sending messages).
	 */
	void UpdateMovementState(entity_pos_t speed);

	/**
	 * React if our move was obstructed.
	 * @returns true if the obstruction required handling, false otherwise.
	 */
	bool HandleObstructedMove();

	/**
	 * Returns true if the target position is valid. False otherwise.
	 * (this may indicate that the target is e.g. out of the world/dead).
	 * NB: for code-writing convenience, if we have no target, this returns true.
	 */
	bool TargetHasValidPosition(const MoveRequest& moveRequest) const;
	bool TargetHasValidPosition() const
	{
		return TargetHasValidPosition(m_MoveRequest);
	}

	/**
	 * Computes the current location of our target entity (plus offset).
	 * Returns false if no target entity or no valid position.
	 */
	bool ComputeTargetPosition(CFixedVector2D& out, const MoveRequest& moveRequest) const;
	bool ComputeTargetPosition(CFixedVector2D& out) const
	{
		return ComputeTargetPosition(out, m_MoveRequest);
	}

	/**
	 * Attempts to replace the current path with a straight line to the target,
	 * if it's close enough and the route is not obstructed.
	 */
	bool TryGoingStraightToTarget(const CFixedVector2D& from);

	/**
	 * Returns whether our we need to recompute a path to reach our target.
	 */
	bool PathingUpdateNeeded(const CFixedVector2D& from) const;

	/**
	 * Rotate to face towards the target point, given the current pos
	 */
	void FaceTowardsPointFromPos(const CFixedVector2D& pos, entity_pos_t x, entity_pos_t z);

	/**
	 * Returns an appropriate obstruction filter for use with path requests.
	 */
	ControlGroupMovementObstructionFilter GetObstructionFilter() const;

	/**
	 * Decide whether to approximate the given range from a square target as a circle,
	 * rather than as a square.
	 */
	bool ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t circleRadius) const;

	/**
	 * Create a PathGoal from a move request.
	 * @returns true if the goal was successfully created.
	 */
	bool ComputeGoal(PathGoal& out, const MoveRequest& moveRequest) const;

	/**
	 * Compute a path to the given goal from the given position.
	 * Might go in a straight line immediately, or might start an asynchronous path request.
	 */
	void ComputePathToGoal(const CFixedVector2D& from, const PathGoal& goal);

	/**
	 * Start an asynchronous long path query.
	 */
	void RequestLongPath(const CFixedVector2D& from, const PathGoal& goal);

	/**
	 * Start an asynchronous short path query.
	 */
	void RequestShortPath(const CFixedVector2D& from, const PathGoal& goal, bool avoidMovingUnits);

	/**
	 * General handler for MoveTo interface functions.
	 */
	bool MoveTo(MoveRequest request);

	/**
	 * Convert a path into a renderable list of lines
	 */
	void RenderPath(const WaypointPath& path, std::vector<SOverlayLine>& lines, CColor color);

	void RenderSubmit(SceneCollector& collector);
};

REGISTER_COMPONENT_TYPE(UnitMotion)

bool CCmpUnitMotion::RejectFartherPaths(const PathGoal& goal, const WaypointPath& path, const CFixedVector2D& pos) const
{
	if (path.m_Waypoints.empty())
		return false;

	// Reject the new path if it does not lead us closer to the target's position.
	if (goal.DistanceToPoint(pos) <= goal.DistanceToPoint(CFixedVector2D(path.m_Waypoints.front().x, path.m_Waypoints.front().z)))
		return true;

	return false;
}

void CCmpUnitMotion::PathResult(u32 ticket, const WaypointPath& path)
{
	// Ignore obsolete path requests
	if (ticket != m_ExpectedPathTicket.m_Ticket || m_MoveRequest.m_Type == MoveRequest::NONE)
		return;

	Ticket::Type ticketType = m_ExpectedPathTicket.m_Type;
	m_ExpectedPathTicket.clear();

	// Check that we are still able to do something with that path
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
	{
		// We will probably fail to move so inform components but keep on trying anyways.
		MoveFailed();
		return;
	}

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	PathGoal goal;
	// If we can't compute a goal, we'll fail in the next Move() call so do nothing special.
	if (!ComputeGoal(goal, m_MoveRequest))
		return;

	if (ticketType == Ticket::LONG_PATH)
	{
		if (RejectFartherPaths(goal, path, pos))
		{
			IncrementFailedPathComputationAndMaybeNotify();
			return;
		}

		m_LongPath = path;

		m_FollowKnownImperfectPath = false;

		// If there's no waypoints then we couldn't get near the target.
		// Sort of hack: Just try going directly to the goal point instead
		// (via the short pathfinder over the next turns), so if we're stuck and the user clicks
		// close enough to the unit then we can probably get unstuck
		// NB: this relies on HandleObstructedMove requesting short paths if we still have long waypoints.
		if (m_LongPath.m_Waypoints.empty())
		{
			IncrementFailedPathComputationAndMaybeNotify();
			CFixedVector2D targetPos;
			if (ComputeTargetPosition(targetPos))
				m_LongPath.m_Waypoints.emplace_back(Waypoint{ targetPos.X, targetPos.Y });
		}
		// If this new path won't put us in range, it's highly likely that we are going somewhere unreachable.
		// This means we will try to recompute the path every turn.
		// To avoid this, act as if our current path leads us to the correct destination.
		// (we will still fail the move when we arrive to the best possible position, and if we were blocked by
		// an obstruction and it goes away we will notice when getting there as having no waypoint goes through
		// HandleObstructedMove, so this is safe).
		// TODO: For now, we won't warn components straight away as that could lead to units idling earlier than expected,
		// but it should be done someday when the message can differentiate between different failure causes.
		else if (PathingUpdateNeeded(pos))
			m_FollowKnownImperfectPath = true;
		return;
	}

	// Reject new short paths if they were aiming at the goal directly (i.e. no long waypoints still exists).
	if (m_LongPath.m_Waypoints.empty() && RejectFartherPaths(goal, path, pos))
	{
		IncrementFailedPathComputationAndMaybeNotify();
		return;
	}

	m_ShortPath = path;

	m_FollowKnownImperfectPath = false;
	if (!m_ShortPath.m_Waypoints.empty())
	{
		if (PathingUpdateNeeded(pos))
			m_FollowKnownImperfectPath = true;
		return;
	}

	if (m_FailedPathComputations >= 1)
	{
		// Inform other components - we might be ordered to stop, and computeGoal will then fail and return early.
		CMessageMotionUpdate msg(CMessageMotionUpdate::OBSTRUCTED);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	// Don't notify if we are a formation member - we can occasionally be stuck for a long time
	// if our current offset is unreachable.
	if (!IsFormationMember())
		IncrementFailedPathComputationAndMaybeNotify();

	// If there's no waypoints then we couldn't get near the target
	// If we're globally following a long path, try to remove the next waypoint,
	// it might be obstructed (e.g. by idle entities which the long-range pathfinder doesn't see).
	if (!m_LongPath.m_Waypoints.empty())
	{
		m_LongPath.m_Waypoints.pop_back();
		if (!m_LongPath.m_Waypoints.empty())
		{
			// Get close enough - this will likely help the short path efficiency, and if we end up taking a wrong way
			// we'll easily be able to revert it using a long path.
			PathGoal goal = { PathGoal::CIRCLE, m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z, ShortPathWaypointRange(m_LongPath) };
			RequestShortPath(pos, goal, true);
			return;
		}
	}

	ComputePathToGoal(pos, goal);
}

void CCmpUnitMotion::Move(fixed dt)
{
	PROFILE("Move");

	// If we were idle and will still be, we can return.
	// TODO: this will need to be removed if pushing is implemented.
	if (m_CurSpeed == fixed::Zero() && m_MoveRequest.m_Type == MoveRequest::NONE)
		return;

	if (PossiblyAtDestination())
		MoveSucceeded();
	else if (!TargetHasValidPosition())
	{
		// Scrap waypoints - we don't know where to go.
		// If the move request remains unchanged and the target again has a valid position later on,
		// moving will be resumed.
		// Units may want to move to move to the target's last known position,
		// but that should be decided by UnitAI (handling MoveFailed), not UnitMotion.
		m_LongPath.m_Waypoints.clear();
		m_ShortPath.m_Waypoints.clear();

		MoveFailed();
	}

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;

	CFixedVector2D initialPos = cmpPosition->GetPosition2D();

	// Keep track of the current unit's position during the update
	CFixedVector2D pos = initialPos;

	// If we're chasing a potentially-moving unit and are currently close
	// enough to its current position, and we can head in a straight line
	// to it, then throw away our current path and go straight to it
	bool wentStraight = TryGoingStraightToTarget(initialPos);

	bool wasObstructed = PerformMove(dt, m_ShortPath, m_LongPath, pos);

	// Update our speed over this turn so that the visual actor shows the correct animation.
	if (pos == initialPos)
		UpdateMovementState(fixed::Zero());
	else
	{
		// Update the Position component after our movement (if we actually moved anywhere)
		CFixedVector2D offset = pos - initialPos;

		// Face towards the target
		entity_angle_t angle = atan2_approx(offset.X, offset.Y);
		cmpPosition->MoveAndTurnTo(pos.X,pos.Y, angle);

		// Calculate the mean speed over this past turn.
		UpdateMovementState(offset.Length() / dt);
	}

	if (wasObstructed && HandleObstructedMove())
		return;
	else if (!wasObstructed)
		m_FailedPathComputations = 0;

	// We may need to recompute our path sometimes (e.g. if our target moves).
	// Since we request paths asynchronously anyways, this does not need to be done before moving.
	if (!wentStraight && PathingUpdateNeeded(pos))
	{
		PathGoal goal;
		if (ComputeGoal(goal, m_MoveRequest))
			ComputePathToGoal(pos, goal);
	}
}

bool CCmpUnitMotion::PossiblyAtDestination() const
{
	if (m_MoveRequest.m_Type == MoveRequest::NONE)
		return false;

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	ENSURE(cmpObstructionManager);

	if (m_MoveRequest.m_Type == MoveRequest::POINT)
		return cmpObstructionManager->IsInPointRange(GetEntityId(), m_MoveRequest.m_Position.X, m_MoveRequest.m_Position.Y, m_MoveRequest.m_MinRange, m_MoveRequest.m_MaxRange, false);
	if (m_MoveRequest.m_Type == MoveRequest::ENTITY)
		return cmpObstructionManager->IsInTargetRange(GetEntityId(), m_MoveRequest.m_Entity, m_MoveRequest.m_MinRange, m_MoveRequest.m_MaxRange, false);
	if (m_MoveRequest.m_Type == MoveRequest::OFFSET)
	{
		CmpPtr<ICmpUnitMotion> cmpControllerMotion(GetSimContext(), m_MoveRequest.m_Entity);
		if (cmpControllerMotion && cmpControllerMotion->IsMoveRequested())
			return false;

		CFixedVector2D targetPos;
		ComputeTargetPosition(targetPos);
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		return cmpObstructionManager->IsInPointRange(GetEntityId(), targetPos.X, targetPos.Y, m_MoveRequest.m_MinRange, m_MoveRequest.m_MaxRange, false);
	}
	return false;
}

bool CCmpUnitMotion::PerformMove(fixed dt, WaypointPath& shortPath, WaypointPath& longPath, CFixedVector2D& pos) const
{
	// If there are no waypoint, behave as though we were obstructed and let HandleObstructedMove handle it.
	if (shortPath.m_Waypoints.empty() && longPath.m_Waypoints.empty())
		return true;

	// TODO: there's some asymmetry here when units look at other
	// units' positions - the result will depend on the order of execution.
	// Maybe we should split the updates into multiple phases to minimise
	// that problem.

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	ENSURE(cmpPathfinder);

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

	// We want to move (at most) maxSpeed*dt units from pos towards the next waypoint

	fixed timeLeft = dt;
	fixed zero = fixed::Zero();

	while (timeLeft > zero)
	{
		// If we ran out of path, we have to stop
		if (shortPath.m_Waypoints.empty() && longPath.m_Waypoints.empty())
			break;

		CFixedVector2D target;
		if (shortPath.m_Waypoints.empty())
			target = CFixedVector2D(longPath.m_Waypoints.back().x, longPath.m_Waypoints.back().z);
		else
			target = CFixedVector2D(shortPath.m_Waypoints.back().x, shortPath.m_Waypoints.back().z);

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

				if (shortPath.m_Waypoints.empty())
					longPath.m_Waypoints.pop_back();
				else
					shortPath.m_Waypoints.pop_back();

				continue;
			}
			else
			{
				// Error - path was obstructed
				return true;
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
				return true;

			break;
		}
	}
	return false;
}

void CCmpUnitMotion::UpdateMovementState(entity_pos_t speed)
{
	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	CmpPtr<ICmpVisual> cmpVisual(GetEntityHandle());
	// Moved last turn, didn't this turn.
	if (speed == fixed::Zero() && m_CurSpeed > fixed::Zero())
	{
		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(false);
		if (cmpVisual)
			cmpVisual->SelectMovementAnimation("idle", fixed::FromInt(1));
	}
	// Moved this turn, didn't last turn
	else if (speed > fixed::Zero() && m_CurSpeed == fixed::Zero())
	{
		if (cmpObstruction)
			cmpObstruction->SetMovingFlag(true);
		if (cmpVisual)
			cmpVisual->SelectMovementAnimation(m_Speed > m_WalkSpeed ? "run" : "walk", m_Speed);
	}
	// Speed change, update the visual actor if necessary.
	else if (speed != m_CurSpeed && cmpVisual)
		cmpVisual->SelectMovementAnimation(m_Speed > m_WalkSpeed ? "run" : "walk", m_Speed);

	m_CurSpeed = speed;
}

bool CCmpUnitMotion::HandleObstructedMove()
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	if (m_FailedPathComputations >= 1)
	{
		// Inform other components - we might be ordered to stop, and computeGoal will then fail and return early.
		CMessageMotionUpdate msg(CMessageMotionUpdate::OBSTRUCTED);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	// Oops, we hit something (very likely another unit).

	PathGoal goal;
	if (!ComputeGoal(goal, m_MoveRequest))
		return false;

	if (!InShortPathRange(goal, pos))
	{
		// If we still have long waypoints, try and compute a short path.
		// Assume the next waypoint is impassable
		if (m_LongPath.m_Waypoints.size() > 1)
			m_LongPath.m_Waypoints.pop_back();
		if (!m_LongPath.m_Waypoints.empty())
		{
			// Get close enough - this will likely help the short path efficiency, and if we end up taking a wrong way
			// we'll easily be able to revert it using a long path.
			PathGoal goal = { PathGoal::CIRCLE, m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z, ShortPathWaypointRange(m_LongPath) };
			RequestShortPath(pos, goal, true);
			return true;
		}
	}

	// Else, just entirely recompute. This will ensure we occasionally run a long path so avoid getting stuck
	// in the short pathfinder, which can happen when an entity is right ober an obstruction's edge.
	ComputePathToGoal(pos, goal);

	// potential TODO: We could switch the short-range pathfinder for something else entirely.
	return true;
}

bool CCmpUnitMotion::TargetHasValidPosition(const MoveRequest& moveRequest) const
{
	if (moveRequest.m_Type != MoveRequest::ENTITY)
		return true;

	CmpPtr<ICmpPosition> cmpPosition(GetSimContext(), moveRequest.m_Entity);
	return cmpPosition && cmpPosition->IsInWorld();
}

bool CCmpUnitMotion::ComputeTargetPosition(CFixedVector2D& out, const MoveRequest& moveRequest) const
{
	if (moveRequest.m_Type == MoveRequest::POINT)
	{
		out = moveRequest.m_Position;
		return true;
	}

	CmpPtr<ICmpPosition> cmpTargetPosition(GetSimContext(), moveRequest.m_Entity);
	if (!cmpTargetPosition || !cmpTargetPosition->IsInWorld())
		return false;

	if (moveRequest.m_Type == MoveRequest::OFFSET)
	{
		// There is an offset, so compute it relative to orientation
		entity_angle_t angle = cmpTargetPosition->GetRotation().Y;
		CFixedVector2D offset = moveRequest.GetOffset().Rotate(angle);
		out = cmpTargetPosition->GetPosition2D() + offset;
	}
	else
	{
		out = cmpTargetPosition->GetPosition2D();
		// If the target is moving, we might never get in range if we just try to reach its current position,
		// so we have to try and move to a position where we will be in-range, including their movement.
		// Since we request paths asynchronously a the end of our turn and the order in which two units move is uncertain,
		// we need to account for twice the movement speed to be sure that we're targeting the correct point.
		// TODO: be cleverer about this. It fixes fleeing nicely currently, but orthogonal movement should be considered,
		// and the overall logic could be improved upon.
		CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), moveRequest.m_Entity);
		if (cmpUnitMotion && cmpUnitMotion->IsMoveRequested())
		{
			CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
			if (!cmpPosition || !cmpPosition->IsInWorld())
				return true; // Still return true since we don't need a position for the target to have one.

			CFixedVector2D tempPos = out + (out - cmpTargetPosition->GetPreviousPosition2D()) * 2;

			// Check if we anticipate the target to go through us, in which case we shouldn't anticipate
			// (or e.g. units fleeing might suddenly turn around towards their attacker).
			if ((out - cmpPosition->GetPosition2D()).Dot(tempPos - cmpPosition->GetPosition2D()) >= fixed::Zero())
				out = tempPos;
		}
	}
	return true;
}

bool CCmpUnitMotion::TryGoingStraightToTarget(const CFixedVector2D& from)
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
	PathGoal goal;
	if (!ComputeGoal(goal, m_MoveRequest))
		return false;
	goal.x = targetPos.X;
	goal.z = targetPos.Y;
	// (we ignore changes to the target's rotation, since only buildings are
	// square and buildings don't move)

	// Find the point on the goal shape that we should head towards
	CFixedVector2D goalPos = goal.NearestPointOnGoal(from);

	// Check if there's any collisions on that route.
	// For entity goals, skip only the specific obstruction tag or with e.g. walls we might ignore too many entities.
	ICmpObstructionManager::tag_t specificIgnore;
	if (m_MoveRequest.m_Type == MoveRequest::ENTITY)
	{
		CmpPtr<ICmpObstruction> cmpTargetObstruction(GetSimContext(), m_MoveRequest.m_Entity);
		if (cmpTargetObstruction)
			specificIgnore = cmpTargetObstruction->GetObstruction();
	}

	if (specificIgnore.valid())
	{
		if (!cmpPathfinder->CheckMovement(SkipTagObstructionFilter(specificIgnore), from.X, from.Y, goalPos.X, goalPos.Y, m_Clearance, m_PassClass))
			return false;
	}
	else if (!cmpPathfinder->CheckMovement(GetObstructionFilter(), from.X, from.Y, goalPos.X, goalPos.Y, m_Clearance, m_PassClass))
		return false;


	// That route is okay, so update our path
	m_LongPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.clear();
	m_ShortPath.m_Waypoints.emplace_back(Waypoint{ goalPos.X, goalPos.Y });
	return true;
}

bool CCmpUnitMotion::PathingUpdateNeeded(const CFixedVector2D& from) const
{
	if (m_MoveRequest.m_Type == MoveRequest::NONE)
		return false;

	CFixedVector2D targetPos;
	if (!ComputeTargetPosition(targetPos))
		return false;

	if (m_FollowKnownImperfectPath)
		return false;

	if (PossiblyAtDestination())
		return false;

	// Get the obstruction shape and translate it where we estimate the target to be.
	ICmpObstructionManager::ObstructionSquare estimatedTargetShape;
	if (m_MoveRequest.m_Type == MoveRequest::ENTITY)
	{
		CmpPtr<ICmpObstruction> cmpTargetObstruction(GetSimContext(), m_MoveRequest.m_Entity);
		if (cmpTargetObstruction)
			cmpTargetObstruction->GetObstructionSquare(estimatedTargetShape);
	}

	estimatedTargetShape.x = targetPos.X;
	estimatedTargetShape.z = targetPos.Y;

	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	ICmpObstructionManager::ObstructionSquare shape;
	if (cmpObstruction)
		cmpObstruction->GetObstructionSquare(shape);

	// Translate our own obstruction shape to our last waypoint or our current position, lacking that.
	if (m_LongPath.m_Waypoints.empty() && m_ShortPath.m_Waypoints.empty())
	{
		shape.x = from.X;
		shape.z = from.Y;
	}
	else
	{
		const Waypoint& lastWaypoint = m_LongPath.m_Waypoints.empty() ? m_ShortPath.m_Waypoints.front() : m_LongPath.m_Waypoints.front();
		shape.x = lastWaypoint.x;
		shape.z = lastWaypoint.z;
	}

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	ENSURE(cmpObstructionManager);

	// Increase the ranges with distance, to avoid recomputing every turn against units that are moving and far-away for example.
	entity_pos_t distance = (from - CFixedVector2D(estimatedTargetShape.x, estimatedTargetShape.z)).Length();
	// When in straight-path distance, we want perfect detection.
	distance = std::max(distance - DIRECT_PATH_RANGE, entity_pos_t::Zero());

	// TODO: it could be worth computing this based on time to collision instead of linear distance.
	entity_pos_t minRange = std::max(m_MoveRequest.m_MinRange - distance / TARGET_UNCERTAINTY_MULTIPLIER, entity_pos_t::Zero());
	entity_pos_t maxRange = m_MoveRequest.m_MaxRange < entity_pos_t::Zero() ? m_MoveRequest.m_MaxRange :
	    m_MoveRequest.m_MaxRange + distance / TARGET_UNCERTAINTY_MULTIPLIER;

	if (cmpObstructionManager->AreShapesInRange(shape, estimatedTargetShape, minRange, maxRange, false))
		return false;

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

ControlGroupMovementObstructionFilter CCmpUnitMotion::GetObstructionFilter() const
{
	return ControlGroupMovementObstructionFilter(ShouldAvoidMovingUnits(), GetGroup());
}

// The pathfinder cannot go to "rounded rectangles" goals, which are what happens with square targets and a non-null range.
// Depending on what the best approximation is, we either pretend the target is a circle or a square.
// One needs to be careful that the approximated geometry will be in the range.
bool CCmpUnitMotion::ShouldTreatTargetAsCircle(entity_pos_t range, entity_pos_t circleRadius) const
{
	// Given a square, plus a target range we should reach, the shape at that distance
	// is a round-cornered square which we can approximate as either a circle or as a square.
	// Previously, we used the shape that minimized the worst-case error.
	// However that is unsage in some situations. So let's be less clever and
	// just check if our range is at least three times bigger than the circleradius
	return (range > circleRadius*3);
}

bool CCmpUnitMotion::ComputeGoal(PathGoal& out, const MoveRequest& moveRequest) const
{
	if (moveRequest.m_Type == MoveRequest::NONE)
		return false;

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	CFixedVector2D pos = cmpPosition->GetPosition2D();

	CFixedVector2D targetPosition;
	if (!ComputeTargetPosition(targetPosition, moveRequest))
		return false;

	ICmpObstructionManager::ObstructionSquare targetObstruction;
	if (moveRequest.m_Type == MoveRequest::ENTITY)
	{
		CmpPtr<ICmpObstruction> cmpTargetObstruction(GetSimContext(), moveRequest.m_Entity);
		if (cmpTargetObstruction)
			cmpTargetObstruction->GetObstructionSquare(targetObstruction);
	}
	targetObstruction.x = targetPosition.X;
	targetObstruction.z = targetPosition.Y;

	ICmpObstructionManager::ObstructionSquare obstruction;
	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	if (cmpObstruction)
		cmpObstruction->GetObstructionSquare(obstruction);
	else
	{
		obstruction.x = pos.X;
		obstruction.z = pos.Y;
	}

	CmpPtr<ICmpObstructionManager> cmpObstructionManager(GetSystemEntity());
	ENSURE(cmpObstructionManager);

	entity_pos_t distance = cmpObstructionManager->DistanceBetweenShapes(obstruction, targetObstruction);

	out.x = targetObstruction.x;
	out.z = targetObstruction.z;
	out.hw = targetObstruction.hw;
	out.hh = targetObstruction.hh;
	out.u = targetObstruction.u;
	out.v = targetObstruction.v;

	if (moveRequest.m_MinRange > fixed::Zero() || moveRequest.m_MaxRange > fixed::Zero() ||
	     targetObstruction.hw > fixed::Zero())
		out.type = PathGoal::SQUARE;
	else
	{
		out.type = PathGoal::POINT;
		return true;
	}

	entity_pos_t circleRadius = CFixedVector2D(targetObstruction.hw, targetObstruction.hh).Length();

	// TODO: because we cannot move to rounded rectangles, we have to make conservative approximations.
	// This means we might end up in a situation where cons(max-range) < min range < max range < cons(min-range)
	// When going outside of the min-range or inside the max-range, the unit will still go through the correct range
	// but if it moves fast enough, this might not be picked up by PossiblyAtDestination().
	// Fixing this involves moving to rounded rectangles, or checking more often in PerformMove().
	// In the meantime, one should avoid that 'Speed over a turn' > MaxRange - MinRange, in case where
	// min-range is not 0 and max-range is not infinity.
	if (distance < moveRequest.m_MinRange)
	{
		// Distance checks are nearest edge to nearest edge, so we need to account for our clearance
		// and we must make sure diagonals also fit so multiply by slightly more than sqrt(2)
		entity_pos_t goalDistance = moveRequest.m_MinRange + m_Clearance * 3 / 2;

		if (ShouldTreatTargetAsCircle(moveRequest.m_MinRange, circleRadius))
		{
			// We are safely away from the obstruction itself if we are away from the circumscribing circle
			out.type = PathGoal::INVERTED_CIRCLE;
			out.hw = circleRadius + goalDistance;
		}
		else
		{
			out.type = PathGoal::INVERTED_SQUARE;
			out.hw = targetObstruction.hw + goalDistance;
			out.hh = targetObstruction.hh + goalDistance;
		}
	}
	else if (moveRequest.m_MaxRange >= fixed::Zero() && distance > moveRequest.m_MaxRange)
	{
		if (ShouldTreatTargetAsCircle(moveRequest.m_MaxRange, circleRadius))
		{
			entity_pos_t goalDistance = moveRequest.m_MaxRange;
			// We must go in-range of the inscribed circle, not the circumscribing circle.
			circleRadius = std::min(targetObstruction.hw, targetObstruction.hh);

			out.type = PathGoal::CIRCLE;
			out.hw = circleRadius + goalDistance;
		}
		else
		{
			// The target is large relative to our range, so treat it as a square and
			// get close enough that the diagonals come within range

			entity_pos_t goalDistance = moveRequest.m_MaxRange * 2 / 3; // multiply by slightly less than 1/sqrt(2)

			out.type = PathGoal::SQUARE;
			entity_pos_t delta = std::max(goalDistance, m_Clearance + entity_pos_t::FromInt(TERRAIN_TILE_SIZE)/16); // ensure it's far enough to not intersect the building itself
			out.hw = targetObstruction.hw + delta;
			out.hh = targetObstruction.hh + delta;
		}
	}
	// Do nothing in particular in case we are already in range.
	return true;
}

void CCmpUnitMotion::ComputePathToGoal(const CFixedVector2D& from, const PathGoal& goal)
{
#if DISABLE_PATHFINDER
	{
		CmpPtr<ICmpPathfinder> cmpPathfinder (GetSimContext(), SYSTEM_ENTITY);
		CFixedVector2D goalPos = m_FinalGoal.NearestPointOnGoal(from);
		m_LongPath.m_Waypoints.clear();
		m_ShortPath.m_Waypoints.clear();
		m_ShortPath.m_Waypoints.emplace_back(Waypoint{ goalPos.X, goalPos.Y });
		return;
	}
#endif

	// If the target is close and we can reach it in a straight line,
	// then we'll just go along the straight line instead of computing a path.
	if (m_FailedPathComputations != MAX_FAILED_PATH_COMPUTATIONS_BEFORE_LONG_PATH && TryGoingStraightToTarget(from))
		return;

	// Otherwise we need to compute a path.

	// If it's close then just do a short path, not a long path
	// TODO: If it's close on the opposite side of a river then we really
	// need a long path, so we shouldn't simply check linear distance
	// the check is arbitrary but should be a reasonably small distance.
	// To avoid getting stuck because the short-range pathfinder is bounded, occasionally compute a long path instead.
	if (m_FailedPathComputations != MAX_FAILED_PATH_COMPUTATIONS_BEFORE_LONG_PATH && InShortPathRange(goal, from))
	{
		m_LongPath.m_Waypoints.clear();
		RequestShortPath(from, goal, true);
	}
	else
	{
		if (m_FailedPathComputations == MAX_FAILED_PATH_COMPUTATIONS_BEFORE_LONG_PATH)
			m_FailedPathComputations++; // This makes sure we don't end up stuck in this special state which can break pathing.
		m_ShortPath.m_Waypoints.clear();
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

	m_ExpectedPathTicket.m_Type = Ticket::LONG_PATH;
	m_ExpectedPathTicket.m_Ticket = cmpPathfinder->ComputePathAsync(from.X, from.Y, improvedGoal, m_PassClass, GetEntityId());
}

void CCmpUnitMotion::RequestShortPath(const CFixedVector2D &from, const PathGoal& goal, bool avoidMovingUnits)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	fixed searchRange = SHORT_PATH_MIN_SEARCH_RANGE + SHORT_PATH_SEARCH_RANGE_INCREMENT * m_FailedPathComputations;
	if (searchRange > SHORT_PATH_MAX_SEARCH_RANGE)
		searchRange = SHORT_PATH_MAX_SEARCH_RANGE;

	m_ExpectedPathTicket.m_Type = Ticket::SHORT_PATH;
	m_ExpectedPathTicket.m_Ticket = cmpPathfinder->ComputeShortPathAsync(from.X, from.Y, m_Clearance, searchRange, goal, m_PassClass, avoidMovingUnits, GetGroup(), GetEntityId());
}

bool CCmpUnitMotion::MoveTo(MoveRequest request)
{
	PROFILE("MoveTo");

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	PathGoal goal;
	if (!ComputeGoal(goal, request))
		return false;

	m_MoveRequest = request;
	m_FailedPathComputations = 0;
	m_FollowKnownImperfectPath = false;

	ComputePathToGoal(cmpPosition->GetPosition2D(), goal);
	return true;
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
