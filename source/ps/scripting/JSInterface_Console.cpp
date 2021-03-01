/* Copyright (C) 2021 Wildfire Games.
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

#include "JSInterface_Console.h"

#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "scriptinterface/FunctionWrapper.h"

namespace
{
CConsole* ConsoleGetter(const ScriptRequest&, JS::CallArgs&)
{
	if (!g_Console)
	{
		LOGERROR("Trying to access the console when it's not initialized!");
		return nullptr;
	}
	return g_Console;
}
}

void JSI_Console::RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&CConsole::IsActive, ConsoleGetter>(rq, "Console_GetVisibleEnabled");
	ScriptFunction::Register<&CConsole::SetVisible, ConsoleGetter>(rq, "Console_SetVisibleEnabled");
}
