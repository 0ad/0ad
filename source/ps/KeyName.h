/* Copyright (C) 2013 Wildfire Games.
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

class CStr8;

extern void InitKeyNameMap();
extern CStr8 FindKeyName( int keycode );
extern int FindKeyCode( const CStr8& keyname );

// Pick a code which is greater than any keycodes used by SDL itself
#if SDL_VERSION_ATLEAST(2, 0, 0)
# define CUSTOM_SDL_KEYCODE SDL_SCANCODE_TO_KEYCODE(SDL_NUM_SCANCODES)
#else
# define CUSTOM_SDL_KEYCODE SDLK_LAST
#endif

enum {
	// Start sequential IDs in the right place
	EXTRA_KEYS_BASE = CUSTOM_SDL_KEYCODE,
	// 'Keycodes' for the unified modifier keys
	UNIFIED_SHIFT,
	UNIFIED_CTRL,
	UNIFIED_ALT,
	UNIFIED_SUPER,
	UNIFIED_LAST,
	// 'Keycodes' for the mouse buttons
	// SDL2 doesn't count wheels as buttons, so just give them the previous sequential IDs
#if SDL_VERSION_ATLEAST(2, 0, 0)
	MOUSE_WHEELUP,
	MOUSE_WHEELDOWN,
#endif
	// Base for mouse buttons.
	// Everything less than MOUSE_BASE is not reported by an SDL mouse button event.
	// Everything greater than MOUSE_BASE is reported by an SDL mouse button event.
	MOUSE_BASE,
	MOUSE_LEFT = MOUSE_BASE + SDL_BUTTON_LEFT,
	MOUSE_RIGHT = MOUSE_BASE + SDL_BUTTON_RIGHT,
	MOUSE_MIDDLE = MOUSE_BASE + SDL_BUTTON_MIDDLE,
#if SDL_VERSION_ATLEAST(2, 0, 0)
	MOUSE_X1 = MOUSE_BASE + SDL_BUTTON_X1,
	MOUSE_X2 = MOUSE_BASE + SDL_BUTTON_X2,
#elif SDL_VERSION_ATLEAST(1, 2, 13)
	// SDL 1.2 defines wheel buttons before X1/X2 buttons
	MOUSE_WHEELUP = MOUSE_BASE + SDL_BUTTON_WHEELUP,
	MOUSE_WHEELDOWN = MOUSE_BASE + SDL_BUTTON_WHEELDOWN,
	MOUSE_X1 = MOUSE_BASE + SDL_BUTTON_X1,
	MOUSE_X2 = MOUSE_BASE + SDL_BUTTON_X2,
#else
	// SDL <1.2.13 doesn't support X1/X2 buttons, so define them manually
	MOUSE_WHEELUP = MOUSE_BASE + SDL_BUTTON_WHEELUP,
	MOUSE_WHEELDOWN = MOUSE_BASE + SDL_BUTTON_WHEELDOWN,
	MOUSE_X1 = MOUSE_BASE + SDL_BUTTON_WHEELDOWN + 1,
	MOUSE_X2 = MOUSE_BASE + SDL_BUTTON_WHEELDOWN + 2,
#endif

}; 

#endif	// #ifndef INCLUDED_KEYNAME
