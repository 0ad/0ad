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

// Ooh, a file of keynames. Fun.

#include "precompiled.h"

#include "KeyName.h"

#include "lib/external_libraries/libsdl.h"
#include "ps/CStr.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

// Some scancodes <-> names that SDL doesn't recognise.
// Those are tested first so they override SDL defaults (useful for UNIFIED keys).
static const std::unordered_map<int, std::vector<CStr>> scancodemap {{
	{ SDL_SCANCODE_DOWN, { "DownArrow" } },
	{ SDL_SCANCODE_UP, { "UpArrow" } },
	{ SDL_SCANCODE_LEFT, { "LeftArrow" } },
	{ SDL_SCANCODE_RIGHT, { "RightArrow" } },

	{ SDL_SCANCODE_EQUALS, { "Plus" } },
	{ SDL_SCANCODE_MINUS, { "Minus" } },

	{ SDL_SCANCODE_KP_ENTER, { "NumEnter" } },
	{ SDL_SCANCODE_KP_DIVIDE, { "NumDivide" } },
	{ SDL_SCANCODE_KP_MULTIPLY, { "NumMultiply" } },
	{ SDL_SCANCODE_KP_EQUALS, { "NumEquals" } },
	{ SDL_SCANCODE_KP_PERIOD, { "NumDecimal" } },
	{ SDL_SCANCODE_KP_PLUS, { "NumPlus" } },
	{ SDL_SCANCODE_KP_MINUS, { "NumMinus" } },
	{ SDL_SCANCODE_KP_0, { "Num0" } },
	{ SDL_SCANCODE_KP_1, { "Num1" } },
	{ SDL_SCANCODE_KP_2, { "Num2" } },
	{ SDL_SCANCODE_KP_3, { "Num3" } },
	{ SDL_SCANCODE_KP_4, { "Num4" } },
	{ SDL_SCANCODE_KP_5, { "Num5" } },
	{ SDL_SCANCODE_KP_6, { "Num6" } },
	{ SDL_SCANCODE_KP_7, { "Num7" } },
	{ SDL_SCANCODE_KP_8, { "Num8" } },
	{ SDL_SCANCODE_KP_9, { "Num9" } },

	{ SDL_SCANCODE_COMMA, { "Comma" } },
	{ SDL_SCANCODE_PERIOD, { "Period" } },
	{ SDL_SCANCODE_APOSTROPHE, { "Quote" } },
	{ SDL_SCANCODE_SEMICOLON, { "Semicolon" } },
	{ SDL_SCANCODE_GRAVE, { "Backquote" } },
	{ SDL_SCANCODE_LEFTBRACKET, { "LeftBracket" } },
	{ SDL_SCANCODE_RIGHTBRACKET, { "RightBracket" } },
	{ SDL_SCANCODE_BACKSLASH, { "Backslash" } },
	{ SDL_SCANCODE_SLASH, { "Slash" } },

	{ SDL_SCANCODE_RETURN, { "Enter" } },
	{ SDL_SCANCODE_ESCAPE, { "Esc" } },
	{ SDL_SCANCODE_PAUSE, { "Break" } },
	{ SDL_SCANCODE_DELETE, { "Del" } },

	{ MOUSE_LEFT, { "MouseLeft" } },
	{ MOUSE_RIGHT, { "MouseRight" } },
	{ MOUSE_MIDDLE, { "MouseMiddle" } },
	{ MOUSE_WHEELUP, { "WheelUp" } },
	{ MOUSE_WHEELDOWN, { "WheelDown" } },
	{ MOUSE_X1, { "WheelLeft", "MouseX1" } },
	{ MOUSE_X2, { "WheelRight", "MouseX2" } },

	{ UNIFIED_SHIFT, { "Shift", "Left Shift", "Right Shift" } },
	{ UNIFIED_CTRL, { "Ctrl", "Left Ctrl", "Right Ctrl" } },
	{ UNIFIED_ALT, { "Alt", "Left Alt", "Right Alt" } },
	{ UNIFIED_SUPER, { "Super", "Left Gui", "Right Gui" } },
}};

SDL_Scancode FindScancode(const CStr8& keyname)
{
	// Find (ignoring case) a corresponding scancode, if one exists.
	std::unordered_map<int, std::vector<CStr>>::const_iterator it =
		std::find_if(scancodemap.begin(), scancodemap.end(), [&keyname](const std::pair<int, std::vector<CStr>>& names) {
			return std::find_if(names.second.begin(), names.second.end(), [&keyname](const CStr& t) {
				return t.LowerCase() == keyname.LowerCase();
			})!= names.second.end();
		});

	if (it != scancodemap.end())
		return static_cast<SDL_Scancode>(it->first);

	SDL_Scancode code = SDL_GetScancodeFromName(keyname.c_str());
	if (code != SDL_SCANCODE_UNKNOWN)
		return code;

	// Parse SYM_XX codes, see below.
	if (keyname.size() > 4 && keyname.Left(4) == "SYM_")
		return static_cast<SDL_Scancode>(CStr(keyname.substr(4)).ToInt());

	return SDL_SCANCODE_UNKNOWN;
}

