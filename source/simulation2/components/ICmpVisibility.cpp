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

#include "ICmpVisibility.h"

#include "simulation2/scripting/ScriptComponent.h"
#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(Visibility)
END_INTERFACE_WRAPPER(Visibility)

class CCmpVisibilityScripted : public ICmpVisibility
{
public:
	DEFAULT_SCRIPT_WRAPPER(VisibilityScripted)

	bool IsActivated() override
	{
		return m_Script.Call<bool>("IsActivated");
	}

	LosVisibility GetVisibility(player_id_t player, bool isVisible, bool isExplored) override
	{
		int visibility = m_Script.Call<int, player_id_t, bool, bool>("GetVisibility", player, isVisible, isExplored);

		switch (visibility)
		{
		case static_cast<int>(LosVisibility::HIDDEN):
			return LosVisibility::HIDDEN;
			case static_cast<int>(LosVisibility::FOGGED):
			return LosVisibility::FOGGED;
		case static_cast<int>(LosVisibility::VISIBLE):
			return LosVisibility::VISIBLE;
		default:
			LOGERROR("Received the invalid visibility value %d from the Visibility scripted component!", visibility);
			return LosVisibility::HIDDEN;
		}
	}

	bool GetRetainInFog() override
	{
		return m_Script.Call<bool>("GetRetainInFog");
	}

	bool GetAlwaysVisible() override
	{
		return m_Script.Call<bool>("GetAlwaysVisible");
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(VisibilityScripted)
