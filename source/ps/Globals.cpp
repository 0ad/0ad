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

#include "precompiled.h"
#include "Globals.h"

#include "lib/external_libraries/sdl.h"


bool g_app_minimized = false;
bool g_app_has_focus = true;

std::map<int32_t, bool> g_keys;
int g_mouse_x = 50, g_mouse_y = 50;
bool g_mouse_active = true;

// unused, left, right, middle, wheel up, wheel down
// (order is given by SDL_BUTTON_* constants).
bool g_mouse_buttons[6] = {0};

PIFrequencyFilter g_frequencyFilter;

// updates the state of the above; never swallows messages.
InReaction GlobalsInputHandler(const SDL_Event_* ev)
{
	size_t c;

	switch(ev->ev.type)
	{
	case SDL_ACTIVEEVENT:
		if(ev->ev.active.state & SDL_APPACTIVE)
			g_app_minimized = (ev->ev.active.gain == 0);	// negated
		if(ev->ev.active.state & SDL_APPINPUTFOCUS)
			g_app_has_focus = (ev->ev.active.gain != 0);
		if(ev->ev.active.state & SDL_APPMOUSEFOCUS)
			g_mouse_active = (ev->ev.active.gain != 0);
		return IN_PASS;

	case SDL_MOUSEMOTION:
		g_mouse_x = ev->ev.motion.x;
		g_mouse_y = ev->ev.motion.y;
		return IN_PASS;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		c = ev->ev.button.button;
		if(c < ARRAY_SIZE(g_mouse_buttons))
			g_mouse_buttons[c] = (ev->ev.type == SDL_MOUSEBUTTONDOWN);
		else
		{
			// don't complain: just ignore people with too many mouse buttons
			//debug_warn(L"invalid mouse button");
		}
		return IN_PASS;

	case SDL_KEYDOWN:
	case SDL_KEYUP:
		g_keys[ev->ev.key.keysym.sym] = (ev->ev.type == SDL_KEYDOWN);
		return IN_PASS;

	default:
		return IN_PASS;
	}

	UNREACHABLE;
}
