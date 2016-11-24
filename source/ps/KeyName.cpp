/* Copyright (C) 2015 Wildfire Games.
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

#include <map>

static std::map<CStr,int> keymap;

struct SKeycodeMapping
{
	int keycode;
	const char* keyname;
	const char* altkeyname;
};

// You can use either key name in the config file...

static const SKeycodeMapping keycodeMapping[] =
{
	/* Just a tad friendlier than SDL_GetKeyName's name */
	{ SDLK_BACKSPACE, "Backspace", "BkSp" },
	{ SDLK_TAB, "Tab", 0 },
	{ SDLK_CLEAR, "Clear", 0 }, // ?
	{ SDLK_RETURN, "Return", "Ret" },
	{ SDLK_PAUSE, "Pause", 0 }, // ?
	{ SDLK_ESCAPE, "Escape", "Esc" },
	{ SDLK_SPACE, "Space", "Spc" },
	{ SDLK_EXCLAIM, "!", "Exclaim" },
	{ SDLK_QUOTEDBL, "\"", "DoubleQuote" },
	{ SDLK_HASH, "#", "Hash" },
	{ SDLK_DOLLAR, "$", "Dollar" },
	{ SDLK_AMPERSAND, "&", "Ampersand" },
	{ SDLK_QUOTE, "'", "SingleQuote" },
	{ SDLK_LEFTPAREN, "(", "LeftParen" },
	{ SDLK_RIGHTPAREN, ")", "RightParen" },
	{ SDLK_ASTERISK, "*", "Asterisk" },
	{ SDLK_PLUS, "+", "Plus" },
	{ SDLK_COMMA, ",", "Comma" },
	{ SDLK_MINUS, "-", "Minus" },
	{ SDLK_PERIOD, ".", "Period" },
	{ SDLK_SLASH, "/", "ForwardSlash" },
	{ SDLK_0, "0", 0 },
	{ SDLK_1, "1", 0 },
	{ SDLK_2, "2", 0 },
	{ SDLK_3, "3", 0 },
	{ SDLK_4, "4", 0 },
	{ SDLK_5, "5", 0 },
	{ SDLK_6, "6", 0 },
	{ SDLK_7, "7", 0 },
	{ SDLK_8, "8", 0 },
	{ SDLK_9, "9", 0 },
	{ SDLK_COLON, ":", "Colon" },
	{ SDLK_SEMICOLON, ";", "Semicolon" },
	{ SDLK_LESS, "<", "LessThan" },
	{ SDLK_EQUALS, "=", "Equals" },
	{ SDLK_GREATER, ">", "GreaterThan" },
	{ SDLK_QUESTION, "?", "Question" },
	{ SDLK_AT, "@", "At" },
	{ SDLK_LEFTBRACKET, "[", "LeftBracket" },
	{ SDLK_BACKSLASH, "\\", "BackSlash" },
	{ SDLK_RIGHTBRACKET, "]", "RightBracket" },
	{ SDLK_CARET, "^", "Caret", },
	{ SDLK_UNDERSCORE, "_", "Underscore" },
	{ SDLK_BACKQUOTE, "`", "BackQuote" },
	{ SDLK_a, "A", 0 },
	{ SDLK_b, "B", 0 },
	{ SDLK_c, "C", 0 },
	{ SDLK_d, "D", 0 },
	{ SDLK_e, "E", 0 },
	{ SDLK_f, "F", 0 },
	{ SDLK_g, "G", 0 },
	{ SDLK_h, "H", 0 },
	{ SDLK_i, "I", 0 },
	{ SDLK_j, "J", 0 },
	{ SDLK_k, "K", 0 },
	{ SDLK_l, "L", 0 },
	{ SDLK_m, "M", 0 },
	{ SDLK_n, "N", 0 },
	{ SDLK_o, "O", 0 },
	{ SDLK_p, "P", 0 },
	{ SDLK_q, "Q", 0 },
	{ SDLK_r, "R", 0 },
	{ SDLK_s, "S", 0 },
	{ SDLK_t, "T", 0 },
	{ SDLK_u, "U", 0 },
	{ SDLK_v, "V", 0 },
	{ SDLK_w, "W", 0 },
	{ SDLK_x, "X", 0 },
	{ SDLK_y, "Y", 0 },
	{ SDLK_z, "Z", 0 },
	{ SDLK_DELETE, "Delete", "Del" },

	{ SDLK_KP_0, "Numpad 0", "Num0" },
	{ SDLK_KP_1, "Numpad 1", "Num1" },
	{ SDLK_KP_2, "Numpad 2", "Num2" },
	{ SDLK_KP_3, "Numpad 3", "Num3" },
	{ SDLK_KP_4, "Numpad 4", "Num4" },
	{ SDLK_KP_5, "Numpad 5", "Num5" },
	{ SDLK_KP_6, "Numpad 6", "Num6" },
	{ SDLK_KP_7, "Numpad 7", "Num7" },
	{ SDLK_KP_8, "Numpad 8", "Num8" },
	{ SDLK_KP_9, "Numpad 9", "Num9" },

	{ SDLK_KP_PERIOD, "Numpad .", "NumPoint" },
	{ SDLK_KP_DIVIDE, "Numpad /", "NumDivide" },
	{ SDLK_KP_MULTIPLY, "Numpad *", "NumMultiply" },
	{ SDLK_KP_MINUS, "Numpad -", "NumMinus" },
	{ SDLK_KP_PLUS, "Numpad +", "NumPlus" },
	{ SDLK_KP_ENTER, "Numpad Enter", "NumEnter" },
	{ SDLK_KP_EQUALS, "Numpad =", "NumEquals" }, //?

	{ SDLK_UP, "Arrow Up", "UpArrow" },
	{ SDLK_DOWN, "Arrow Down", "DownArrow" },
	{ SDLK_RIGHT, "Arrow Right", "RightArrow" },
	{ SDLK_LEFT, "Arrow Left", "LeftArrow" },
	{ SDLK_INSERT, "Insert", "Ins" },
	{ SDLK_HOME, "Home", 0 },
	{ SDLK_END, "End", 0 },
	{ SDLK_PAGEUP, "Page Up", "PgUp" },
	{ SDLK_PAGEDOWN, "Page Down", "PgDn" },

	{ SDLK_F1, "F1", 0 },
	{ SDLK_F2, "F2", 0 },
	{ SDLK_F3, "F3", 0 },
	{ SDLK_F4, "F4", 0 },
	{ SDLK_F5, "F5", 0 },
	{ SDLK_F6, "F6", 0 },
	{ SDLK_F7, "F7", 0 },
	{ SDLK_F8, "F8", 0 },
	{ SDLK_F9, "F9", 0 },
	{ SDLK_F10, "F10", 0 },
	{ SDLK_F11, "F11", 0 },
	{ SDLK_F12, "F12", 0 },
	{ SDLK_F13, "F13", 0 },
	{ SDLK_F14, "F14", 0 },
	{ SDLK_F15, "F15", 0 },

	{ SDLK_NUMLOCKCLEAR, "Num Lock", "NumLock" },

	{ SDLK_CAPSLOCK, "Caps Lock", "CapsLock" },

	{ SDLK_SCROLLLOCK, "Scroll Lock", "ScrlLock" },

	{ SDLK_RSHIFT, "Right Shift", "RightShift" },
	{ SDLK_LSHIFT, "Left Shift", "LeftShift" },
	{ SDLK_RCTRL, "Right Ctrl", "RightCtrl" },
	{ SDLK_LCTRL, "Left Ctrl", "LeftCtrl" },
	{ SDLK_RALT, "Right Alt", "RightAlt" },
	{ SDLK_LALT, "Left Alt", "LeftAlt" },

	{ SDLK_LGUI, "Left Super", "LeftWin" },        /* "Windows" keys */
	{ SDLK_RGUI, "Right Super", "RightWin" },

	{ SDLK_MODE, "Alt Gr", "AltGr" },

	{ SDLK_HELP, "Help", 0 }, // ?

	{ SDLK_PRINTSCREEN, "Print Screen", "PrtSc" },

	{ SDLK_SYSREQ, "SysRq", 0 },

	{ SDLK_STOP, "Break", 0 },

	{ SDLK_MENU, "Menu", 0 }, // ?
	{ SDLK_POWER, "Power", 0 }, // ?
	{ SDLK_UNDO, "Undo", 0 }, // ?
	{ MOUSE_LEFT, "Left Mouse Button", "MouseLeft" },
	{ MOUSE_RIGHT, "Right Mouse Button", "MouseRight" },
	{ MOUSE_MIDDLE, "Middle Mouse Button", "MouseMiddle" },
	{ MOUSE_WHEELUP, "Mouse Wheel Up", "WheelUp" },
	{ MOUSE_WHEELDOWN, "Mouse Wheel Down", "WheelDown" },
	{ MOUSE_X1, "Mouse X1", "MouseX1" },
	{ MOUSE_X2, "Mouse X2", "MouseX2" },
	{ UNIFIED_SHIFT, "Shift", "AnyShift" },
	{ UNIFIED_CTRL, "Ctrl", "AnyCtrl" },
	{ UNIFIED_ALT, "Alt", "AnyAlt" },
	{ UNIFIED_SUPER, "Super", "AnyWindows" },
	{ 0, 0, 0 },
};

void InitKeyNameMap()
{
	for (const SKeycodeMapping* it = keycodeMapping; it->keycode != 0; ++it)
	{
		keymap.insert(std::pair<CStr,int>(CStr(it->keyname).LowerCase(), it->keycode));
		if(it->altkeyname)
			keymap.insert(std::pair<CStr,int>(CStr(it->altkeyname).LowerCase(), it->keycode));
	}

	// Extra mouse buttons.
	for (int i = 1; i < 256; ++i) // There is no mouse 0
	{
		keymap.insert(std::pair<CStr,int>("mousebutton" + CStr::FromInt(i), MOUSE_BASE + i));
		keymap.insert(std::pair<CStr,int>("mousen" + CStr::FromInt(i), MOUSE_BASE + i));
	}
}

int FindKeyCode(const CStr& keyname)
{
	std::map<CStr,int>::iterator it;
	it = keymap.find(keyname.LowerCase());
	if (it != keymap.end())
		return it->second;
	return 0;
}

CStr FindKeyName(int keycode)
{
	for (const SKeycodeMapping* it = keycodeMapping; it->keycode != 0; ++it)
		if (it->keycode == keycode)
			return CStr(it->keyname);

	return CStr("Unknown");
}


