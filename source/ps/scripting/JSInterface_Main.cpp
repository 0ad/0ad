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

#include "JSInterface_Main.h"

#include "graphics/FontMetrics.h"
#include "graphics/MapReader.h"
#include "lib/sysdep/sysdep.h"
#include "lib/utf8.h"
#include "ps/CStrIntern.h"
#include "ps/GUID.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "tools/atlas/GameInterface/GameLoop.h"

extern void restart_mainloop_in_atlas();
extern void kill_mainloop();

void JSI_Main::ExitProgram(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	kill_mainloop();
}

void JSI_Main::RestartInAtlas(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	restart_mainloop_in_atlas();
}

bool JSI_Main::AtlasIsAvailable(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return ATLAS_IsAvailable();
}

bool JSI_Main::IsAtlasRunning(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return g_AtlasGameLoop && g_AtlasGameLoop->running;
}

void JSI_Main::OpenURL(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& url)
{
	sys_open_url(url);
}

std::wstring JSI_Main::GetSystemUsername(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return sys_get_user_name();
}

std::wstring JSI_Main::GetMatchID(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	return ps_generate_guid().FromUTF8();
}

JS::Value JSI_Main::LoadMapSettings(ScriptInterface::CxPrivate* pCxPrivate, const VfsPath& pathname)
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

bool JSI_Main::HotkeyIsPressed_(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& hotkeyName)
{
	return HotkeyIsPressed(hotkeyName);
}

// This value is recalculated once a frame. We take special care to
// filter it, so it is both accurate and free of jitter.
int JSI_Main::GetFps(ScriptInterface::CxPrivate* UNUSED(pCxPrivate))
{
	if (!g_frequencyFilter)
		return 0;

	return g_frequencyFilter->StableFrequency();
}

int JSI_Main::GetTextWidth(ScriptInterface::CxPrivate* UNUSED(pCxPrivate), const std::string& fontName, const std::wstring& text)
{
	int width = 0;
	int height = 0;
	CStrIntern _fontName(fontName);
	CFontMetrics fontMetrics(_fontName);
	fontMetrics.CalculateStringSize(text.c_str(), width, height);
	return width;
}

void JSI_Main::RegisterScriptFunctions(const ScriptInterface& scriptInterface)
{
	scriptInterface.RegisterFunction<void, &ExitProgram>("Exit");
	scriptInterface.RegisterFunction<void, &RestartInAtlas>("RestartInAtlas");
	scriptInterface.RegisterFunction<bool, &AtlasIsAvailable>("AtlasIsAvailable");
	scriptInterface.RegisterFunction<bool, &IsAtlasRunning>("IsAtlasRunning");
	scriptInterface.RegisterFunction<void, std::string, &OpenURL>("OpenURL");
	scriptInterface.RegisterFunction<std::wstring, &GetSystemUsername>("GetSystemUsername");
	scriptInterface.RegisterFunction<std::wstring, &GetMatchID>("GetMatchID");
	scriptInterface.RegisterFunction<JS::Value, VfsPath, &LoadMapSettings>("LoadMapSettings");
	scriptInterface.RegisterFunction<bool, std::string, &HotkeyIsPressed_>("HotkeyIsPressed");
	scriptInterface.RegisterFunction<int, &GetFps>("GetFPS");
	scriptInterface.RegisterFunction<int, std::string, std::wstring, &GetTextWidth>("GetTextWidth");
}
