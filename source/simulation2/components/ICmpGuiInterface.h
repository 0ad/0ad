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

#ifndef INCLUDED_ICMPGUIINTERFACE
#define INCLUDED_ICMPGUIINTERFACE

#include "simulation2/system/Interface.h"

struct CColor;

class ICmpGuiInterface : public IComponent
{
public:
	/**
	 * Generic call function, for use by GUI scripts to talk to the GuiInterface script.
	 */
	virtual void ScriptCall(int player, const std::wstring& cmd, JS::HandleValue data, JS::MutableHandleValue ret) = 0;
	// TODO: some of the earlier functions should just use ScriptCall.

	DECLARE_INTERFACE_TYPE(GuiInterface)
};

#endif // INCLUDED_ICMPGUIINTERFACE
