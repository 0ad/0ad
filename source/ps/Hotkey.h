/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_HOTKEY
#define INCLUDED_HOTKEY

/**
 * @file
 * Hotkey system.
 *
 * Hotkeys consist of a name (an arbitrary string), and a key mapping.
 * The names and mappings are loaded from the config system (any
 * config setting with the name prefix "hotkey.").
 * When a hotkey is pressed or released, SDL_HOTKEYDOWN and SDL_HOTKEYUP
 * events are triggered, with the hotkey name stored in ev.user.data1
 * as a const char*.
 */

#include "CStr.h"
#include "lib/input.h"
#include "lib/external_libraries/libsdl.h"	// see note below

// note: we need the real SDL header - it defines SDL_USEREVENT, which is
// required for our HOTKEY event type definition. this is OK since
// hotkey.h is not included from any headers.

const int SDL_HOTKEYDOWN = SDL_USEREVENT;
const int SDL_HOTKEYUP = SDL_USEREVENT + 1;

extern void LoadHotkeys();
extern void UnloadHotkeys();

extern InReaction HotkeyStateChange(const SDL_Event_* ev);
extern InReaction HotkeyInputHandler(const SDL_Event_* ev);

extern bool HotkeyIsPressed(const CStr& keyname);

#endif // INCLUDED_HOTKEY
