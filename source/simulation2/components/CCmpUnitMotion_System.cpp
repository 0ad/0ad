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

#include "precompiled.h"

#include "CCmpUnitMotion.h"
#include "CCmpUnitMotionManager.h"

#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"

#include <unordered_set>

// NB: this TU contains the CCmpUnitMotion/CCmpUnitMotionManager couple.
// In practice, UnitMotionManager functions need access to the full implementation of UnitMotion,
// but UnitMotion needs access to MotionState (defined in UnitMotionManager).
// To avoid inclusion issues, implementation of UnitMotionManager that uses UnitMotion is here.

namespace {
	/**
	 * Units push only within their own grid square. This is the size of each square (in arbitrary units).
	 * TODO: check other values.
	 */
	static const int PUSHING_GRID_SIZE = 20;

	/**
	 * Pushing is ignored if the combined push force has lower magnitude than this.
	 */
	static const entity_pos_t MINIMAL_PUSHING = entity_pos_t::FromInt(3) / 10;

	/**
	 * For pushing, treat the clearances as a circle - they're defined as squares,
	 * so we'll take the circumscribing square (approximately).
	 * Clerances are also full-width instead of half, so we want to divide by two. sqrt(2)/2 is about 0.71 < 5/7.
	 */
	static const entity_pos_t PUSHING_CORRECTION = entity_pos_t::FromInt(5) / 7;

	/**
	 * When moving, units exert a pushing influence at a greater distance.
	 */
	static const entity_pos_t PUSHING_MOVING_INFLUENCE_EXTENSION = entity_pos_t::FromInt(1);

	/**
	 * Arbitrary constant used to reduce pushing to levels that won't break physics for our turn length.
	 */
	static const int PUSHING_REDUCTION_FACTOR = 2;
}

CCmpUnitMotionManager::MotionState::MotionState(CmpPtr<ICmpPosition> cmpPos, CCmpUnitMotion* cmpMotion)
	: cmpPosition(cmpPos), cmpUnitMotion(cmpMotion)
{
}

void CCmpUnitMotionManager::Init(const CParamNode&)
{
	// Load some data - see CCmpPathfinder.xml.
	// This assumes the pathfinder component is initialised first and registers the validator.
	// TODO: there seems to be no real reason why we could not register a 'system' entity somewhere instead.
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml", "pathfinder");
	const CParamNode radius = externalParamNode.GetChild("Pathfinder").GetChild("PushingRadius");
	if (radius.IsOk())
	{
		m_PushingRadius = radius.ToFixed();
		if (m_PushingRadius < entity_pos_t::Zero())
		{
			LOGWARNING("Pushing radius cannot be below 0. De-activating pushing but 'pathfinder.xml' should be updated.");
			m_PushingRadius = entity_pos_t::Zero();
		}
		// No upper value, but things won't behave sanely if values are too high.
	}
	else
		m_PushingRadius = entity_pos_t::FromInt(8) / 5;
	m_PushingRadius = m_PushingRadius.Multiply(PUSHING_CORRECTION);
}

void CCmpUnitMotionManager::Register(CCmpUnitMotion* component, entity_id_t ent, bool formationController)
{
	MotionState state(CmpPtr<ICmpPosition>(GetSimContext(), ent), component);
	if (!formationController)
		m_Units.insert(ent, state);
	else
		m_FormationControllers.insert(ent, state);
}

void CCmpUnitMotionManager::Unregister(entity_id_t ent)
{
	EntityMap<MotionState>::iterator it = m_Units.find(ent);
	if (it != m_Units.end())
	{
		m_Units.erase(it);
		return;
	}
	it = m_FormationControllers.find(ent);
	if (it != m_FormationControllers.end())
		m_FormationControllers.erase(it);
}

void CCmpUnitMotionManager::OnTurnStart()
{
	for (EntityMap<MotionState>::value_type& data : m_FormationControllers)
		data.second.cmpUnitMotion->OnTurnStart();

	for (EntityMap<MotionState>::value_type& data : m_Units)
		data.second.cmpUnitMotion->OnTurnStart();
}

void CCmpUnitMotionManager::MoveUnits(fixed dt)
{
	Move(m_Units, dt);
}

void CCmpUnitMotionManager::MoveFormations(fixed dt)
{
	Move(m_FormationControllers, dt);
}

