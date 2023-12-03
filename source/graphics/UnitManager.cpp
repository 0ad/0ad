/* Copyright (C) 2023 Wildfire Games.
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

#include "graphics/Model.h"
#include "graphics/ObjectEntry.h"
#include "graphics/ObjectManager.h"
#include "graphics/Unit.h"
#include "graphics/UnitManager.h"
#include "ps/Game.h"
#include "ps/World.h"

#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// CUnitManager constructor
CUnitManager::CUnitManager() :
	m_ObjectManager(NULL)
{
}

///////////////////////////////////////////////////////////////////////////////
// CUnitManager destructor
CUnitManager::~CUnitManager() = default;


///////////////////////////////////////////////////////////////////////////////
// AddUnit: add given unit to world
CUnit* CUnitManager::AddUnit(std::unique_ptr<CUnit> unit)
{
	return m_Units.emplace_back(std::move(unit)).get();
}


///////////////////////////////////////////////////////////////////////////////
// DeleteUnit: remove given unit from world and delete it
void CUnitManager::DeleteUnit(CUnit* unit)
{
	const auto it = std::find_if(m_Units.begin(), m_Units.end(), [&](const std::unique_ptr<CUnit>& elem)
		{
			return elem.get() == unit;
		});

	if (it != m_Units.end())
		m_Units.erase(it);
}

///////////////////////////////////////////////////////////////////////////////
// CreateUnit: create a new unit and add it to the world
CUnit* CUnitManager::CreateUnit(const CStrW& actorName, const entity_id_t id, const uint32_t seed)
{
	if (!m_ObjectManager)
		return nullptr;

	std::unique_ptr<CUnit> unit{CUnit::Create(actorName, id, seed, *m_ObjectManager)};
	if (!unit)
		return nullptr;

	return AddUnit(std::move(unit));
}

void CUnitManager::MakeTerrainDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1, int dirtyFlags)
{
	if (!(dirtyFlags & RENDERDATA_UPDATE_VERTICES))
		return;
	for (const std::unique_ptr<CUnit>& unit : m_Units)
		unit->GetModel().SetTerrainDirty(i0, j0, i1, j1);
}
