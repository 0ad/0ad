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

#ifndef INCLUDED_KEYNAME
#define INCLUDED_KEYNAME

// Need SDLK_* enum values.
#include "lib/external_libraries/sdl.h"

extern void InitKeyNameMap();
extern CStr FindKeyName( int keycode );
extern int FindKeyCode( const CStr& keyname );

enum {
	// 'Keycodes' for the mouse buttons
	MOUSE_LEFT = SDLK_LAST + SDL_BUTTON_LEFT,
	MOUSE_RIGHT = SDLK_LAST + SDL_BUTTON_RIGHT,
	MOUSE_MIDDLE = SDLK_LAST + SDL_BUTTON_MIDDLE,
	MOUSE_WHEELUP = SDLK_LAST + SDL_BUTTON_WHEELUP,
	MOUSE_WHEELDOWN = SDLK_LAST + SDL_BUTTON_WHEELDOWN,

	// 'Keycodes' for the unified modifier keys
	UNIFIED_SHIFT,
	UNIFIED_CTRL,
	UNIFIED_ALT,
	UNIFIED_META,
	UNIFIED_SUPER
}; 

#endif	// #ifndef INCLUDED_KEYNAME
