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

#ifndef INCLUDED_GAMESETUP
#define INCLUDED_GAMESETUP

//
// GUI integration
//

// display progress / description in loading screen
extern void GUI_DisplayLoadProgress(int percent, const wchar_t* pending_task);

extern void Render();

/**
 * initialize global modules that are be needed before Init.
 * must be called from the very beginning of main.
 **/
extern void EarlyInit();

enum InitFlags
{
	// avoid setting a video mode / initializing OpenGL; assume that has
	// already been done and everything is ready for rendering.
	// needed by map editor because it creates its own window.
	INIT_HAVE_VMODE = 1,

	// skip initializing the in-game GUI.
	// needed by map editor because it uses its own GUI.
	INIT_NO_GUI = 2,

	// avoid setting display_error app hook
	// needed by map editor because it has its own wx error display
	INIT_HAVE_DISPLAY_ERROR = 4,

	// initialize the mod folders from command line parameters
	INIT_MODS = 8,

	// mount the public mod
	// needed by the map editor as "mod" does not provide everything it needs
	INIT_MODS_PUBLIC = 16
};

enum ShutdownFlags
{
	// start shutdown from config down
	// needed for loading mods as specified in the config
	// without having to go through a full init-shutdown cycle
	SHUTDOWN_FROM_CONFIG = 1
};

/**
 * enable/disable rendering of the GUI (intended mainly for screenshots)
 */
extern void RenderGui(bool RenderingState);
extern void RenderLogger(bool RenderingState);

/**
 * enable/disable rendering of the cursor - this does not hide cursor, but reverts to OS style
 */
extern void RenderCursor(bool RenderingState);


class CmdLineArgs;
class Paths;
extern std::vector<CStr>& GetMods(const CmdLineArgs& args, int flags);
extern void MountMods(const Paths& paths, const std::vector<CStr>& mods);
/**
 * Returns true if successful, false if mods changed and restart_engine was called.
 * In the latter case the caller should call Shutdown() with SHUTDOWN_FROM_CONFIG.
 */
extern bool Init(const CmdLineArgs& args, int flags);
extern void InitGraphics(const CmdLineArgs& args, int flags);
extern void Shutdown(int flags);
extern void CancelLoad(const CStrW& message);

extern bool InDevelopmentCopy();

#endif // INCLUDED_GAMESETUP
