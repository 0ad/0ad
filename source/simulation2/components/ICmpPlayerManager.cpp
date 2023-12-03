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

#include "ICmpPlayerManager.h"

#include "simulation2/system/InterfaceScripted.h"
#include "simulation2/scripting/ScriptComponent.h"

BEGIN_INTERFACE_WRAPPER(PlayerManager)
END_INTERFACE_WRAPPER(PlayerManager)

class CCmpPlayerManagerScripted : public ICmpPlayerManager
{
public:
	DEFAULT_SCRIPT_WRAPPER(PlayerManagerScripted)

	int32_t GetNumPlayers() override
	{
		return m_Script.Call<int32_t>("GetNumPlayers");
	}

	entity_id_t GetPlayerByID(int32_t id) override
	{
		return m_Script.Call<entity_id_t>("GetPlayerByID", (int)id);
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(PlayerManagerScripted)
