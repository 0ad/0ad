/* Copyright (C) 2010 Wildfire Games.
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

#include "ICmpGuiInterface.h"

#include "simulation2/system/InterfaceScripted.h"
#include "simulation2/scripting/ScriptComponent.h"

BEGIN_INTERFACE_WRAPPER(GuiInterface)
END_INTERFACE_WRAPPER(GuiInterface)

class CCmpGuiInterfaceScripted : public ICmpGuiInterface
{
public:
	DEFAULT_SCRIPT_WRAPPER(GuiInterfaceScripted)

	virtual CScriptVal GetSimulationState(int player)
	{
		return m_Script.Call<CScriptVal> ("GetSimulationState", player);
	}

	virtual CScriptVal GetEntityState(int player, entity_id_t ent)
	{
		return m_Script.Call<CScriptVal> ("GetEntityState", player, ent);
	}

	virtual void SetSelectionHighlight(entity_id_t ent, const CColor& color)
	{
		m_Script.Call<CScriptVal> ("SetSelectionHighlight", ent, color);
		// ignore return value
	}

	virtual CScriptVal ScriptCall(std::string name, CScriptVal data)
	{
		return m_Script.Call<CScriptVal> ("ScriptCall", name, data);
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(GuiInterfaceScripted)
