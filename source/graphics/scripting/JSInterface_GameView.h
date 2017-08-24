/* Copyright (C) 2017 Wildfire Games.
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


#ifndef INCLUDED_JSINTERFACE_GAMEVIEW
#define INCLUDED_JSINTERFACE_GAMEVIEW

#include "ps/CStr.h"
#include "scriptinterface/ScriptInterface.h"

#define DECLARE_BOOLEAN_SCRIPT_SETTING(NAME) \
	bool Get##NAME##Enabled(ScriptInterface::CxPrivate* pCxPrivate); \
	void Set##NAME##Enabled(ScriptInterface::CxPrivate* pCxPrivate, bool Enabled);

namespace JSI_GameView
{
	void RegisterScriptFunctions(const ScriptInterface& ScriptInterface);

	DECLARE_BOOLEAN_SCRIPT_SETTING(Culling);
	DECLARE_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
	DECLARE_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);
}

#undef DECLARE_BOOLEAN_SCRIPT_SETTING

#endif

