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

#include "precompiled.h"

#include "JSInterface_GameView.h"

#include "graphics/GameView.h"
#include "ps/Game.h"
#include "ps/CLogger.h"
#include "ps/Profile.h"
#include "scriptinterface/ScriptInterface.h"

#define IMPLEMENT_BOOLEAN_SCRIPT_SETTING(NAME) \
bool JSI_GameView::Get##NAME##Enabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate)) \
{ \
	if (!g_Game || !g_Game->GetView()) \
	{ \
		LOGERROR("Trying to get a setting from GameView when it's not initialized!"); \
		return false; \
	} \
	return g_Game->GetView()->Get##NAME##Enabled(); \
} \
\
void JSI_GameView::Set##NAME##Enabled(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), bool Enabled) \
{ \
	if (!g_Game || !g_Game->GetView()) \
	{ \
		LOGERROR("Trying to set a setting of GameView when it's not initialized!"); \
		return; \
	} \
	g_Game->GetView()->Set##NAME##Enabled(Enabled); \
}

IMPLEMENT_BOOLEAN_SCRIPT_SETTING(Culling);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
IMPLEMENT_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);

#undef IMPLEMENT_BOOLEAN_SCRIPT_SETTING


#define REGISTER_BOOLEAN_SCRIPT_SETTING(NAME) \
scriptInterface.RegisterFunction<bool, &JSI_GameView::Get##NAME##Enabled>("GameView_Get" #NAME "Enabled"); \
scriptInterface.RegisterFunction<void, bool, &JSI_GameView::Set##NAME##Enabled>("GameView_Set" #NAME "Enabled");

void JSI_GameView::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	REGISTER_BOOLEAN_SCRIPT_SETTING(Culling);
	REGISTER_BOOLEAN_SCRIPT_SETTING(LockCullCamera);
	REGISTER_BOOLEAN_SCRIPT_SETTING(ConstrainCamera);
}

#undef REGISTER_BOOLEAN_SCRIPT_SETTING



