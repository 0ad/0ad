/* Copyright (C) 2022 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "JSInterface_Main.h"

#include "graphics/FontMetrics.h"
#include "graphics/MapReader.h"
#include "lib/sysdep/sysdep.h"
#include "lib/utf8.h"
#include "maths/Size2D.h"
#include "maths/MD5.h"
#include "ps/CStrIntern.h"
#include "ps/GUID.h"
#include "ps/GameSetup/Atlas.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Util.h"
#include "scriptinterface/FunctionWrapper.h"
#include "tools/atlas/GameInterface/GameLoop.h"

extern void QuitEngine();
extern void StartAtlas();

namespace JSI_Main
{
void QuitEngine()
{
	::QuitEngine();
}

void StartAtlas()
{
	::StartAtlas();
}

bool AtlasIsAvailable()
{
	return ATLAS_IsAvailable();
}

bool IsAtlasRunning()
{
	return g_AtlasGameLoop && g_AtlasGameLoop->running;
}

void OpenURL(const std::string& url)
{
	sys_open_url(url);
}

std::wstring GetSystemUsername()
{
	return sys_get_user_name();
}

std::wstring GetMatchID()
{
	return ps_generate_guid().FromUTF8();
}

JS::Value LoadMapSettings(const ScriptInterface& scriptInterface, const VfsPath& pathname)
{
	ScriptRequest rq(scriptInterface);

	CMapSummaryReader reader;

	if (reader.LoadMap(pathname) != PSRETURN_OK)
		return JS::UndefinedValue();

	JS::RootedValue settings(rq.cx);
	reader.GetMapSettings(scriptInterface, &settings);
	return settings;
}

// This value is recalculated once a frame. We take special care to
// filter it, so it is both accurate and free of jitter.
int GetFps()
{
	if (!g_frequencyFilter)
		return 0;

	return g_frequencyFilter->StableFrequency();
}

CSize2D GetTextSize(const std::string& fontName, const std::wstring& text)
{
	int width = 0;
	int height = 0;
	CStrIntern _fontName(fontName);
	CFontMetrics fontMetrics(_fontName);
	fontMetrics.CalculateStringSize(text.c_str(), width, height);
	return CSize2D(width, height);
}

int GetTextWidth(const std::string& fontName, const std::wstring& text)
{
	return GetTextSize(fontName, text).Width;
}

std::string CalculateMD5(const std::string& input)
{
	u8 digest[MD5::DIGESTSIZE];

	MD5 m;
	m.Update((const u8*)input.c_str(), input.length());
	m.Final(digest);

	return Hexify(digest, MD5::DIGESTSIZE);
}

void RegisterScriptFunctions(const ScriptRequest& rq)
{
	ScriptFunction::Register<&QuitEngine>(rq, "Exit");
	ScriptFunction::Register<&StartAtlas>(rq, "RestartInAtlas");
	ScriptFunction::Register<&AtlasIsAvailable>(rq, "AtlasIsAvailable");
	ScriptFunction::Register<&IsAtlasRunning>(rq, "IsAtlasRunning");
	ScriptFunction::Register<&OpenURL>(rq, "OpenURL");
	ScriptFunction::Register<&GetSystemUsername>(rq, "GetSystemUsername");
	ScriptFunction::Register<&GetMatchID>(rq, "GetMatchID");
	ScriptFunction::Register<&LoadMapSettings>(rq, "LoadMapSettings");
	ScriptFunction::Register<&GetFps>(rq, "GetFPS");
	ScriptFunction::Register<&GetTextSize>(rq, "GetTextSize");
	ScriptFunction::Register<&GetTextWidth>(rq, "GetTextWidth");
	ScriptFunction::Register<&CalculateMD5>(rq, "CalculateMD5");
}
}
