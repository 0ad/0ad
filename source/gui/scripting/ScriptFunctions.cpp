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

#include "graphics/FontMetrics.h"
#include "graphics/GameView.h"
#include "graphics/MapReader.h"
#include "graphics/scripting/JSInterface_GameView.h"
#include "gui/IGUIObject.h"
#include "gui/scripting/JSInterface_GUIManager.h"
#include "gui/scripting/JSInterface_GUITypes.h"
#include "i18n/scripting/JSInterface_L10n.h"
#include "lib/sysdep/sysdep.h"
#include "lib/utf8.h"
#include "lobby/scripting/JSInterface_Lobby.h"
#include "network/scripting/JSInterface_Network.h"
#include "ps/GUID.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/Globals.h"	// g_frequencyFilter
#include "ps/Hotkey.h"
#include "ps/scripting/JSInterface_ConfigDB.h"
#include "ps/scripting/JSInterface_Console.h"
#include "ps/scripting/JSInterface_Debug.h"
#include "ps/scripting/JSInterface_Game.h"
#include "ps/scripting/JSInterface_Mod.h"
#include "ps/scripting/JSInterface_SavedGame.h"
#include "ps/scripting/JSInterface_VFS.h"
#include "ps/scripting/JSInterface_VisualReplay.h"
#include "renderer/scripting/JSInterface_Renderer.h"
#include "simulation2/scripting/JSInterface_Simulation.h"
#include "soundmanager/scripting/JSInterface_Sound.h"
#include "tools/atlas/GameInterface/GameLoop.h"

/*
 * This file defines a set of functions that are available to GUI scripts, to allow
 * interaction with the rest of the engine.
 * Functions are exposed to scripts within the global object 'Engine', so
 * scripts should call "Engine.FunctionName(...)" etc.
 */

extern void restart_mainloop_in_atlas(); // from main.cpp
extern void EndGame();
extern void kill_mainloop();

namespace {

void OpenURL(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& url)
{
	sys_open_url(url);
}

std::wstring GetMatchID(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return ps_generate_guid().FromUTF8();
}

void RestartInAtlas(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	restart_mainloop_in_atlas();
}

bool AtlasIsAvailable(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return ATLAS_IsAvailable();
}

bool IsAtlasRunning(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_AtlasGameLoop && g_AtlasGameLoop->running;
}

JS::Value LoadMapSettings(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& pathname)
{
	JSContext* cx = pCxPrivate->pScriptInterface->GetContext();
	JSAutoRequest rq(cx);

	CMapSummaryReader reader;

	if (reader.LoadMap(pathname) != PSRETURN_OK)
		return JS::UndefinedValue();

	JS::RootedValue settings(cx);
	reader.GetMapSettings(*(pCxPrivate->pScriptInterface), &settings);
	return settings;
}

bool HotkeyIsPressed_(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& hotkeyName)
{
	return HotkeyIsPressed(hotkeyName);
}

void Script_EndGame(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	EndGame();
}

CStrW GetSystemUsername(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return sys_get_user_name();
}

void ExitProgram(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	kill_mainloop();
}

// - This value is recalculated once a frame. We take special care to
//   filter it, so it is both accurate and free of jitter.
int GetFps(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	int freq = 0;
	if (g_frequencyFilter)
		freq = g_frequencyFilter->StableFrequency();
	return freq;
}

int GetTextWidth(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const CStr& fontName, const CStrW& text)
{
	int width = 0;
	int height = 0;
	CStrIntern _fontName(fontName);
	CFontMetrics fontMetrics(_fontName);
	fontMetrics.CalculateStringSize(text.c_str(), width, height);
	return width;
}

} // namespace

void GuiScriptingInit(ScriptInterface& scriptInterface)
{
	JSI_IGUIObject::init(scriptInterface);
	JSI_GUITypes::init(scriptInterface);

	JSI_GameView::RegisterScriptFunctions(scriptInterface);
	JSI_Renderer::RegisterScriptFunctions(scriptInterface);
	JSI_Console::RegisterScriptFunctions(scriptInterface);
	JSI_ConfigDB::RegisterScriptFunctions(scriptInterface);
	JSI_Debug::RegisterScriptFunctions(scriptInterface);
	JSI_Game::RegisterScriptFunctions(scriptInterface);
	JSI_GUIManager::RegisterScriptFunctions(scriptInterface);
	JSI_Mod::RegisterScriptFunctions(scriptInterface);
	JSI_Network::RegisterScriptFunctions(scriptInterface);
	JSI_SavedGame::RegisterScriptFunctions(scriptInterface);
	JSI_Sound::RegisterScriptFunctions(scriptInterface);
	JSI_Simulation::RegisterScriptFunctions(scriptInterface);
	JSI_L10n::RegisterScriptFunctions(scriptInterface);
	JSI_Lobby::RegisterScriptFunctions(scriptInterface);
	JSI_VFS::RegisterScriptFunctions(scriptInterface);
	JSI_VisualReplay::RegisterScriptFunctions(scriptInterface);

	scriptInterface.RegisterFunction<void, &Script_EndGame>("EndGame");
	scriptInterface.RegisterFunction<void, std::string, &OpenURL>("OpenURL");
	scriptInterface.RegisterFunction<std::wstring, &GetMatchID>("GetMatchID");
	scriptInterface.RegisterFunction<void, &RestartInAtlas>("RestartInAtlas");
	scriptInterface.RegisterFunction<bool, &AtlasIsAvailable>("AtlasIsAvailable");
	scriptInterface.RegisterFunction<bool, &IsAtlasRunning>("IsAtlasRunning");
	scriptInterface.RegisterFunction<JS::Value, VfsPath, &LoadMapSettings>("LoadMapSettings");
	scriptInterface.RegisterFunction<bool, std::string, &HotkeyIsPressed_>("HotkeyIsPressed");
	scriptInterface.RegisterFunction<void, &ExitProgram>("Exit");
	scriptInterface.RegisterFunction<int, &GetFps>("GetFPS");
	scriptInterface.RegisterFunction<int, CStr, CStrW, &GetTextWidth>("GetTextWidth");
	scriptInterface.RegisterFunction<CStrW, &GetSystemUsername>("GetSystemUsername");
}
