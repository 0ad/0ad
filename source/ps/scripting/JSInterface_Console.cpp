/* Copyright (C) 2014 Wildfire Games.
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

#include "scriptinterface/ScriptInterface.h"
#include "JSInterface_Console.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"

bool JSI_Console::CheckGlobalInitialized()
{	
	if (!g_Console)
	{
		LOGERROR(L"Trying to access the console when it's not initialized!");
		return false;
	}
	return true;
}

bool JSI_Console::GetVisibleEnabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!CheckGlobalInitialized())
		return false;
	return g_Console->IsActive();
}

void JSI_Console::SetVisibleEnabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool Enabled)
{
	if (!CheckGlobalInitialized())
		return;
	g_Console->SetVisible(Enabled);
}

void JSI_Console::RegisterScriptFunctions(ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<bool, &JSI_Console::GetVisibleEnabled>("Console_GetVisibleEnabled");
	scriptInterface.RegisterFunction<void, bool, &JSI_Console::SetVisibleEnabled>("Console_SetVisibleEnabled");
}