CStr FindScancodeName(SDL_Scancode scancode)
{
	if (scancodemap.find(scancode) != scancodemap.end())
		return scancodemap.at(scancode).front();

	const char* name = SDL_GetScancodeName(scancode);
	// Some scancodes have no name, but we must have something to save/load/recognize it, so parse it as SYM_XX
	if (strlen(name) == 0)
		return CStr("SYM_") + CStr::FromInt(scancode);
	return name;
}

// Rename some SDL key names (!scancodes) for easier readability.
// NB: this does not intend to be exhaustive, merely cover the usual suspects.
static const std::unordered_map<SDL_Keycode, CStr> keyNames {{
	{ SDLK_COMMA, "Comma" },
	{ SDLK_SEMICOLON, "Semicolon" },
	{ SDLK_COLON, "Colon" },
	{ SDLK_PERIOD, "Period" },
	{ SDLK_EQUALS, "Equals" },
	{ SDLK_PLUS, "Plus" },
	{ SDLK_MINUS, "Minus" },

	{ SDLK_QUOTE, "SingleQuote" },
	{ SDLK_QUOTEDBL, "DoubleQuote" },
	{ SDLK_BACKQUOTE, "BackQuote" },

	{ SDLK_LEFTPAREN, { "LeftParen" } },

	{ SDLK_LEFTBRACKET, { "LeftBracket" } },
	{ SDLK_RIGHTBRACKET, { "RightBracket" } },
	{ SDLK_BACKSLASH, { "Backslash" } },
	{ SDLK_SLASH, { "Slash" } },

	{ SDLK_KP_ENTER, "NumEnter" },
	{ SDLK_KP_DIVIDE, "NumDivide" },
	{ SDLK_KP_MULTIPLY, "NumMultiply" },
	{ SDLK_KP_EQUALS, "NumEquals" },
	{ SDLK_KP_PERIOD, "NumDecimal" },
	{ SDLK_KP_PLUS, "NumPlus" },
	{ SDLK_KP_MINUS, "NumMinus" },
	{ SDLK_KP_0, "Num0" },
	{ SDLK_KP_1, "Num1" },
	{ SDLK_KP_2, "Num2" },
	{ SDLK_KP_3, "Num3" },
	{ SDLK_KP_4, "Num4" },
	{ SDLK_KP_5, "Num5" },
	{ SDLK_KP_6, "Num6" },
	{ SDLK_KP_7, "Num7" },
	{ SDLK_KP_8, "Num8" },
	{ SDLK_KP_9, "Num9" },

	{ SDLK_UP, "\xe2\x86\x91" },
	{ SDLK_DOWN, "\xe2\x86\x93" },
	{ SDLK_LEFT, "\xe2\x86\x90" },
	{ SDLK_RIGHT, "\xe2\x86\x92" },
}};

CStr FindKeyName(SDL_Scancode scancode)
{
	// Mouse and unified modifiers are harcoded.
	if (static_cast<int>(scancode) == UNIFIED_SHIFT)
		return "Shift";
	else if (static_cast<int>(scancode) == UNIFIED_ALT)
		return "Alt";
	else if (static_cast<int>(scancode) == UNIFIED_CTRL)
		return "Ctrl";
	else if (static_cast<int>(scancode) == UNIFIED_SUPER)
		return "Super";
	else if (static_cast<int>(scancode) == MOUSE_LEFT)
		return "MouseLeft";
	else if (static_cast<int>(scancode) == MOUSE_RIGHT)
		return "MouseRight";
	else if (static_cast<int>(scancode) == MOUSE_MIDDLE)
		return "MouseMiddle";
	else if (static_cast<int>(scancode) == MOUSE_WHEELUP)
		return "WheelUp";
	else if (static_cast<int>(scancode) == MOUSE_WHEELDOWN)
		return "WheelDown";
	else if (static_cast<int>(scancode) == MOUSE_X1)
		return "WheelLeft";
	else if (static_cast<int>(scancode) == MOUSE_X2)
		return "WheelRight";

	SDL_Keycode code = SDL_GetKeyFromScancode(scancode);

	if (keyNames.find(code) != keyNames.end())
		return keyNames.at(code);

	if (code != SDLK_UNKNOWN)
	{
		const char* keyName = SDL_GetKeyName(code);
		if (strlen(keyName) != 0)
			return keyName;
	}

	// Try the scancode name.
	const char* name = SDL_GetScancodeName(scancode);

	// xxtreme hack: some SDLKeycodes map to chars, and we need to escape [ and \ .
	if (keyNames.find(static_cast<SDL_Keycode>(*name)) != keyNames.end())
		return keyNames.at(static_cast<SDL_Keycode>(*name));

	if (strlen(name) != 0)
		return name;

	// Else, show something regardless, so the player knows it's at least recognized.
	return CStr("SYM_") + CStr::FromInt(scancode);
}


