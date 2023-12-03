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

#include "ICmpValueModificationManager.h"

#include "simulation2/system/InterfaceScripted.h"
#include "simulation2/scripting/ScriptComponent.h"

BEGIN_INTERFACE_WRAPPER(ValueModificationManager)
END_INTERFACE_WRAPPER(ValueModificationManager)

class CCmpValueModificationManagerScripted : public ICmpValueModificationManager
{
public:
	DEFAULT_SCRIPT_WRAPPER(ValueModificationManagerScripted)

	fixed ApplyModifications(std::wstring valueName, fixed currentValue, entity_id_t entity) const override
	{
		return m_Script.Call<fixed>("ApplyModifications", valueName, currentValue, entity);
	}

	u32 ApplyModifications(std::wstring valueName, u32 currentValue, entity_id_t entity) const override
	{
		return m_Script.Call<u32>("ApplyModifications", valueName, currentValue, entity);
	}

	u16 ApplyModifications(std::wstring valueName, u16 currentValue, entity_id_t entity) const override
	{
		return m_Script.Call<u16>("ApplyModifications", valueName, currentValue, entity);
	}

	std::wstring ApplyModifications(std::wstring valueName, std::wstring currentValue, entity_id_t entity) const override
	{
		return m_Script.Call<std::wstring>("ApplyModifications", valueName, currentValue, entity);
	}

	bool ApplyModifications(std::wstring valueName, bool currentValue, entity_id_t entity) const override
	{
		return m_Script.Call<bool>("ApplyModifications", valueName, currentValue, entity);
	}
};

REGISTER_COMPONENT_SCRIPT_WRAPPER(ValueModificationManagerScripted)
