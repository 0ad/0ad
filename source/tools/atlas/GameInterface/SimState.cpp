/* Copyright (C) 2009 Wildfire Games.
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

#include "SimState.h"

#include "graphics/Model.h"
#include "graphics/ObjectBase.h"
#include "graphics/ObjectEntry.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "maths/MathUtil.h"
#include "ps/Player.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "simulation/Entity.h"
#include "simulation/EntityTemplate.h"
#include "simulation/EntityTemplateCollection.h"
#include "simulation/EntityManager.h"
#include "simulation/Projectile.h"

SimState::Entity SimState::Entity::Freeze(CUnit* unit)
{
	CEntity* entity = unit->GetEntity();
	debug_assert(entity);
	
	Entity e;
	e.templateName = entity->m_base->m_Tag;
	e.unitID = unit->GetID();
	e.selections = unit->GetActorSelections();
	e.playerID = entity->GetPlayer()->GetPlayerID();
	e.position = entity->m_position;
	e.angle = entity->m_orientation.Y;
	return e;
}

CEntity* SimState::Entity::Thaw()
{
	CEntityTemplate* base = g_EntityTemplateCollection.GetTemplate(templateName, g_Game->GetPlayer(playerID));
	if (! base)
		return NULL;
	
	HEntity ent = g_EntityManager.Create(base, position, angle, selections);
	if (! ent)
		return NULL;
		
	ent->m_actor->SetPlayerID(playerID);
	ent->m_actor->SetID(unitID);
	ent->Initialize();
	return ent;
}

SimState::Nonentity SimState::Nonentity::Freeze(CUnit* unit)
{
	Nonentity n;
	n.actorName = unit->GetObject()->m_Base->m_Name;
	n.unitID = unit->GetID();
	n.selections = unit->GetActorSelections();
	n.position = unit->GetModel()->GetTransform().GetTranslation();
	CVector3D orient = unit->GetModel()->GetTransform().GetIn();
	n.angle = atan2(-orient.X, -orient.Z);
	return n;
}

CUnit* SimState::Nonentity::Thaw()
{
	CUnitManager& unitMan = g_Game->GetWorld()->GetUnitManager();
	CUnit* unit = unitMan.CreateUnit(actorName, NULL, selections);
	if (! unit)
		return NULL;
	CMatrix3D m;
	m.SetYRotation(angle + PI);
	m.Translate(position);
	unit->GetModel()->SetTransform(m);

	unit->SetID(unitID);
	return unit;
}

SimState* SimState::Freeze(bool onlyEntities)
{
	SimState* simState = new SimState();
	simState->onlyEntities = onlyEntities;

	CUnitManager& unitMan = g_Game->GetWorld()->GetUnitManager();
	const std::vector<CUnit*>& units = unitMan.GetUnits();

	for (std::vector<CUnit*>::const_iterator unit = units.begin(); unit != units.end(); ++unit)
	{
		// Ignore objects that aren't entities
		if (! (*unit)->GetEntity())
			continue;

		Entity e = Entity::Freeze(*unit);
		simState->entities.push_back(e);
	}

	if (! onlyEntities)
	{
		for (std::vector<CUnit*>::const_iterator unit = units.begin(); unit != units.end(); ++unit)
		{
			// Ignore objects that are entities
			if ((*unit)->GetEntity())
				continue;

			Nonentity n = Nonentity::Freeze(*unit);
			simState->nonentities.push_back(n);
		}
	}
	
	return simState;
}

void SimState::Thaw()
{
	CUnitManager& unitMan = g_Game->GetWorld()->GetUnitManager();

	// delete all existing entities
	g_Game->GetWorld()->GetProjectileManager().DeleteAll();
	g_EntityManager.DeleteAll();

	if (! onlyEntities)
	{
		// delete all remaining non-entity units
		unitMan.DeleteAll();
		// don't reset the unitID counter - there's no need, since it'll work alright anyway
	}

	for (size_t i = 0; i < entities.size(); ++i)
		entities[i].Thaw();

	g_EntityManager.InitializeAll();

	if (! onlyEntities)
		for (size_t i = 0; i < nonentities.size(); ++i)
			nonentities[i].Thaw();
}
