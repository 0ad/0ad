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

#ifndef INCLUDED_PS_GLOBALS
#define INCLUDED_PS_GLOBALS

#include "lib/input.h"
#include "lib/frequency_filter.h"

#include <map>

// thin abstraction layer on top of SDL.
// game code should use it instead of SDL_GetMouseState etc. because
// Atlas does not completely emulate SDL (it can only send events).

extern bool g_app_minimized;
extern bool g_app_has_focus;

extern int g_mouse_x, g_mouse_y;

/**
 * Indicates whether the mouse is focused on the game window (mouse positions
 * should usually be considered inaccurate if this is false)
 */
extern bool g_mouse_active;

/**
 * g_keys: Key states, indexed by SDLK* constants. If an entry is true,
 * it represents a pressed key.
 * Updated by GlobalsInputHandler in response to key press/release events.
 */
extern std::map<int32_t, bool> g_keys;

/**
 * g_mouse_buttons: Mouse buttons states, indexed by SDL_BUTTON_* constants.
 * If an entry is true, it represents a pressed button.
 * Updated by GlobalsInputHandler in response to mouse button up/down events.
 *
 * Be aware that SDL_BUTTON_* constants start at 1. Therefore,
 * g_mouse_buttons[0] is unused. The order of entries is:
 * { unused, left, right, middle, wheel up, wheel down }
 */
extern bool g_mouse_buttons[6];

extern InReaction GlobalsInputHandler(const SDL_Event_* ev);

extern PIFrequencyFilter g_frequencyFilter;

#endif // INCLUDED_PS_GLOBALS
