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

#include "scriptinterface/ScriptInterface.h"

#include "graphics/scripting/JSInterface_GameView.h"
#include "gui/IGUIObject.h"
#include "gui/scripting/JSInterface_GUIManager.h"
#include "gui/scripting/JSInterface_GUITypes.h"
#include "i18n/scripting/JSInterface_L10n.h"
#include "lobby/scripting/JSInterface_Lobby.h"
#include "network/scripting/JSInterface_Network.h"
#include "ps/scripting/JSInterface_ConfigDB.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_Debug.h"
#include "ps/scripting/JSInterface_Game.h"
#include "ps/scripting/JSInterface_Main.h"
#include "ps/scripting/JSInterface_Mod.h"
#include "ps/scripting/JSInterface_SavedGame.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "ps/scripting/JSInterface_VisualReplay.h"
#include "renderer/scripting/JSInterface_Renderer.h"
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
	JSI_IGUIObject::init(scriptInterface);
	JSI_GUITypes::init(scriptInterface);

	JSI_ConfigDB::RegisterScriptFunctions(scriptInterface);
	JSI_Console::RegisterScriptFunctions(scriptInterface);
	JSI_Debug::RegisterScriptFunctions(scriptInterface);
	JSI_GUIManager::RegisterScriptFunctions(scriptInterface);
	JSI_Game::RegisterScriptFunctions(scriptInterface);
	JSI_GameView::RegisterScriptFunctions(scriptInterface);
	JSI_L10n::RegisterScriptFunctions(scriptInterface);
	JSI_Lobby::RegisterScriptFunctions(scriptInterface);
	JSI_Main::RegisterScriptFunctions(scriptInterface);
	JSI_Mod::RegisterScriptFunctions(scriptInterface);
	JSI_Network::RegisterScriptFunctions(scriptInterface);
	JSI_Renderer::RegisterScriptFunctions(scriptInterface);
	JSI_SavedGame::RegisterScriptFunctions(scriptInterface);
	JSI_Simulation::RegisterScriptFunctions(scriptInterface);
	JSI_Sound::RegisterScriptFunctions(scriptInterface);
	JSI_VFS::RegisterScriptFunctions_GUI(scriptInterface);
	JSI_VisualReplay::RegisterScriptFunctions(scriptInterface);
}
