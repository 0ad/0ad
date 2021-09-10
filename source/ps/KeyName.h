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

#ifndef INCLUDED_KEYNAME
#define INCLUDED_KEYNAME

// Need SDLK_* enum values.
#include "lib/external_libraries/libsdl.h"
#include "ps/CStr.h"

extern SDL_Scancode FindScancode(const CStr8& keyname);
// Map a scancode to a locale-independent scancode name.
extern CStr8 FindScancodeName(SDL_Scancode scancode);
// Map a scancode to a locale-dependent key name (to show the user).
extern CStr8 FindKeyName(SDL_Scancode scancode);

enum {
	// Start sequential IDs in the right place
	// Pick a code which is greater than any scancodes used by SDL itself
	EXTRA_KEYS_BASE = SDL_NUM_SCANCODES,
	// 'Keycodes' for the unified modifier keys
	UNIFIED_SHIFT,
	UNIFIED_CTRL,
	UNIFIED_ALT,
	UNIFIED_SUPER,
	UNIFIED_LAST,
	// 'Keycodes' for the mouse buttons
	// Base for mouse buttons.
	// Everything less than MOUSE_BASE is not reported by an SDL mouse button event.
	// Everything greater than MOUSE_BASE is reported by an SDL mouse button event.
	MOUSE_BASE,
	MOUSE_LEFT = MOUSE_BASE + SDL_BUTTON_LEFT,
	MOUSE_MIDDLE = MOUSE_BASE + SDL_BUTTON_MIDDLE,
	MOUSE_RIGHT = MOUSE_BASE + SDL_BUTTON_RIGHT,
	// SDL2 doesn't count wheels as buttons, so just give them the previous sequential IDs
	MOUSE_WHEELUP = MOUSE_BASE + 4,
	MOUSE_WHEELDOWN = MOUSE_BASE + 5,
	MOUSE_X1 = MOUSE_BASE + SDL_BUTTON_X1 + 2,
	MOUSE_X2 = MOUSE_BASE + SDL_BUTTON_X2 + 2,
	MOUSE_LAST,
};

#endif	// #ifndef INCLUDED_KEYNAME
