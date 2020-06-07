/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_PS_GAMESETUP_CONFIG
#define INCLUDED_PS_GAMESETUP_CONFIG

#include "ps/CStr.h"

// flag to pause the game on window focus loss
extern bool g_PauseOnFocusLoss;

extern int g_xres, g_yres;
extern float g_GuiScale;
extern bool g_VSync;

extern bool g_Quickstart;
extern bool g_DisableAudio;

extern CStrW g_CursorName;
extern const wchar_t g_DefaultCursor[];

class CmdLineArgs;
extern void CONFIG_Init(const CmdLineArgs& args);

#endif // INCLUDED_PS_GAMESETUP_CONFIG