void CCmpUnitMotionManager::Move(EntityMap<MotionState>& ents, fixed dt)
{
	PROFILE2("MotionMgr_Move");
	std::unordered_set<std::vector<EntityMap<MotionState>::iterator>*> assigned;
	for (EntityMap<MotionState>::iterator it = ents.begin(); it != ents.end(); ++it)
	{
		if (!it->second.cmpPosition->IsInWorld())
		{
			it->second.needUpdate = false;
			continue;
		}
		else
			it->second.cmpUnitMotion->PreMove(it->second);
		it->second.initialPos = it->second.cmpPosition->GetPosition2D();
		it->second.initialAngle = it->second.cmpPosition->GetRotation().Y;
		it->second.pos = it->second.initialPos;
		it->second.angle = it->second.initialAngle;
		ENSURE(it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE < m_MovingUnits.width() &&
			   it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE < m_MovingUnits.height());
		std::vector<EntityMap<MotionState>::iterator>& subdiv = m_MovingUnits.get(
			it->second.pos.X.ToInt_RoundToZero() / PUSHING_GRID_SIZE,
			it->second.pos.Y.ToInt_RoundToZero() / PUSHING_GRID_SIZE
		);
		subdiv.emplace_back(it);
		assigned.emplace(&subdiv);
	}

	for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		for (EntityMap<MotionState>::iterator& it : *vec)
			if (it->second.needUpdate)
				it->second.cmpUnitMotion->Move(it->second, dt);

	// Skip pushing entirely if the radius is 0
	if (&ents == &m_Units && m_PushingRadius != entity_pos_t::Zero())
	{
		PROFILE2("MotionMgr_Pushing");
		for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		{
			ENSURE(!vec->empty());

			std::vector<EntityMap<MotionState>::iterator>::iterator cit1 = vec->begin();
			do
			{
				if ((*cit1)->second.ignore)
					continue;
				std::vector<EntityMap<MotionState>::iterator>::iterator cit2 = cit1;
				while(++cit2 != vec->end())
					if (!(*cit2)->second.ignore)
						Push(**cit1, **cit2, dt);
			}
			while(++cit1 != vec->end());
		}
	}

	if (m_PushingRadius != entity_pos_t::Zero())
	{
		PROFILE2("MotionMgr_PushAdjust");
		CmpPtr<ICmpPathfinder> cmpPathfinder(GetSystemEntity());
		for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		{
			for (EntityMap<MotionState>::iterator& it : *vec)
			{

				if (!it->second.needUpdate || it->second.ignore)
					continue;

				// Prevent pushed units from crossing uncrossable boundaries
				// (we can assume that normal movement didn't push units into impassable terrain).
				if ((it->second.push.X != entity_pos_t::Zero() || it->second.push.Y != entity_pos_t::Zero()) &&
					!cmpPathfinder->CheckMovement(it->second.cmpUnitMotion->GetObstructionFilter(),
						it->second.pos.X, it->second.pos.Y,
						it->second.pos.X + it->second.push.X, it->second.pos.Y + it->second.push.Y,
						it->second.cmpUnitMotion->m_Clearance,
						it->second.cmpUnitMotion->m_PassClass))
				{
					// Mark them as obstructed - this could possibly be optimised
					// perhaps it'd make more sense to mark the pushers as blocked.
					it->second.wasObstructed = true;
					it->second.wentStraight = false;
					it->second.push = CFixedVector2D();
				}
				// Only apply pushing if the effect is significant enough.
				if (it->second.push.CompareLength(MINIMAL_PUSHING) > 0)
				{
					// If there was an attempt at movement, and the pushed movement is in a sufficiently different direction
					// (measured by an extremely arbitrary dot product)
					// then mark the unit as obstructed still.
					if (it->second.pos != it->second.initialPos &&
						(it->second.pos - it->second.initialPos).Dot(it->second.pos + it->second.push - it->second.initialPos) < entity_pos_t::FromInt(1)/2)
					{
						it->second.wasObstructed = true;
						it->second.wentStraight = false;
						// Push anyways.
					}
					it->second.pos += it->second.push;
				}
				it->second.push = CFixedVector2D();
			}
		}
	}
	{
		PROFILE2("MotionMgr_PostMove");
		for (EntityMap<MotionState>::value_type& data : ents)
		{
			if (!data.second.needUpdate)
				continue;
			data.second.cmpUnitMotion->PostMove(data.second, dt);
		}
	}
	for (std::vector<EntityMap<MotionState>::iterator>* vec : assigned)
		vec->clear();
}

