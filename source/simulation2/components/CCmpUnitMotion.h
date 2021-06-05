/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_CCMPUNITMOTION
#define INCLUDED_CCMPUNITMOTION

#include "simulation2/system/Component.h"
#include "ICmpUnitMotion.h"

#include "simulation2/components/CCmpUnitMotionManager.h"
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
#include "simulation2/serialization/SerializedPathfinder.h"
#include "simulation2/serialization/SerializedTypes.h"

#include "graphics/Overlay.h"
#include "maths/FixedVector2D.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "renderer/Scene.h"

// NB: this implementation of ICmpUnitMotion is very tightly coupled with UnitMotionManager.
// As such, both are compiled in the same TU.

// For debugging; units will start going straight to the target
// instead of calling the pathfinder
#define DISABLE_PATHFINDER 0

namespace
{
/**
 * Min/Max range to restrict short path queries to. (Larger ranges are (much) slower,
 * smaller ranges might miss some legitimate routes around large obstacles.)
 * NB: keep the max-range in sync with the vertex pathfinder "move the search space" heuristic.
 */
constexpr entity_pos_t SHORT_PATH_MIN_SEARCH_RANGE = entity_pos_t::FromInt(12 * Pathfinding::NAVCELL_SIZE_INT);
constexpr entity_pos_t SHORT_PATH_MAX_SEARCH_RANGE = entity_pos_t::FromInt(56 * Pathfinding::NAVCELL_SIZE_INT);
constexpr entity_pos_t SHORT_PATH_SEARCH_RANGE_INCREMENT = entity_pos_t::FromInt(4 * Pathfinding::NAVCELL_SIZE_INT);
constexpr u8 SHORT_PATH_SEARCH_RANGE_INCREASE_DELAY = 1;

/**
 * When using the short-pathfinder to rejoin a long-path waypoint, aim for a circle of this radius around the waypoint.
 */
constexpr entity_pos_t SHORT_PATH_LONG_WAYPOINT_RANGE = entity_pos_t::FromInt(4 * Pathfinding::NAVCELL_SIZE_INT);

/**
 * Minimum distance to goal for a long path request
 */
constexpr entity_pos_t LONG_PATH_MIN_DIST = entity_pos_t::FromInt(16 * Pathfinding::NAVCELL_SIZE_INT);

/**
 * If we are this close to our target entity/point, then think about heading
 * for it in a straight line instead of pathfinding.
 */
constexpr entity_pos_t DIRECT_PATH_RANGE = entity_pos_t::FromInt(24 * Pathfinding::NAVCELL_SIZE_INT);

/**
 * To avoid recomputing paths too often, have some leeway for target range checks
 * based on our distance to the target. Increase that incertainty by one navcell
 * for every this many tiles of distance.
 */
constexpr entity_pos_t TARGET_UNCERTAINTY_MULTIPLIER = entity_pos_t::FromInt(8 * Pathfinding::NAVCELL_SIZE_INT);

/**
 * When following a known imperfect path (i.e. a path that won't take us in range of our goal
 * we still recompute a new path every N turn to adapt to moving targets (for example, ships that must pickup
 * units may easily end up in this state, they still need to adjust to moving units).
 * This is rather arbitrary and mostly for simplicity & optimisation (a better recomputing algorithm
 * would not need this).
 */
constexpr u8 KNOWN_IMPERFECT_PATH_RESET_COUNTDOWN = 12;

/**
 * When we fail to move this many turns in a row, inform other components that the move will fail.
 * Experimentally, this number needs to be somewhat high or moving groups of units will lead to stuck units.
 * However, too high means units will look idle for a long time when they are failing to move.
 * TODO: if UnitMotion could send differentiated "unreachable" and "currently stuck" failing messages,
 * this could probably be lowered.
 * TODO: when unit pushing is implemented, this number can probably be lowered.
 */
constexpr u8 MAX_FAILED_MOVEMENTS = 35;

/**
 * When computing paths but failing to move, we want to occasionally alternate pathfinder systems
 * to avoid getting stuck (the short pathfinder can unstuck the long-range one and vice-versa, depending).
 */
constexpr u8 ALTERNATE_PATH_TYPE_DELAY = 3;
constexpr u8 ALTERNATE_PATH_TYPE_EVERY = 6;

/**
 * After this many failed computations, start sending "VERY_OBSTRUCTED" messages instead.
 * Should probably be larger than ALTERNATE_PATH_TYPE_DELAY.
 */
constexpr u8 VERY_OBSTRUCTED_THRESHOLD = 10;

const CColor OVERLAY_COLOR_LONG_PATH(1, 1, 1, 1);
const CColor OVERLAY_COLOR_SHORT_PATH(1, 0, 0, 1);
} // anonymous namespace

class CCmpUnitMotion final : public ICmpUnitMotion
{
	friend class CCmpUnitMotionManager;
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeToMessageType(MT_Create);
		componentManager.SubscribeToMessageType(MT_Destroy);
		componentManager.SubscribeToMessageType(MT_PathResult);
		componentManager.SubscribeToMessageType(MT_OwnershipChanged);
		componentManager.SubscribeToMessageType(MT_ValueModification);
		componentManager.SubscribeToMessageType(MT_MovementObstructionChanged);
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

	// Whether the unit participates in pushing.
	bool m_Pushing = false;

