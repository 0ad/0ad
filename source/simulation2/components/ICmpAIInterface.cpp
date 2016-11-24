/* Copyright (C) 2011 Wildfire Games.
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

#include "ICmpAIInterface.h"

#include "simulation2/system/InterfaceScripted.h"
#include "simulation2/scripting/ScriptComponent.h"

BEGIN_INTERFACE_WRAPPER(AIInterface)
END_INTERFACE_WRAPPER(AIInterface)

class CCmpAIInterfaceScripted : public ICmpAIInterface
{
public:
	DEFAULT_SCRIPT_WRAPPER(AIInterfaceScripted)

	virtual void GetRepresentation(JS::MutableHandleValue ret)
	{
		return m_Script.CallRef("GetRepresentation", ret);
	}
	virtual void GetFullRepresentation(JS::MutableHandleValue ret, bool flushEvents = false)
	{
		return m_Script.CallRef("GetFullRepresentation",flushEvents, ret);
	}

};

REGISTER_COMPONENT_SCRIPT_WRAPPER(AIInterfaceScripted)
