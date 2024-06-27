/* Copyright (C) 2024 Wildfire Games.
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

#include "ps/CStr.h"

#include <vector>

class CmdLineArgs;
class Paths;
class ScriptContext;
class ScriptInterface;

/**
 * initialize global modules that are be needed before Init.
 * must be called from the very beginning of main.
 **/
extern void EarlyInit();

extern void EndGame();

enum InitFlags
{
	// avoid setting a video mode / initializing OpenGL; assume that has
	// already been done and everything is ready for rendering.
	// needed by map editor because it creates its own window.
	INIT_HAVE_VMODE = 1,

	// skip initializing the in-game GUI.
	// needed by map editor because it uses its own GUI.
	INIT_NO_GUI = 2,

	// initialize the mod folders from command line parameters
	INIT_MODS = 8,

	// mount the public mod
	// needed by the map editor as "mod" does not provide everything it needs
	INIT_MODS_PUBLIC = 16
};

extern const std::vector<CStr>& GetMods(const CmdLineArgs& args, int flags);

/**
 * Mounts all files of the given mods in the global VFS.
 * Make sure to call CacheEnabledModVersions after every call to this.
 */
extern void MountMods(const Paths& paths, const std::vector<CStr>& mods);

void InitVfs(const CmdLineArgs& args);

/**
 * Returns true if successful, false if Init is aborted early (for instance if
 * mods changed, or if we are using -dumpSchema).
 * `ShutdownConfigAndSubsequent` has to be called later.
 */
extern bool Init(const CmdLineArgs& args, int flags);
extern void InitInput();

/**
 * `ShutdownNetworkAndUI` has to be called later.
 */
void InitGraphics(const CmdLineArgs& args, int flags, const std::vector<CStr>& installedMods,
	ScriptContext& scriptContext, ScriptInterface& scriptInterface);

/**
 * `ShutdownNetworkAndUI` has to be called later.
 */
extern bool InitNonVisual(const CmdLineArgs& args);

/**
 * Has to be called before `ShutdownConfigAndSubsequent`.
 */
void ShutdownNetworkAndUI();
void ShutdownConfigAndSubsequent();

extern void CancelLoad(const CStrW& message);

extern bool InDevelopmentCopy();

#endif // INCLUDED_GAMESETUP
