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

#include "ScriptFunctions.h"

#include "graphics/scripting/JSInterface_GameView.h"
#include "gui/Scripting/JSInterface_GUIManager.h"
#include "gui/Scripting/JSInterface_GUISize.h"
#include "i18n/scripting/JSInterface_L10n.h"
#include "lobby/scripting/JSInterface_Lobby.h"
#include "network/scripting/JSInterface_Network.h"
#include "ps/scripting/JSInterface_ConfigDB.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_Debug.h"
#include "ps/scripting/JSInterface_Game.h"
#include "ps/scripting/JSInterface_Hotkey.h"
#include "ps/scripting/JSInterface_Main.h"
#include "ps/scripting/JSInterface_Mod.h"
#include "ps/scripting/JSInterface_ModIo.h"
#include "ps/scripting/JSInterface_SavedGame.h"
#include "ps/scripting/JSInterface_UserReport.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "ps/scripting/JSInterface_VisualReplay.h"
#include "renderer/scripting/JSInterface_Renderer.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/scripting/JSInterface_Simulation.h"
#include "soundmanager/scripting/JSInterface_Sound.h"

/*
 * This file defines a set of functions that are available to GUI scripts, to allow
 * interaction with the rest of the engine.
 * Functions are exposed to scripts within the global object 'Engine', so
 * scripts should call "Engine.FunctionName(...)" etc.
 */
void GuiScriptingInit(ScriptInterface& scriptInterface)
{
	ScriptRequest rq(scriptInterface);

	JSI_GUISize::RegisterScriptClass(scriptInterface);
	JSI_ConfigDB::RegisterScriptFunctions(rq);
	JSI_Console::RegisterScriptFunctions(rq);
	JSI_Debug::RegisterScriptFunctions(rq);
	JSI_GUIManager::RegisterScriptFunctions(rq);
	JSI_Game::RegisterScriptFunctions(rq);
	JSI_GameView::RegisterScriptFunctions(rq);
	JSI_Hotkey::RegisterScriptFunctions(rq);
	JSI_L10n::RegisterScriptFunctions(rq);
	JSI_Lobby::RegisterScriptFunctions(rq);
	JSI_Main::RegisterScriptFunctions(rq);
	JSI_Mod::RegisterScriptFunctions(rq);
	JSI_ModIo::RegisterScriptFunctions(rq);
	JSI_Network::RegisterScriptFunctions(rq);
	JSI_Renderer::RegisterScriptFunctions(rq);
	JSI_SavedGame::RegisterScriptFunctions(rq);
	JSI_Simulation::RegisterScriptFunctions(rq);
	JSI_Sound::RegisterScriptFunctions(rq);
	JSI_UserReport::RegisterScriptFunctions(rq);
	JSI_VFS::RegisterScriptFunctions_ReadWriteAnywhere(rq);
	JSI_VisualReplay::RegisterScriptFunctions(rq);
}
