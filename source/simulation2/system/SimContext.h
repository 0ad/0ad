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

#ifndef INCLUDED_SIMCONTEXT
#define INCLUDED_SIMCONTEXT

#include "Entity.h"

class CComponentManager;
class CUnitManager;
class CTerrain;
class ScriptInterface;

/**
 * Contains pointers to various 'global' objects that are needed by the simulation code,
 * to allow easy access without using real (evil) global variables.
 */
class CSimContext
{
public:
	CSimContext();
	~CSimContext();

	CComponentManager& GetComponentManager() const;
	void SetComponentManager(CComponentManager* man);

	bool HasUnitManager() const;
	CUnitManager& GetUnitManager() const;

	CTerrain& GetTerrain() const;

	ScriptInterface& GetScriptInterface() const;

	void SetSystemEntity(CEntityHandle ent) { m_SystemEntity = ent; }
	CEntityHandle GetSystemEntity() const { ASSERT(m_SystemEntity.GetId() == SYSTEM_ENTITY); return m_SystemEntity; }

	/**
	 * Returns the player ID that the current display is being rendered for.
	 * Currently relies on g_Game being initialised (evil globals...)
	 */
	int GetCurrentDisplayedPlayer() const;
	void SetCurrentDisplayedPlayer(int player);

private:
	CComponentManager* m_ComponentManager;
	CUnitManager* m_UnitManager;
	CTerrain* m_Terrain;

	CEntityHandle m_SystemEntity;

	int currentDisplayedPlayer;

	friend class CSimulation2Impl;
};

#endif // INCLUDED_SIMCONTEXT