	// Whether the unit blocks movement (& is blocked by movement blockers)
	// Cached from ICmpObstruction.
	bool m_BlockMovement = false;

	// Internal counter used when recovering from obstructed movement.
	// Most notably, increases the search range of the vertex pathfinder.
	// See HandleObstructedMove() for more details.
	u8 m_FailedMovements = 0;

	// If > 0, PathingUpdateNeeded returns false always.
	// This exists because the goal may be unreachable to the short/long pathfinder.
	// In such cases, we would compute inacceptable paths and PathingUpdateNeeded would trigger every turn,
	// which would be quite bad for performance.
	// To avoid that, when we know the new path is imperfect, treat it as OK and follow it anyways.
	// When reaching the end, we'll go through HandleObstructedMove and reset regardless.
	// To still recompute now and then (the target may be moving), this is a countdown decremented on each frame.
	u8 m_FollowKnownImperfectPathCountdown = 0;

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
			"</element>"
			"<optional>"
				"<element name='DisablePushing'>"
					"<data type='boolean'/>"
				"</element>"
			"</optional>";
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
			m_PassClassName = paramNode.GetChild("PassabilityClass").ToString();
			m_PassClass = cmpPathfinder->GetPassabilityClass(m_PassClassName);
			m_Clearance = cmpPathfinder->GetClearance(m_PassClass);

			CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
			if (cmpObstruction)
			{
				cmpObstruction->SetUnitClearance(m_Clearance);
				m_BlockMovement = cmpObstruction->GetBlockMovementFlag(true);
			}
		}

		SetParticipateInPushing(!paramNode.GetChild("DisablePushing").IsOk() || !paramNode.GetChild("DisablePushing").ToBool());

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
		Serializer(serialize, "ticket type", m_ExpectedPathTicket.m_Type, Ticket::Type::LONG_PATH);

		serialize.NumberU8_Unbounded("failed movements", m_FailedMovements);
		serialize.NumberU8_Unbounded("followknownimperfectpath", m_FollowKnownImperfectPathCountdown);

		Serializer(serialize, "target type", m_MoveRequest.m_Type, MoveRequest::Type::OFFSET);
		serialize.NumberU32_Unbounded("target entity", m_MoveRequest.m_Entity);
		serialize.NumberFixed_Unbounded("target pos x", m_MoveRequest.m_Position.X);
		serialize.NumberFixed_Unbounded("target pos y", m_MoveRequest.m_Position.Y);
		serialize.NumberFixed_Unbounded("target min range", m_MoveRequest.m_MinRange);
		serialize.NumberFixed_Unbounded("target max range", m_MoveRequest.m_MaxRange);

		serialize.NumberFixed_Unbounded("speed multiplier", m_SpeedMultiplier);

		serialize.NumberFixed_Unbounded("current speed", m_CurSpeed);

		serialize.Bool("facePointAfterMove", m_FacePointAfterMove);
		serialize.Bool("pushing", m_Pushing);

		Serializer(serialize, "long path", m_LongPath.m_Waypoints);
		Serializer(serialize, "short path", m_ShortPath.m_Waypoints);
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

		CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
		if (cmpObstruction)
			m_BlockMovement = cmpObstruction->GetBlockMovementFlag(false);
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
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
		case MT_Create:
		{
			if (!ENTITY_IS_LOCAL(GetEntityId()))
				CmpPtr<ICmpUnitMotionManager>(GetSystemEntity())->Register(this, GetEntityId(), m_FormationController);
			break;
		}
		case MT_Destroy:
		{
			if (!ENTITY_IS_LOCAL(GetEntityId()))
				CmpPtr<ICmpUnitMotionManager>(GetSystemEntity())->Unregister(GetEntityId());
			break;
		}
		case MT_MovementObstructionChanged:
		{
			CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
			if (cmpObstruction)
				m_BlockMovement = cmpObstruction->GetBlockMovementFlag(false);
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
		{
			OnValueModification();
			break;
		}
		case MT_Deserialized:
		{
			OnValueModification();
			if (!ENTITY_IS_LOCAL(GetEntityId()))
				CmpPtr<ICmpUnitMotionManager>(GetSystemEntity())->Register(this, GetEntityId(), m_FormationController);
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

	virtual CFixedVector2D EstimateFuturePosition(const fixed dt) const
	{
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		if (!cmpPosition || !cmpPosition->IsInWorld())
			return CFixedVector2D();

		// TODO: formation members should perhaps try to use the controller's position.

		CFixedVector2D pos = cmpPosition->GetPosition2D();
		entity_angle_t angle = cmpPosition->GetRotation().Y;

		// Copy the path so we don't change it.
		WaypointPath shortPath = m_ShortPath;
		WaypointPath longPath = m_LongPath;

		PerformMove(dt, cmpPosition->GetTurnRate(), shortPath, longPath, pos, angle);
		return pos;
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

	virtual bool GetFacePointAfterMove() const
	{
		return m_FacePointAfterMove;
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

	virtual bool IsTargetRangeReachable(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange);

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
	bool IsFormationMember() const
	{
		// TODO: this really shouldn't be what we are checking for.
		return m_MoveRequest.m_Type == MoveRequest::OFFSET;
	}

	bool IsFormationControllerMoving() const
	{
		CmpPtr<ICmpUnitMotion> cmpControllerMotion(GetSimContext(), m_MoveRequest.m_Entity);
		return cmpControllerMotion && cmpControllerMotion->IsMoveRequested();
	}

	entity_id_t GetGroup() const
	{
		return IsFormationMember() ? m_MoveRequest.m_Entity : GetEntityId();
	}

	void SetParticipateInPushing(bool pushing)
	{
		CmpPtr<ICmpUnitMotionManager> cmpUnitMotionManager(GetSystemEntity());
		m_Pushing = pushing && cmpUnitMotionManager->IsPushingActivated();
	}

	/**
	 * Warns other components that our current movement will likely fail (e.g. we won't be able to reach our target)
	 * This should only be called before the actual movement in a given turn, or units might both move and try to do things
	 * on the same turn, leading to gliding units.
	 */
	void MoveFailed()
	{
		// Don't notify if we are a formation member in a moving formation - we can occasionally be stuck for a long time
		// if our current offset is unreachable, but we don't want to end up stuck.
		// (If the formation controller has stopped moving however, we can safely message).
		if (IsFormationMember() && IsFormationControllerMoving())
			return;

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
		// Don't notify if we are a formation member in a moving formation - we can occasionally be stuck for a long time
		// if our current offset is unreachable, but we don't want to end up stuck.
		// (If the formation controller has stopped moving however, we can safely message).
		if (IsFormationMember() && IsFormationControllerMoving())
			return;

		CMessageMotionUpdate msg(CMessageMotionUpdate::LIKELY_SUCCESS);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	/**
	 * Warns other components that our current movement was obstructed (i.e. we failed to move this turn).
	 * This should only be called before the actual movement in a given turn, or units might both move and try to do things
	 * on the same turn, leading to gliding units.
	 */
	void MoveObstructed()
	{
		// Don't notify if we are a formation member in a moving formation - we can occasionally be stuck for a long time
		// if our current offset is unreachable, but we don't want to end up stuck.
		// (If the formation controller has stopped moving however, we can safely message).
		if (IsFormationMember() && IsFormationControllerMoving())
			return;

		CMessageMotionUpdate msg(m_FailedMovements >= VERY_OBSTRUCTED_THRESHOLD ?
			CMessageMotionUpdate::VERY_OBSTRUCTED : CMessageMotionUpdate::OBSTRUCTED);
		GetSimContext().GetComponentManager().PostMessage(GetEntityId(), msg);
	}

	/**
	 * Increment the number of failed movements and notify other components if required.
	 * @returns true if the failure was notified, false otherwise.
	 */
	bool IncrementFailedMovementsAndMaybeNotify()
	{
		m_FailedMovements++;
		if (m_FailedMovements >= MAX_FAILED_MOVEMENTS)
		{
			MoveFailed();
			m_FailedMovements = 0;
			return true;
		}
		return false;
	}

	/**
	 * If path would take us farther away from the goal than pos currently is, return false, else return true.
	 */
	bool RejectFartherPaths(const PathGoal& goal, const WaypointPath& path, const CFixedVector2D& pos) const;

	bool ShouldAlternatePathfinder() const
	{
		return (m_FailedMovements == ALTERNATE_PATH_TYPE_DELAY) || ((MAX_FAILED_MOVEMENTS - ALTERNATE_PATH_TYPE_DELAY) % ALTERNATE_PATH_TYPE_EVERY == 0);
	}

	bool InShortPathRange(const PathGoal& goal, const CFixedVector2D& pos) const
	{
		return goal.DistanceToPoint(pos) < LONG_PATH_MIN_DIST;
	}

	entity_pos_t ShortPathSearchRange() const
	{
		u8 multiple = m_FailedMovements < SHORT_PATH_SEARCH_RANGE_INCREASE_DELAY ? 0 : m_FailedMovements - SHORT_PATH_SEARCH_RANGE_INCREASE_DELAY;
		fixed searchRange = SHORT_PATH_MIN_SEARCH_RANGE + SHORT_PATH_SEARCH_RANGE_INCREMENT * multiple;
		if (searchRange > SHORT_PATH_MAX_SEARCH_RANGE)
			searchRange = SHORT_PATH_MAX_SEARCH_RANGE;
		return searchRange;
	}

	/**
	 * Handle the result of an asynchronous path query.
	 */
	void PathResult(u32 ticket, const WaypointPath& path);

	void OnValueModification()
	{
		CmpPtr<ICmpValueModificationManager> cmpValueModificationManager(GetSystemEntity());
		if (!cmpValueModificationManager)
			return;

		m_WalkSpeed = cmpValueModificationManager->ApplyModifications(L"UnitMotion/WalkSpeed", m_TemplateWalkSpeed, GetEntityId());
		m_RunMultiplier = cmpValueModificationManager->ApplyModifications(L"UnitMotion/RunMultiplier", m_TemplateRunMultiplier, GetEntityId());

		// For MT_Deserialize compute m_Speed from the serialized m_SpeedMultiplier.
		// For MT_ValueModification and MT_OwnershipChanged, adjust m_SpeedMultiplier if needed
		// (in case then new m_RunMultiplier value is lower than the old).
		SetSpeedMultiplier(m_SpeedMultiplier);
	}

	/**
	 * Check if we are at destination early in the turn, this both lets units react faster
	 * and ensure that distance comparisons are done while units are not being moved
	 * (otherwise they won't be commutative).
	 */
	void OnTurnStart();

	void PreMove(CCmpUnitMotionManager::MotionState& state);
	void Move(CCmpUnitMotionManager::MotionState& state, fixed dt);
	void PostMove(CCmpUnitMotionManager::MotionState& state, fixed dt);

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
	bool PerformMove(fixed dt, const fixed& turnRate, WaypointPath& shortPath, WaypointPath& longPath, CFixedVector2D& pos, entity_angle_t& angle) const;

	/**
	 * Update other components on our speed.
	 * (For performance, this should try to avoid sending messages).
	 */
	void UpdateMovementState(entity_pos_t speed);

	/**
	 * React if our move was obstructed.
	 * @param moved - true if the unit still managed to move.
	 * @returns true if the obstruction required handling, false otherwise.
	 */
	bool HandleObstructedMove(bool moved);

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
	bool TryGoingStraightToTarget(const CFixedVector2D& from, bool updatePaths);

	/**
	 * Returns whether our we need to recompute a path to reach our target.
	 */
	bool PathingUpdateNeeded(const CFixedVector2D& from) const;

	/**
	 * Rotate to face towards the target point, given the current pos
	 */
	void FaceTowardsPointFromPos(const CFixedVector2D& pos, entity_pos_t x, entity_pos_t z);

	/**
	 * Units in 'pushing' mode are marked as 'moving' in the obstruction manager.
	 * Units in 'pushing' mode should skip them in checkMovement (to enable pushing).
	 * However, units for which pushing is deactivated should collide against everyone.
	 * Units that don't block movement never participate in pushing, but they also
	 * shouldn't collide with pushing units.
	 */
	bool ShouldCollideWithMovingUnits() const
	{
		return !m_Pushing && m_BlockMovement;
	}

	/**
	 * Returns an appropriate obstruction filter for use with path requests.
	 */
	ControlGroupMovementObstructionFilter GetObstructionFilter() const
	{
		return ControlGroupMovementObstructionFilter(ShouldCollideWithMovingUnits(), GetGroup());
	}
	/**
	 * Filter a specific tag on top of the existing control groups.
	 */
	SkipTagAndControlGroupObstructionFilter GetObstructionFilter(const ICmpObstructionManager::tag_t& tag) const
	{
		return SkipTagAndControlGroupObstructionFilter(tag, ShouldCollideWithMovingUnits(), GetGroup());
	}

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
	 * @param extendRange - if true, extend the search range to at least the distance to the goal.
	 */
	void RequestShortPath(const CFixedVector2D& from, const PathGoal& goal, bool extendRange);

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

	// If we not longer have a position, we won't be able to do much.
	// Fail in the next Move() call.
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return;
	CFixedVector2D pos = cmpPosition->GetPosition2D();

	// Assume all long paths were towards the goal, and assume short paths were if there are no long waypoints.
	bool pathedTowardsGoal = ticketType == Ticket::LONG_PATH || m_LongPath.m_Waypoints.empty();

	// Check if we need to run the short-path hack (warning: tricky control flow).
	bool shortPathHack = false;
	if (path.m_Waypoints.empty())
	{
		// No waypoints means pathing failed. If this was a long-path, try the short-path hack.
		if (!pathedTowardsGoal)
			return;
		shortPathHack = ticketType == Ticket::LONG_PATH;
	}
	else if (PathGoal goal; pathedTowardsGoal && ComputeGoal(goal, m_MoveRequest) && RejectFartherPaths(goal, path, pos))
	{
		// Reject paths that would take the unit further away from the goal.
		// This assumes that we prefer being closer 'as the crow flies' to unreachable goals.
		// This is a hack of sorts around units 'dancing' between two positions (see e.g. #3144),
		// but never actually failing to move, ergo never actually informing unitAI that it succeeds/fails.
		// (for short paths, only do so if aiming directly for the goal
		// as sub-goals may be farther than we are).

		// If this was a long-path and we no longer have waypoints, try the short-path hack.
		if (!m_LongPath.m_Waypoints.empty())
			return;
		shortPathHack = ticketType == Ticket::LONG_PATH;
	}

	// Short-path hack: if the long-range pathfinder doesn't find an acceptable path, push a fake waypoint at the goal.
	// This means HandleObstructedMove will use the short-pathfinder to try and reach it,
	// and that may find a path as the vertex pathfinder is more precise.
	if (shortPathHack)
	{
		// If we're resorting to the short-path hack, the situation is dire. Most likely, the goal is unreachable.
		// We want to find a path or fail fast. Bump failed movements so the short pathfinder will run at max-range
		// right away. This is safe from a performance PoV because it can only happen if the target is unreachable to
		// the long-range pathfinder, which is rare, and since the entity will fail to move if the goal is actually unreachable,
		// the failed movements will be increased to MAX anyways, so just shortcut.
		m_FailedMovements = MAX_FAILED_MOVEMENTS - 2;

		CFixedVector2D targetPos;
		if (ComputeTargetPosition(targetPos))
			m_LongPath.m_Waypoints.emplace_back(Waypoint{ targetPos.X, targetPos.Y });
		return;
	}

	if (ticketType == Ticket::LONG_PATH)
	{
		m_LongPath = path;
		// Long paths don't properly follow diagonals because of JPS/the grid. Since units now take time turning,
		// they can actually slow down substantially if they have to do a one navcell diagonal movement,
		// which is somewhat common at the beginning of a new path.
		// For that reason, if the first waypoint is really close, check if we can't go directly to the second.
		if (m_LongPath.m_Waypoints.size() >= 2)
		{
			const Waypoint& firstWpt = m_LongPath.m_Waypoints.back();
			if (CFixedVector2D(firstWpt.x - pos.X, firstWpt.z - pos.Y).CompareLength(Pathfinding::NAVCELL_SIZE * 4) <= 0)
			{
				CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
				ENSURE(cmpPathfinder);
				const Waypoint& secondWpt = m_LongPath.m_Waypoints[m_LongPath.m_Waypoints.size() - 2];
				if (cmpPathfinder->CheckMovement(GetObstructionFilter(), pos.X, pos.Y, secondWpt.x, secondWpt.z, m_Clearance, m_PassClass))
					m_LongPath.m_Waypoints.pop_back();
			}

		}
	}
	else
		m_ShortPath = path;

	m_FollowKnownImperfectPathCountdown = 0;

	if (!pathedTowardsGoal)
		return;

	// Performance hack: If we were pathing towards the goal and this new path won't put us in range,
	// it's highly likely that we are going somewhere unreachable.
	// However, Move() will try to recompute the path every turn, which can be quite slow.
	// To avoid this, act as if our current path leads us to the correct destination.
	// NB: for short-paths, the problem might be that the search space is too small
	// but we'll still follow this path until the en and try again then.
	// Because we reject farther paths, it works out.
	if (PathingUpdateNeeded(pos))
	{
		// Inform other components early, as they might have better behaviour than waiting for the path to carry out.
		// Send OBSTRUCTED at first - moveFailed is likely to trigger path recomputation and we might end up
		// recomputing too often for nothing.
		if (!IncrementFailedMovementsAndMaybeNotify())
			MoveObstructed();
		// We'll automatically recompute a path when this reaches 0, as a way to improve behaviour.
		// (See D665 - this is needed because the target may be moving, and we should adjust to that).
		m_FollowKnownImperfectPathCountdown = KNOWN_IMPERFECT_PATH_RESET_COUNTDOWN;
	}
}

void CCmpUnitMotion::OnTurnStart()
{
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
}

void CCmpUnitMotion::PreMove(CCmpUnitMotionManager::MotionState& state)
{
	state.ignore = !m_Pushing || !m_BlockMovement;

	state.wasObstructed = false;
	state.wentStraight = false;

	// If we were idle and will still be, no need for an update.
	state.needUpdate = state.cmpPosition->IsInWorld() &&
		(m_CurSpeed != fixed::Zero() || m_MoveRequest.m_Type != MoveRequest::NONE);

	if (!m_BlockMovement)
		return;

	state.controlGroup = IsFormationMember() ? m_MoveRequest.m_Entity : INVALID_ENTITY;

	// Update moving flag, this is an internal construct used for pushing,
	// so it does not really reflect whether the unit is actually moving or not.
	state.isMoving = m_Pushing && m_MoveRequest.m_Type != MoveRequest::NONE;
	CmpPtr<ICmpObstruction> cmpObstruction(GetEntityHandle());
	if (cmpObstruction)
		cmpObstruction->SetMovingFlag(state.isMoving);
}

void CCmpUnitMotion::Move(CCmpUnitMotionManager::MotionState& state, fixed dt)
{
	PROFILE("Move");

	// If we're chasing a potentially-moving unit and are currently close
	// enough to its current position, and we can head in a straight line
	// to it, then throw away our current path and go straight to it.
	state.wentStraight = TryGoingStraightToTarget(state.initialPos, true);

	state.wasObstructed = PerformMove(dt, state.cmpPosition->GetTurnRate(), m_ShortPath, m_LongPath, state.pos, state.angle);
}

void CCmpUnitMotion::PostMove(CCmpUnitMotionManager::MotionState& state, fixed dt)
{
	// Update our speed over this turn so that the visual actor shows the correct animation.
	if (state.pos == state.initialPos)
	{
		if (state.angle != state.initialAngle)
			state.cmpPosition->TurnTo(state.angle);
		UpdateMovementState(fixed::Zero());
	}
	else
	{
		// Update the Position component after our movement (if we actually moved anywhere)
		CFixedVector2D offset = state.pos - state.initialPos;
		// When moving always set the angle in the direction of the movement,
		// if we are not trying to move, assume this is pushing-related movement,
		// and maintain the current angle instead.
		if (IsMoveRequested())
			state.angle = atan2_approx(offset.X, offset.Y);
		state.cmpPosition->MoveAndTurnTo(state.pos.X, state.pos.Y, state.angle);

		// Calculate the mean speed over this past turn.
		UpdateMovementState(offset.Length() / dt);
	}

	if (state.wasObstructed && HandleObstructedMove(state.pos != state.initialPos))
		return;
	else if (!state.wasObstructed && state.pos != state.initialPos)
		m_FailedMovements = 0;

	// If we moved straight, and didn't quite finish the path, reset - we'll update it next turn if still OK.
	if (state.wentStraight && !state.wasObstructed)
		m_ShortPath.m_Waypoints.clear();

	// We may need to recompute our path sometimes (e.g. if our target moves).
	// Since we request paths asynchronously anyways, this does not need to be done before moving.
	if (!state.wentStraight && PathingUpdateNeeded(state.pos))
	{
		PathGoal goal;
		if (ComputeGoal(goal, m_MoveRequest))
			ComputePathToGoal(state.pos, goal);
	}
	else if (m_FollowKnownImperfectPathCountdown > 0)
		--m_FollowKnownImperfectPathCountdown;
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

		// In formation, return a match only if we are exactly at the target position.
		// Otherwise, units can go in an infinite "walzting" loop when the Idle formation timer
		// reforms them.
		CFixedVector2D targetPos;
		ComputeTargetPosition(targetPos);
		CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
		return (targetPos-cmpPosition->GetPosition2D()).CompareLength(fixed::Zero()) <= 0;
	}
	return false;
}

bool CCmpUnitMotion::PerformMove(fixed dt, const fixed& turnRate, WaypointPath& shortPath, WaypointPath& longPath, CFixedVector2D& pos, entity_angle_t& angle) const
{
	// If there are no waypoint, behave as though we were obstructed and let HandleObstructedMove handle it.
	if (shortPath.m_Waypoints.empty() && longPath.m_Waypoints.empty())
		return true;

	// Wrap the angle to (-Pi, Pi].
	while (angle > entity_angle_t::Pi())
		angle -= entity_angle_t::Pi() * 2;
	while (angle < -entity_angle_t::Pi())
		angle += entity_angle_t::Pi() * 2;

	// TODO: there's some asymmetry here when units look at other
	// units' positions - the result will depend on the order of execution.
	// Maybe we should split the updates into multiple phases to minimise
	// that problem.

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	ENSURE(cmpPathfinder);

	fixed basicSpeed = m_Speed;
	// If in formation, run to keep up; otherwise just walk.
	if (IsFormationMember())
		basicSpeed = m_Speed.Multiply(m_RunMultiplier);

	// Find the speed factor of the underlying terrain.
	// (We only care about the tile we start on - it doesn't matter if we're moving
	// partially onto a much slower/faster tile).
	// TODO: Terrain-dependent speeds are not currently supported.
	fixed terrainSpeed = fixed::FromInt(1);

	fixed maxSpeed = basicSpeed.Multiply(terrainSpeed);

	fixed timeLeft = dt;
	fixed zero = fixed::Zero();

	ICmpObstructionManager::tag_t specificIgnore;
	if (m_MoveRequest.m_Type == MoveRequest::ENTITY)
	{
		CmpPtr<ICmpObstruction> cmpTargetObstruction(GetSimContext(), m_MoveRequest.m_Entity);
		if (cmpTargetObstruction)
			specificIgnore = cmpTargetObstruction->GetObstruction();
	}

	while (timeLeft > zero)
	{
		// If we ran out of path, we have to stop.
		if (shortPath.m_Waypoints.empty() && longPath.m_Waypoints.empty())
			break;

		CFixedVector2D target;
		if (shortPath.m_Waypoints.empty())
			target = CFixedVector2D(longPath.m_Waypoints.back().x, longPath.m_Waypoints.back().z);
		else
			target = CFixedVector2D(shortPath.m_Waypoints.back().x, shortPath.m_Waypoints.back().z);

		CFixedVector2D offset = target - pos;
		if (turnRate > zero && !offset.IsZero())
		{
			fixed maxRotation = turnRate.Multiply(timeLeft);
			fixed angleDiff = angle - atan2_approx(offset.X, offset.Y);
			if (angleDiff != zero)
			{
				fixed absoluteAngleDiff = angleDiff.Absolute();
				if (absoluteAngleDiff > entity_angle_t::Pi())
					absoluteAngleDiff = entity_angle_t::Pi() * 2 - absoluteAngleDiff;

				// Figure out whether rotating will increase or decrease the angle, and how far we need to rotate in that direction.
				int direction = (entity_angle_t::Zero() < angleDiff && angleDiff <= entity_angle_t::Pi()) || angleDiff < -entity_angle_t::Pi() ? -1 : 1;

				// Can't rotate far enough, just rotate in the correct direction.
				if (absoluteAngleDiff > maxRotation)
				{
					angle += maxRotation * direction;
					if (angle * direction > entity_angle_t::Pi())
						angle -= entity_angle_t::Pi() * 2 * direction;
					break;
				}
				// Rotate towards the next waypoint and continue moving.
				angle = atan2_approx(offset.X, offset.Y);
				// Give some 'free' rotation for angles below 0.5 radians.
				timeLeft = (std::min(maxRotation, maxRotation - absoluteAngleDiff + fixed::FromInt(1)/2)) / turnRate;
			}
		}

		// Work out how far we can travel in timeLeft.
		fixed maxdist = maxSpeed.Multiply(timeLeft);

		// If the target is close, we can move there directly.
		fixed offsetLength = offset.Length();
		if (offsetLength <= maxdist)
		{
			if (cmpPathfinder->CheckMovement(GetObstructionFilter(specificIgnore), pos.X, pos.Y, target.X, target.Y, m_Clearance, m_PassClass))
			{
				pos = target;

				// Spend the rest of the time heading towards the next waypoint.
				timeLeft = (maxdist - offsetLength) / maxSpeed;

				if (shortPath.m_Waypoints.empty())
					longPath.m_Waypoints.pop_back();
				else
					shortPath.m_Waypoints.pop_back();

				continue;
			}
			else
			{
				// Error - path was obstructed.
				return true;
			}
		}
		else
		{
			// Not close enough, so just move in the right direction.
			offset.Normalize(maxdist);
			target = pos + offset;

			if (cmpPathfinder->CheckMovement(GetObstructionFilter(specificIgnore), pos.X, pos.Y, target.X, target.Y, m_Clearance, m_PassClass))
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
	CmpPtr<ICmpVisual> cmpVisual(GetEntityHandle());
	if (cmpVisual)
	{
		if (speed == fixed::Zero())
			cmpVisual->SelectMovementAnimation("idle", fixed::FromInt(1));
		else
			cmpVisual->SelectMovementAnimation(speed > (m_WalkSpeed / 2).Multiply(m_RunMultiplier + fixed::FromInt(1)) ? "run" : "walk", speed);
	}

	m_CurSpeed = speed;
}

bool CCmpUnitMotion::HandleObstructedMove(bool moved)
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	// We failed to move, inform other components as they might handle it.
	// (don't send messages on the first failure, as that would be too noisy).
	// Also don't increment above the initial MoveObstructed message if we actually manage to move a little.
	if (!moved || m_FailedMovements < 2)
	{
		if (!IncrementFailedMovementsAndMaybeNotify() && m_FailedMovements >= 2)
			MoveObstructed();
	}

	PathGoal goal;
	if (!ComputeGoal(goal, m_MoveRequest))
		return false;

	// At this point we have a position in the world since ComputeGoal checked for that.
	CFixedVector2D pos = cmpPosition->GetPosition2D();

	// Assume that we are merely obstructed and the long path is salvageable, so try going around the obstruction.
	// This could be a separate function, but it doesn't really make sense to call it outside of here, and I can't find a name.
	// I use an IIFE to have nice 'return' semantics still.
	if ([&]() -> bool {
		// If the goal is close enough, we should ignore any remaining long waypoint and just
		// short path there directly, as that improves behaviour in general - see D2095).
		if (InShortPathRange(goal, pos))
			return false;

		// Delete the next waypoint if it's reasonably close,
		// because it might be blocked by units and thus unreachable.
		// NB: this number is tricky. Make it too high, and units start going down dead ends, which looks odd (#5795)
		// Make it too low, and they might get stuck behind other obstructed entities.
		// It also has performance implications because it calls the short-pathfinder.
		fixed skipbeyond = std::max(ShortPathSearchRange() / 3, Pathfinding::NAVCELL_SIZE * 8);
		if (m_LongPath.m_Waypoints.size() > 1 &&
		    (pos - CFixedVector2D(m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z)).CompareLength(skipbeyond) < 0)
		{
			m_LongPath.m_Waypoints.pop_back();
		}
		else if (ShouldAlternatePathfinder())
		{
			// Recompute the whole thing occasionally, in case we got stuck in a dead end from removing long waypoints.
			RequestLongPath(pos, goal);
			return true;
		}

		if (m_LongPath.m_Waypoints.empty())
			return false;

		// Compute a short path in the general vicinity of the next waypoint, to help pathfinding in crowds.
		// The goal here is to manage to move in the general direction of our target, not to be super accurate.
		fixed radius = Clamp(skipbeyond/3, Pathfinding::NAVCELL_SIZE * 4, Pathfinding::NAVCELL_SIZE * 12);
		PathGoal subgoal = { PathGoal::CIRCLE, m_LongPath.m_Waypoints.back().x, m_LongPath.m_Waypoints.back().z, radius };
		RequestShortPath(pos, subgoal, false);
		return true;
	}()) return true;

	// If we couldn't use a workaround, try recomputing the entire path.
	ComputePathToGoal(pos, goal);

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
		// Position is only updated after all units have moved & pushed.
		// Therefore, we may need to interpolate the target position, depending on when this call takes place during the turn:
		//  - On "Turn Start", we'll check positions directly without interpolation.
		//  - During movement, we'll call this for direct-pathing & we need to interpolate
		//    (this way, we move where the unit will end up at the end of _this_ turn, making it match on next turn start).
		//  - After movement, we'll call this to request paths & we need to interpolate
		//    (this way, we'll move where the unit ends up in the end of _next_ turn, making it a match in 2 turns).
		// TODO: This does not really aim many turns in advance, with orthogonal trajectories it probably should.
		CmpPtr<ICmpUnitMotion> cmpUnitMotion(GetSimContext(), moveRequest.m_Entity);
		CmpPtr<ICmpUnitMotionManager> cmpUnitMotionManager(GetSystemEntity());
		bool needInterpolation = cmpUnitMotion && cmpUnitMotion->IsMoveRequested() && cmpUnitMotionManager->ComputingMotion();
		if (needInterpolation)
		{
			// Add predicted movement.
			CFixedVector2D tempPos = out + (out - cmpTargetPosition->GetPreviousPosition2D());
			out = tempPos;
		}
	}
	return true;
}

bool CCmpUnitMotion::TryGoingStraightToTarget(const CFixedVector2D& from, bool updatePaths)
{
	// Assume if we have short paths we want to follow them.
	// Exception: offset movement (formations) generally have very short deltas
	// and to look good we need them to walk-straight most of the time.
	if (!IsFormationMember() && !m_ShortPath.m_Waypoints.empty())
		return false;

	CFixedVector2D targetPos;
	if (!ComputeTargetPosition(targetPos))
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

	// Fail if the target is too far away
	if ((goalPos - from).CompareLength(DIRECT_PATH_RANGE) > 0)
		return false;

	// Check if there's any collisions on that route.
	// For entity goals, skip only the specific obstruction tag or with e.g. walls we might ignore too many entities.
	ICmpObstructionManager::tag_t specificIgnore;
	if (m_MoveRequest.m_Type == MoveRequest::ENTITY)
	{
		CmpPtr<ICmpObstruction> cmpTargetObstruction(GetSimContext(), m_MoveRequest.m_Entity);
		if (cmpTargetObstruction)
			specificIgnore = cmpTargetObstruction->GetObstruction();
	}

	// Check movement against units - we want to use the short pathfinder to walk around those if needed.
	if (specificIgnore.valid())
	{
		if (!cmpPathfinder->CheckMovement(GetObstructionFilter(specificIgnore), from.X, from.Y, goalPos.X, goalPos.Y, m_Clearance, m_PassClass))
			return false;
	}
	else if (!cmpPathfinder->CheckMovement(GetObstructionFilter(), from.X, from.Y, goalPos.X, goalPos.Y, m_Clearance, m_PassClass))
		return false;

	if (!updatePaths)
		return true;

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

	if (m_FollowKnownImperfectPathCountdown > 0 && (!m_LongPath.m_Waypoints.empty() || !m_ShortPath.m_Waypoints.empty()))
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
			entity_pos_t delta = std::max(goalDistance, m_Clearance + entity_pos_t::FromInt(4)/16); // ensure it's far enough to not intersect the building itself
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

	// If the target is close enough, hope that we'll be able to go straight next turn.
	if (!ShouldAlternatePathfinder() && TryGoingStraightToTarget(from, false))
	{
		// NB: since we may fail to move straight next turn, we should edge our bets.
		// Since the 'go straight' logic currently fires only if there's no short path,
		// we'll compute a long path regardless to make sure _that_ stays up to date.
		// (it's also extremely likely to be very fast to compute, so no big deal).
		m_ShortPath.m_Waypoints.clear();
		RequestLongPath(from, goal);
		return;
	}

	// Otherwise we need to compute a path.

	// If it's close then just do a short path, not a long path
	// TODO: If it's close on the opposite side of a river then we really
	// need a long path, so we shouldn't simply check linear distance
	// the check is arbitrary but should be a reasonably small distance.
	// We want to occasionally compute a long path if we're computing short-paths, because the short path domain
	// is bounded and thus it can't around very large static obstacles.
	// Likewise, we want to compile a short-path occasionally when the target is far because we might be stuck
	// on a navcell surrounded by impassable navcells, but the short-pathfinder could move us out of there.
	bool shortPath = InShortPathRange(goal, from);
	if (ShouldAlternatePathfinder())
		shortPath = !shortPath;
	if (shortPath)
	{
		m_LongPath.m_Waypoints.clear();
		// Extend the range so that our first path is probably valid.
		RequestShortPath(from, goal, true);
	}
	else
	{
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

void CCmpUnitMotion::RequestShortPath(const CFixedVector2D &from, const PathGoal& goal, bool extendRange)
{
	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
	if (!cmpPathfinder)
		return;

	entity_pos_t searchRange = ShortPathSearchRange();
	if (extendRange)
	{
		CFixedVector2D dist(from.X - goal.x, from.Y - goal.z);
		if (dist.CompareLength(searchRange - entity_pos_t::FromInt(1)) >= 0)
		{
			searchRange = dist.Length() + fixed::FromInt(1);
			if (searchRange > SHORT_PATH_MAX_SEARCH_RANGE)
				searchRange = SHORT_PATH_MAX_SEARCH_RANGE;
		}
	}

	m_ExpectedPathTicket.m_Type = Ticket::SHORT_PATH;
	m_ExpectedPathTicket.m_Ticket = cmpPathfinder->ComputeShortPathAsync(from.X, from.Y, m_Clearance, searchRange, goal, m_PassClass, true, GetGroup(), GetEntityId());
}

bool CCmpUnitMotion::MoveTo(MoveRequest request)
{
	PROFILE("MoveTo");

	if (request.m_MinRange == request.m_MaxRange && !request.m_MinRange.IsZero())
		LOGWARNING("MaxRange must be larger than MinRange; See CCmpUnitMotion.cpp for more information");

	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	PathGoal goal;
	if (!ComputeGoal(goal, request))
		return false;

	m_MoveRequest = request;
	m_FailedMovements = 0;
	m_FollowKnownImperfectPathCountdown = 0;

	ComputePathToGoal(cmpPosition->GetPosition2D(), goal);
	return true;
}

bool CCmpUnitMotion::IsTargetRangeReachable(entity_id_t target, entity_pos_t minRange, entity_pos_t maxRange)
{
	CmpPtr<ICmpPosition> cmpPosition(GetEntityHandle());
	if (!cmpPosition || !cmpPosition->IsInWorld())
		return false;

	MoveRequest request(target, minRange, maxRange);
	PathGoal goal;
	if (!ComputeGoal(goal, request))
		return false;

	CmpPtr<ICmpPathfinder> cmpPathfinder(GetSimContext(), SYSTEM_ENTITY);
	CFixedVector2D pos = cmpPosition->GetPosition2D();
	return cmpPathfinder->IsGoalReachable(pos.X, pos.Y, goal, m_PassClass);
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

#endif // INCLUDED_CCMPUNITMOTION
