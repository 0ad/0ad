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

#ifndef INCLUDED_ATLAS
#define INCLUDED_ATLAS

class CmdLineArgs;

// free reference to Atlas UI SO (avoids resource leak report)
extern void ATLAS_Shutdown();

// returns whether the Atlas UI SO is available for loading
extern bool ATLAS_IsAvailable();

// starts the Atlas UI if an "-editor" switch is found on the command line.
// this is the alternative to starting the main menu and clicking on
// the editor button; it is much faster because it's called during early
// init and therefore skips GUI setup.
extern bool ATLAS_RunIfOnCmdLine(const CmdLineArgs& args, bool force);

#endif // INCLUDED_ATLAS
