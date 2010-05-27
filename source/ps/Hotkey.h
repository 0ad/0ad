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

#ifndef PS_HOTKEY_H
#define PS_HOTKEY_H

// Hotkey.h
// 
// Constant definitions and a couple of exports for the hotkey processor
//
// Hotkeys can be mapped onto SDL events (for use internal to the engine),
// or used to trigger activation of GUI buttons.
//
// Adding a hotkey (SDL event type):
//
// - Define your constant in the enum, just below;
// - Add an entry to hotkeyInfo[], in Hotkey.cpp
//     first column is this constant, second is the config string (minus 'hotkey.') it maps to
//     third and fourth are the default keys, used if the config file doesn't contain that string.
// - Create an input handler for SDL_HOTKEYDOWN, SDL_HOTKEYUP, or poll the hotkeys[] array.
//     For SDL_HOTKEYDOWN, SDL_HOTKEYUP, the constant is passed in as ev->ev.user.code.
// - Add some bindings to the config file.

#include "CStr.h"
#include "lib/input.h"
#include "lib/external_libraries/sdl.h"	// see note below

// note: we need the real SDL header - it defines SDL_USEREVENT, which is
// required for our HOTKEY event type definition. this is OK since
// hotkey.h is not included from any headers.

const int SDL_HOTKEYDOWN = SDL_USEREVENT;
const int SDL_HOTKEYUP = SDL_USEREVENT + 1;
const int SDL_GUIHOTKEYPRESS = SDL_USEREVENT + 2;

enum
{
	HOTKEY_EXIT,
	HOTKEY_SCREENSHOT,
	HOTKEY_BIGSCREENSHOT,
	HOTKEY_WIREFRAME,
	HOTKEY_CAMERA_RESET,
	HOTKEY_CAMERA_RESET_ORIGIN,
	HOTKEY_CAMERA_ZOOM_IN,
	HOTKEY_CAMERA_ZOOM_OUT,
	HOTKEY_CAMERA_ZOOM_WHEEL_IN,
	HOTKEY_CAMERA_ZOOM_WHEEL_OUT,
	HOTKEY_CAMERA_ROTATE,
	HOTKEY_CAMERA_ROTATE_KEYBOARD,
	HOTKEY_CAMERA_ROTATE_ABOUT_TARGET,
	HOTKEY_CAMERA_ROTATE_ABOUT_TARGET_KEYBOARD,
	HOTKEY_CAMERA_PAN,
	HOTKEY_CAMERA_PAN_KEYBOARD,
	HOTKEY_CAMERA_LEFT,
	HOTKEY_CAMERA_RIGHT,
	HOTKEY_CAMERA_UP,
	HOTKEY_CAMERA_DOWN,
	HOTKEY_CAMERA_UNIT_VIEW,
	HOTKEY_CAMERA_UNIT_ATTACH,
	HOTKEY_CAMERA_BOOKMARK_0,
	HOTKEY_CAMERA_BOOKMARK_1,
	HOTKEY_CAMERA_BOOKMARK_2,
	HOTKEY_CAMERA_BOOKMARK_3,
	HOTKEY_CAMERA_BOOKMARK_4,
	HOTKEY_CAMERA_BOOKMARK_5,
	HOTKEY_CAMERA_BOOKMARK_6,
	HOTKEY_CAMERA_BOOKMARK_7,
	HOTKEY_CAMERA_BOOKMARK_8,
	HOTKEY_CAMERA_BOOKMARK_9,
	HOTKEY_CAMERA_BOOKMARK_SAVE,
	HOTKEY_CAMERA_BOOKMARK_SNAP,
	HOTKEY_CAMERA_CINEMA_ADD,
	HOTKEY_CAMERA_CINEMA_DELETE,
	HOTKEY_CAMERA_CINEMA_DELETE_ALL,
	HOTKEY_CAMERA_CINEMA_QUEUE,
	HOTKEY_CONSOLE_TOGGLE,
	HOTKEY_CONSOLE_COPY,
	HOTKEY_CONSOLE_PASTE,
	HOTKEY_SELECTION_ADD,
	HOTKEY_SELECTION_REMOVE,
	HOTKEY_SELECTION_GROUP_0,
	HOTKEY_SELECTION_GROUP_1,
	HOTKEY_SELECTION_GROUP_2,
	HOTKEY_SELECTION_GROUP_3,
	HOTKEY_SELECTION_GROUP_4,
	HOTKEY_SELECTION_GROUP_5,
	HOTKEY_SELECTION_GROUP_6,
	HOTKEY_SELECTION_GROUP_7,
	HOTKEY_SELECTION_GROUP_8,
	HOTKEY_SELECTION_GROUP_9,
	HOTKEY_SELECTION_GROUP_10,
	HOTKEY_SELECTION_GROUP_11,
	HOTKEY_SELECTION_GROUP_12,
	HOTKEY_SELECTION_GROUP_13,
	HOTKEY_SELECTION_GROUP_14,
	HOTKEY_SELECTION_GROUP_15,
	HOTKEY_SELECTION_GROUP_16,
	HOTKEY_SELECTION_GROUP_17,
	HOTKEY_SELECTION_GROUP_18,
	HOTKEY_SELECTION_GROUP_19,
	HOTKEY_SELECTION_GROUP_ADD,
	HOTKEY_SELECTION_GROUP_SAVE,
	HOTKEY_SELECTION_GROUP_SNAP,
	HOTKEY_SELECTION_SNAP,
	HOTKEY_ORDER_QUEUE,
	HOTKEY_CONTEXTORDER_NEXT,
	HOTKEY_CONTEXTORDER_PREVIOUS,
	HOTKEY_HIGHLIGHTALL,
	HOTKEY_PROFILE_TOGGLE,
	HOTKEY_PROFILE_SAVE,
	HOTKEY_PLAYMUSIC,
	HOTKEY_PAUSE,
	HOTKEY_SPEED_INCREASE,
	HOTKEY_SPEED_DECREASE,
	
	HOTKEY_LAST,

	HOTKEY_NEGATION_FLAG = 65536
};

extern void LoadHotkeys();
extern InReaction HotkeyInputHandler(const SDL_Event_* ev);
extern void HotkeyRegisterGuiObject(const CStr& objName, const CStr& hotkeyName);

/**
 * @return the name of the specified HOTKEY_*, or empty string if not defined
 **/
extern CStr HotkeyGetName(int hotkey);

/**
 * @return whether the specified HOTKEY_* responds to the specified SDLK_*
 * (mainly for the screenshot system to know whether it needs to override
 * the printscreen screen). Ignores modifier keys.
 **/
extern bool HotkeyRespondsTo(int hotkey, int sdlkey);

/**
 * @return whether one of the key combinations for the given hotkey is pressed
 **/
extern bool HotkeyIsPressed(const CStr& keyname);

extern bool hotkeys[HOTKEY_LAST];

#endif // PS_HOTKEY_H
