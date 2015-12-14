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

#include "precompiled.h"

#include "KeyMap.h"

#include "SDL_version.h"
#include "SDL_keycode.h"

int GetSDLKeyFromWxKeyCode(int wxkey)
{
	// wx gives uppercase letters, SDL expects lowercase
	if (wxkey >= 'A' && wxkey <= 'Z')
		return wxkey + 'a' - 'A';

	if (wxkey < 256)
		return wxkey;

	switch (wxkey)
	{
	case WXK_NUMPAD0: return SDLK_KP_0;
	case WXK_NUMPAD1: return SDLK_KP_1;
	case WXK_NUMPAD2: return SDLK_KP_2;
	case WXK_NUMPAD3: return SDLK_KP_3;
	case WXK_NUMPAD4: return SDLK_KP_4;
	case WXK_NUMPAD5: return SDLK_KP_5;
	case WXK_NUMPAD6: return SDLK_KP_6;
	case WXK_NUMPAD7: return SDLK_KP_7;
	case WXK_NUMPAD8: return SDLK_KP_8;
	case WXK_NUMPAD9: return SDLK_KP_9;
	case WXK_NUMPAD_DECIMAL: return SDLK_KP_PERIOD;
	case WXK_NUMPAD_DIVIDE: return SDLK_KP_DIVIDE;
	case WXK_NUMPAD_MULTIPLY: return SDLK_KP_MULTIPLY;
	case WXK_NUMPAD_SUBTRACT: return SDLK_KP_MINUS;
	case WXK_NUMPAD_ADD: return SDLK_KP_PLUS;
	case WXK_NUMPAD_ENTER: return SDLK_KP_ENTER;
	case WXK_NUMPAD_EQUAL: return SDLK_KP_EQUALS;

	case WXK_UP: return SDLK_UP;
	case WXK_DOWN: return SDLK_DOWN;
	case WXK_RIGHT: return SDLK_RIGHT;
	case WXK_LEFT: return SDLK_LEFT;
	case WXK_INSERT: return SDLK_INSERT;
	case WXK_HOME: return SDLK_HOME;
	case WXK_END: return SDLK_END;
	case WXK_PAGEUP: return SDLK_PAGEUP;
	case WXK_PAGEDOWN: return SDLK_PAGEDOWN;

	case WXK_F1: return SDLK_F1;
	case WXK_F2: return SDLK_F2;
	case WXK_F3: return SDLK_F3;
	case WXK_F4: return SDLK_F4;
	case WXK_F5: return SDLK_F5;
	case WXK_F6: return SDLK_F6;
	case WXK_F7: return SDLK_F7;
	case WXK_F8: return SDLK_F8;
	case WXK_F9: return SDLK_F9;
	case WXK_F10: return SDLK_F10;
	case WXK_F11: return SDLK_F11;
	case WXK_F12: return SDLK_F12;
	case WXK_F13: return SDLK_F13;
	case WXK_F14: return SDLK_F14;
	case WXK_F15: return SDLK_F15;

	case WXK_NUMLOCK: return SDLK_NUMLOCKCLEAR;
	case WXK_SCROLL: return SDLK_SCROLLLOCK;
//	case WXK_: return SDLK_CAPSLOCK;
	case WXK_SHIFT: return SDLK_RSHIFT;
//	case WXK_: return SDLK_LSHIFT;
	case WXK_CONTROL: return SDLK_RCTRL;
//	case WXK_: return SDLK_LCTRL;
	case WXK_ALT: return SDLK_RALT;
//	case WXK_: return SDLK_LALT;
//	case WXK_: return SDLK_RMETA;
//	case WXK_: return SDLK_LMETA;
//	case WXK_: return SDLK_LSUPER;
//	case WXK_: return SDLK_RSUPER;
//	case WXK_: return SDLK_MODE;
//	case WXK_: return SDLK_COMPOSE;

	case WXK_HELP: return SDLK_HELP;
	case WXK_PRINT: return SDLK_PRINTSCREEN;
//	case WXK_: return SDLK_SYSREQ;
//	case WXK_: return SDLK_BREAK;
	case WXK_MENU: return SDLK_MENU;
//	case WXK_: return SDLK_POWER;
//	case WXK_: return SDLK_EURO;
//	case WXK_: return SDLK_UNDO;
	}

	return 0;
}
