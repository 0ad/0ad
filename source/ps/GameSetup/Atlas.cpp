/* Copyright (C) 2014 Wildfire Games.
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

#include "Atlas.h"

#include "ps/GameSetup/CmdLineArgs.h"
#include "ps/DllLoader.h"

//----------------------------------------------------------------------------
// Atlas (map editor) integration
//----------------------------------------------------------------------------

DllLoader atlas_dll("AtlasUI", CLogger::Normal);

enum AtlasRunFlags
{
	// used by ATLAS_RunIfOnCmdLine; makes any error output go through
	// DEBUG_DISPLAY_ERROR rather than a GUI dialog box (because GUI init was
	// skipped to reduce load time).
	ATLAS_NO_GUI = 1
};

// starts the Atlas UI.
static void ATLAS_Run(const CmdLineArgs& args, int flags = 0)
{
	// first check if we can run at all
	if(!atlas_dll.LoadDLL())
	{
		if(flags & ATLAS_NO_GUI)
			DEBUG_DISPLAY_ERROR(L"The Atlas UI was not successfully loaded and therefore cannot be started as requested.");
		else
			DEBUG_DISPLAY_ERROR(L"The Atlas UI was not successfully loaded and therefore cannot be started as requested.");// TODO: implement GUI error message
		return;
	}

	// TODO (make nicer)
	extern bool BeginAtlas(const CmdLineArgs& args, const DllLoader& dll);
	if (!BeginAtlas(args, atlas_dll))
	{
		debug_warn(L"Atlas loading failed");
		return;
	}
}

bool ATLAS_IsAvailable()
{
	return atlas_dll.LoadDLL();
}

// starts the Atlas UI if an "-editor" switch is found on the command line.
// this is the alternative to starting the main menu and clicking on
// the editor button; it is much faster because it's called during early
// init and therefore skips GUI setup.
// notes:
// - GUI init still runs, but some GUI setup will be skipped since
//   ATLAS_IsRunning() will return true.
bool ATLAS_RunIfOnCmdLine(const CmdLineArgs& args, bool force)
{
	if (force || args.Has("editor"))
	{
		ATLAS_Run(args, ATLAS_NO_GUI);
		return true;
	}

	return false;
}