// TODO: ought to better simulate in-flight pushing, e.g. if units would cross in-between turns.
void CCmpUnitMotionManager::Push(EntityMap<MotionState>::value_type& a, EntityMap<MotionState>::value_type& b, fixed dt)
{
	// The hard problem for pushing is knowing when to actually use the pathfinder to go around unpushable obstacles.
	// For simplicitly, the current logic separates moving & stopped entities:
	// moving entities will push moving entities, but not stopped ones, and vice-versa.
	// this still delivers most of the value of pushing, without a lot of the complexity.
	int movingPush = a.second.isMoving + b.second.isMoving;

	// Exception: units in the same control group (i.e. the same formation) never push farther than themselves
	// and are also allowed to push idle units (obstructions are ignored within formations,
	// so pushing idle units makes one member crossing the formation look better).
	if (a.second.controlGroup != INVALID_ENTITY && a.second.controlGroup == b.second.controlGroup)
		movingPush = 0;

	if (movingPush == 1)
		return;

	entity_pos_t combinedClearance = (a.second.cmpUnitMotion->m_Clearance + b.second.cmpUnitMotion->m_Clearance).Multiply(m_PushingRadius);
	entity_pos_t maxDist = combinedClearance;
	if (movingPush)
		maxDist += PUSHING_MOVING_INFLUENCE_EXTENSION;

	CFixedVector2D offset = a.second.pos - b.second.pos;
	if (offset.CompareLength(maxDist) > 0)
		return;

	entity_pos_t offsetLength = offset.Length();
	// If the offset is small enough that precision would be problematic, pick an arbitrary vector instead.
	if (offsetLength <= entity_pos_t::Epsilon() * 10)
	{
		// Throw in some 'randomness' so that clumped units unclump more naturally.
		bool dir = a.first % 2;
		offset.X = entity_pos_t::FromInt(dir ? 1 : 0);
		offset.Y = entity_pos_t::FromInt(dir ? 0 : 1);
		offsetLength = entity_pos_t::FromInt(1);
	}
	else
	{
		offset.X = offset.X / offsetLength;
		offset.Y = offset.Y / offsetLength;
	}

	// If the units are moving in opposite direction, check if they might have phased through each other.
	// If it looks like yes, move them perpendicularily so it looks like they avoid each other.
	// NB: this isn't very precise, nor will it catch 100% of intersections - it's meant as a cheap improvement.
	if (movingPush && (a.second.pos - a.second.initialPos).Dot(b.second.pos - b.second.initialPos) < entity_pos_t::Zero())
		// Perform some finer checking.
		if (Geometry::TestRayAASquare(a.second.initialPos - b.second.initialPos, a.second.pos - b.second.initialPos,
								  CFixedVector2D(combinedClearance, combinedClearance))
			||
			Geometry::TestRayAASquare(a.second.initialPos - b.second.pos, a.second.pos - b.second.pos,
									  CFixedVector2D(combinedClearance, combinedClearance)))
		{
			offset = offset.Perpendicular();
			offsetLength = fixed::Zero();
		}



	// The formula expects 'normal' pushing if the two entities edges are touching.
	entity_pos_t distanceFactor = movingPush ? (maxDist - offsetLength) / (maxDist - combinedClearance) : combinedClearance - offsetLength + entity_pos_t::FromInt(1);
	distanceFactor = Clamp(distanceFactor, entity_pos_t::Zero(), entity_pos_t::FromInt(2));

	// Mark both as needing an update so they actually get moved.
	a.second.needUpdate = true;
	b.second.needUpdate = true;

	CFixedVector2D pushingDir = offset.Multiply(distanceFactor);

	// Divide by an arbitrary constant to avoid pushing too much.
	a.second.push += pushingDir.Multiply(movingPush ? dt : dt / PUSHING_REDUCTION_FACTOR);
	b.second.push -= pushingDir.Multiply(movingPush ? dt : dt / PUSHING_REDUCTION_FACTOR);
}
