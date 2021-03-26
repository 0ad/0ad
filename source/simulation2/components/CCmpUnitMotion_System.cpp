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

#include "ps/CLogger.h"
#include "ps/Profile.h"

// NB: this TU contains the CCmpUnitMotion/CCmpUnitMotionManager couple.
// In practice, UnitMotionManager functions need access to the full implementation of UnitMotion,
// but UnitMotion needs access to MotionState (defined in UnitMotionManager).
// To avoid inclusion issues, implementation of UnitMotionManager that uses UnitMotion is here.

void CCmpUnitMotionManager::Register(CCmpUnitMotion* component, entity_id_t ent, bool formationController)
{
	MotionState state = {
		CmpPtr<ICmpPosition>(GetSimContext(), ent),
		component,
		CFixedVector2D(),
		CFixedVector2D(),
		fixed::Zero(),
		fixed::Zero(),
		false,
		false
	};
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
	m_MovingUnits.clear();
	for (EntityMap<MotionState>::iterator it = ents.begin(); it != ents.end(); ++it)
	{
		it->second.cmpUnitMotion->PreMove(it->second);
		if (!it->second.needUpdate)
			continue;
		m_MovingUnits.push_back(it);
		it->second.initialPos = it->second.cmpPosition->GetPosition2D();
		it->second.initialAngle = it->second.cmpPosition->GetRotation().Y;
		it->second.pos = it->second.initialPos;
		it->second.angle = it->second.initialAngle;
	}

	for (EntityMap<MotionState>::iterator& it : m_MovingUnits)
		it->second.cmpUnitMotion->Move(it->second, dt);

	for (EntityMap<MotionState>::iterator& it : m_MovingUnits)
		it->second.cmpUnitMotion->PostMove(it->second, dt);
}
