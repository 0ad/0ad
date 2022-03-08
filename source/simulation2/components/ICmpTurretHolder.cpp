/* Copyright (C) 2022 Wildfire Games.
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

#include "ICmpTurretHolder.h"

#include "simulation2/scripting/ScriptComponent.h"
#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(TurretHolder)
END_INTERFACE_WRAPPER(TurretHolder)

class CCmpTurretHolderScripted : public ICmpTurretHolder
{
public:
	DEFAULT_SCRIPT_WRAPPER(TurretHolderScripted)

	/**
	 * @return - Correlation between garrisoned turrets (their ID) and which
	 *	turret point they occupy (name).
	 */
	std::vector<std::pair<std::string, entity_id_t> > GetTurrets() const override
	{
		std::vector<std::pair<std::string, entity_id_t> > turrets;
		std::vector<entity_id_t> entities = m_Script.Call<std::vector<entity_id_t>>("GetEntities");
		for (entity_id_t entity : entities)
			turrets.push_back(std::make_pair(
				m_Script.Call<std::string>("GetOccupiedTurretPointName", entity),
				entity
			));

		return turrets;
	}

	/**
	 * Correlation between entities (ID) and the turret point they ought to occupy (name).
	 */
	void SetInitEntities(std::vector<std::pair<std::string, entity_id_t>>&& entities) override
	{
		for (const std::pair<std::string, entity_id_t>& p : entities)
			m_Script.CallVoid("SetInitEntity", p.first, p.second);
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(TurretHolderScripted)
