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

#include "Config.h"

#include "lib/timer.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/GameSetup/CmdLineArgs.h"

// (these variables are documented in the header.)
bool g_PauseOnFocusLoss = false;

int g_xres, g_yres;

bool g_Quickstart = false;
bool g_DisableAudio = false;

//----------------------------------------------------------------------------
// config
//----------------------------------------------------------------------------

// Fill in the globals from the config files.
static void LoadGlobals()
{
	CFG_GET_VAL("pauseonfocusloss", g_PauseOnFocusLoss);
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

	if (args.Has("rejointest"))
		g_ConfigDB.SetValueString(CFG_COMMAND, "rejointest", args.Get("rejointest"));
}


void CONFIG_Init(const CmdLineArgs& args)
{
	TIMER(L"CONFIG_Init");

	CConfigDB::Initialise();

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

	// Collect information from system.cfg, the profile file,
	// and any command-line overrides to fill in the globals.
	LoadGlobals();	// 64ms
}
