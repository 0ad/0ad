/* Copyright (C) 2012 Wildfire Games.
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

#include "Config.h"

#include "ps/ConfigDB.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "lib/timer.h"
#include "soundmanager/ISoundManager.h"

// (these variables are documented in the header.)

CStrW g_CursorName = L"test";

bool g_NoGLS3TC = false;
bool g_NoGLAutoMipmap = false;
bool g_NoGLVBO = false;

bool g_PauseOnFocusLoss = false;

bool g_RenderActors = true;

bool g_Shadows = false;
bool g_ShadowPCF = false;

bool g_WaterUgly = false;
bool g_WaterFancyEffects = false;
bool g_WaterRealDepth = false;
bool g_WaterRefraction = false;
bool g_WaterReflection = false;
bool g_WaterShadows = false;

bool g_Particles = false;
bool g_Silhouettes = false;
bool g_ShowSky = false;

float g_Gamma = 1.0f;

CStr g_RenderPath = "default";

int g_xres, g_yres;
bool g_VSync = false;

bool g_Quickstart = false;
bool g_DisableAudio = false;

bool g_JSDebuggerEnabled = false;
bool g_ScriptProfilingEnabled = false;

// flag to switch on drawing terrain overlays
bool g_ShowPathfindingOverlay = false;

// flag to switch on triangulation pathfinding
bool g_TriPathfind = false;


// If non-empty, specified map will be automatically loaded
CStr g_AutostartMap = "";


//----------------------------------------------------------------------------
// config
//----------------------------------------------------------------------------

// Fill in the globals from the config files.
static void LoadGlobals()
{
	CFG_GET_VAL("vsync", Bool, g_VSync);

	CFG_GET_VAL("nos3tc", Bool, g_NoGLS3TC);
	CFG_GET_VAL("noautomipmap", Bool, g_NoGLAutoMipmap);
	CFG_GET_VAL("novbo", Bool, g_NoGLVBO);
	CFG_GET_VAL("pauseonfocusloss", Bool, g_PauseOnFocusLoss);
	CFG_GET_VAL("renderactors", Bool, g_RenderActors);
	CFG_GET_VAL("shadows", Bool, g_Shadows);
	CFG_GET_VAL("shadowpcf", Bool, g_ShadowPCF);

	CFG_GET_VAL("waterugly",Bool, g_WaterUgly);
	CFG_GET_VAL("waterfancyeffects",Bool, g_WaterFancyEffects);
	CFG_GET_VAL("waterrealdepth",Bool, g_WaterRealDepth);
	CFG_GET_VAL("waterrefraction",Bool, g_WaterRefraction);
	CFG_GET_VAL("waterreflection",Bool, g_WaterReflection);
	CFG_GET_VAL("shadowsonwater",Bool, g_WaterShadows);

	CFG_GET_VAL("renderpath", String, g_RenderPath);
	CFG_GET_VAL("particles", Bool, g_Particles);
	CFG_GET_VAL("silhouettes", Bool, g_Silhouettes);
	CFG_GET_VAL("showsky", Bool, g_ShowSky);

	if (g_SoundManager)
	{
		float gain = 0.5f;
		float musicGain = 0.5f;
		float ambientGain = 0.5f;
		float actionGain = 0.5f;
		float uiGain = 0.5f;

		CFG_GET_VAL("sound.mastergain", Float, gain);
		CFG_GET_VAL("sound.musicgain", Float, musicGain);
		CFG_GET_VAL("sound.ambientgain", Float, ambientGain);
		CFG_GET_VAL("sound.actiongain", Float, actionGain);
		CFG_GET_VAL("sound.uigain", Float, uiGain);

	g_SoundManager->SetMasterGain(gain);
	g_SoundManager->SetMusicGain(musicGain);
	g_SoundManager->SetAmbientGain(ambientGain);
	g_SoundManager->SetActionGain(actionGain);
	g_SoundManager->SetUIGain(uiGain);
	}


	CFG_GET_VAL("jsdebugger.enable", Bool, g_JSDebuggerEnabled);
	CFG_GET_VAL("profiler2.script.enable", Bool, g_ScriptProfilingEnabled);

	if (g_JSDebuggerEnabled)
		LOGERROR(L"JS debugger temporarily disabled during the SpiderMonkey upgrade (check trac ticket #2348 for details)");
	// Script Debugging and profiling does not make sense together because of the hooks
	// that reduce performance a lot - and it wasn't tested if it even works together.
	if (g_JSDebuggerEnabled && g_ScriptProfilingEnabled)
		LOGERROR(L"Enabling both script profiling and script debugging is not supported!");
}


static void ProcessCommandLineArgs(const CmdLineArgs& args)
{
	// TODO: all these options (and the ones processed elsewhere) should
	// be documented somewhere for users.

	// Handle "-conf=key:value" (potentially multiple times)
	std::vector<CStr> conf = args.GetMultiple("conf");
	for (size_t i = 0; i < conf.size(); ++i)
	{
		CStr name_value = conf[i];
		if (name_value.Find(':') != -1)
		{
			CStr name = name_value.BeforeFirst(":");
			CStr value = name_value.AfterFirst(":");
			g_ConfigDB.SetValueString(CFG_COMMAND, name, value);
		}
	}

	if (args.Has("g"))
	{
		g_Gamma = args.Get("g").ToFloat();
		if (g_Gamma == 0.0f)
			g_Gamma = 1.0f;
	}

//	if (args.Has("listfiles"))
//		trace_enable(true);

	if (args.Has("profile"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "profile", args.Get("profile"));

	if (args.Has("quickstart"))
	{
		g_Quickstart = true;
		g_DisableAudio = true; // do this for backward-compatibility with user expectations
	}

	if (args.Has("nosound"))
		g_DisableAudio = true;

	if (args.Has("shadows"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "shadows", "true");

	if (args.Has("xres"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "xres", args.Get("xres"));

	if (args.Has("yres"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "yres", args.Get("yres"));

	if (args.Has("vsync"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "vsync", "true");

	if (args.Has("ooslog"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "ooslog", "true");

	if (args.Has("serializationtest"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "serializationtest", "true");
}


void CONFIG_Init(const CmdLineArgs& args)
{
	TIMER(L"CONFIG_Init");

	new CConfigDB;

	// Load the global, default config file
	g_ConfigDB.SetConfigFile(CFG_DEFAULT, L"config/default.cfg");
	g_ConfigDB.Reload(CFG_DEFAULT);	// 216ms
	// Try loading the local system config file (which doesn't exist by
	// default) - this is designed as a way of letting developers edit the
	// system config without accidentally committing their changes back to SVN.
	g_ConfigDB.SetConfigFile(CFG_SYSTEM, L"config/local.cfg");
	g_ConfigDB.Reload(CFG_SYSTEM);

	g_ConfigDB.SetConfigFile(CFG_USER, L"config/user.cfg");
	g_ConfigDB.Reload(CFG_USER);

	g_ConfigDB.SetConfigFile(CFG_MOD, L"config/mod.cfg");
	// No point in reloading mod.cfg here - we haven't mounted mods yet

	ProcessCommandLineArgs(args);

	// Initialise console history file
	int max_history_lines = 200;
	CFG_GET_VAL("console.history.size", Int, max_history_lines);
	g_Console->UseHistoryFile(L"config/console.txt", max_history_lines);

	// Collect information from system.cfg, the profile file,
	// and any command-line overrides to fill in the globals.
	LoadGlobals();	// 64ms
}
