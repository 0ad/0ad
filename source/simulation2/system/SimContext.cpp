/* Copyright (C) 2016 Wildfire Games.
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

#include "SimContext.h"

#include "ComponentManager.h"

#include "ps/Game.h"

CSimContext::CSimContext() :
	m_ComponentManager(NULL), m_UnitManager(NULL), m_Terrain(NULL)
{
}

CSimContext::~CSimContext()
{
}

CComponentManager& CSimContext::GetComponentManager() const
{
	return *m_ComponentManager;
}

bool CSimContext::HasUnitManager() const
{
	return m_UnitManager != NULL;
}

CUnitManager& CSimContext::GetUnitManager() const
{
	ENSURE(m_UnitManager);
	return *m_UnitManager;
}

CTerrain& CSimContext::GetTerrain() const
{
	ENSURE(m_Terrain);
	return *m_Terrain;
}

void CSimContext::SetComponentManager(CComponentManager* man)
{
	ENSURE(!m_ComponentManager);
	m_ComponentManager = man;
}

ScriptInterface& CSimContext::GetScriptInterface() const
{
	return GetComponentManager().GetScriptInterface();
}

int CSimContext::GetCurrentDisplayedPlayer() const
{
	return g_Game ? g_Game->GetViewedPlayerID() : -1;
}
