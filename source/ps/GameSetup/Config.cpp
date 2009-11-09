/* Copyright (C) 2009 Wildfire Games.
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

#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CConsole.h"
#include "ps/GameSetup/CmdLineArgs.h"
#include "lib/timer.h"
#include "lib/wchar.h"
#include "lib/res/sound/snd_mgr.h"
#include "Config.h"

#define LOG_CATEGORY L"config"


// (these variables are documented in the header.)

CStrW g_CursorName = L"test";
CStr g_ActiveProfile = "default";

bool g_NoGLS3TC = false;
bool g_NoGLAutoMipmap = false;
bool g_NoGLVBO = false;
bool g_NoGLFramebufferObject = false;

bool g_Shadows = false;
bool g_FancyWater = false;

bool g_FixedFrameTiming = false;

float g_LodBias = 0.0f;
float g_Gamma = 1.0f;

bool g_EntGraph = false;
CStr g_RenderPath = "default";

int g_xres, g_yres;
int g_bpp;
int g_freq;
bool g_VSync = false;

bool g_Quickstart = false;

// flag to switch on drawing terrain overlays
bool g_ShowPathfindingOverlay = false;

// flag to switch on triangulation pathfinding
bool g_TriPathfind = false;


// If non-empty, specified map will be automatically loaded
CStr g_AutostartMap = "";


//----------------------------------------------------------------------------
// config and profile
//----------------------------------------------------------------------------

static void LoadProfile( const CStr& profile )
{
	VfsPath path = VfsPath(L"profiles") / wstring_from_utf8(profile);

	VfsPath configFilename = path / L"settings/user.cfg";
	g_ConfigDB.SetConfigFile(CFG_USER, true, configFilename.string().c_str());
	g_ConfigDB.Reload(CFG_USER);

	int max_history_lines = 200;
	CFG_GET_USER_VAL("console.history.size", Int, max_history_lines);
	g_Console->UseHistoryFile(path / L"settings/history", max_history_lines);
}


// Fill in the globals from the config files.
static void LoadGlobals()
{
	CFG_GET_SYS_VAL("profile", String, g_ActiveProfile);

	// Now load the profile before trying to retrieve the values of the rest of these.

	LoadProfile( g_ActiveProfile );

	CFG_GET_USER_VAL("xres", Int, g_xres);
	CFG_GET_USER_VAL("yres", Int, g_yres);
	CFG_GET_USER_VAL("vsync", Bool, g_VSync);

	CFG_GET_USER_VAL("nos3tc", Bool, g_NoGLS3TC);
	CFG_GET_USER_VAL("noautomipmap", Bool, g_NoGLAutoMipmap);
	CFG_GET_USER_VAL("novbo", Bool, g_NoGLVBO);
	CFG_GET_USER_VAL("noframebufferobject", Bool, g_NoGLFramebufferObject);
	CFG_GET_USER_VAL("shadows", Bool, g_Shadows);
	CFG_GET_USER_VAL("fancywater", Bool, g_FancyWater);
	CFG_GET_USER_VAL("renderpath", String, g_RenderPath);

	CFG_GET_USER_VAL("lodbias", Float, g_LodBias);

	float gain = -1.0f;
	CFG_GET_USER_VAL("sound.mastergain", Float, gain);
	if(gain > 0.0f)
		WARN_ERR(snd_set_master_gain(gain));

	LOG(CLogger::Normal,  LOG_CATEGORY, L"g_x/yres is %dx%d", g_xres, g_yres);
	LOG(CLogger::Normal,  LOG_CATEGORY, L"Active profile is %hs", g_ActiveProfile.c_str());
}


static void ProcessCommandLineArgs(const CmdLineArgs& args)
{
	// TODO: all these options (and the ones processed elsewhere) should
	// be documented somewhere for users.

	if (args.Has("autostart"))
		 g_AutostartMap = args.Get("autostart");

	if (args.Has("buildarchive"))
	{
		// note: VFS init is sure to have been completed by now
		// (since CONFIG_Init reads from file); therefore,
		// it is safe to call this from here directly.
//		vfs_opt_rebuild_main_archive("mods/official/official1.zip", "../logs/trace.txt");
	}

	// Handle "-conf=key:value" (potentially multiple times)
	std::vector<CStr> conf = args.GetMultiple("conf");
	for (size_t i = 0; i < conf.size(); ++i)
	{
		CStr name_value = conf[i];
		if (name_value.Find(':') != -1)
		{
			CStr name = name_value.BeforeFirst(":");
			CStr value = name_value.AfterFirst(":");
			g_ConfigDB.CreateValue(CFG_COMMAND, name)->m_String = value;
		}
	}

	if (args.Has("entgraph"))
		g_EntGraph = true;

	if (args.Has("fixedframe"))
		g_FixedFrameTiming = true;

	if (args.Has("g"))
	{
		g_Gamma = (float)atof(args.Get("g"));
		if (g_Gamma == 0.0f)
			g_Gamma = 1.0f;
	}

//	if (args.Has("listfiles"))
//		trace_enable(true);

	if (args.Has("profile"))
		g_ConfigDB.CreateValue(CFG_COMMAND, "profile")->m_String = args.Get("profile");

	if (args.Has("quickstart"))
		g_Quickstart = true;

	if (args.Has("shadows"))
		g_ConfigDB.CreateValue(CFG_COMMAND, "shadows")->m_String = "true";

	if (args.Has("xres"))
		g_ConfigDB.CreateValue(CFG_COMMAND, "xres")->m_String = args.Get("xres");

	if (args.Has("yres"))
		g_ConfigDB.CreateValue(CFG_COMMAND, "yres")->m_String = args.Get("yres");

	if (args.Has("vsync"))
		g_ConfigDB.CreateValue(CFG_COMMAND, "vsync")->m_String = "true";

	if (args.Has("showOverlay"))
		g_ShowPathfindingOverlay = true;

	if (args.Has("triPathfind"))
		g_TriPathfind = true;
}


void CONFIG_Init(const CmdLineArgs& args)
{
	TIMER(L"CONFIG_Init");
	MICROLOG(L"init config");

	new CConfigDB;

	// Load the global, default config file
	g_ConfigDB.SetConfigFile(CFG_DEFAULT, false, L"config/default.cfg");
	g_ConfigDB.Reload(CFG_DEFAULT);	// 216ms
	// Try loading the local system config file (which doesn't exist by
	// default) - this is designed as a way of letting developers edit the
	// system config without accidentally committing their changes back to SVN.
	g_ConfigDB.SetConfigFile(CFG_SYSTEM, false, L"config/local.cfg");
	g_ConfigDB.Reload(CFG_SYSTEM);

	g_ConfigDB.SetConfigFile(CFG_MOD, true, L"config/mod.cfg");
	// No point in reloading mod.cfg here - we haven't mounted mods yet

	ProcessCommandLineArgs(args);

	// Collect information from system.cfg, the profile file,
	// and any command-line overrides to fill in the globals.
	LoadGlobals();	// 64ms
}
