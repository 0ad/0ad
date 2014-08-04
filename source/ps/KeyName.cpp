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

// Ooh, a file of keynames. Fun.

#include "precompiled.h"

#include "KeyName.h"
#include "CStr.h"
#include "lib/external_libraries/libsdl.h"

#include <map>

static std::map<CStr,int> keymap;

struct SKeycodeMapping
{
	int keycode;
	const char* keyname;
	const char* altkeyname;
};

// You can use either key name in the config file...

static SKeycodeMapping keycodeMapping[] =
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

#if SDL_VERSION_ATLEAST(2, 0, 0)
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
#else
	{ SDLK_WORLD_0, "World0", "W0" },
	{ SDLK_WORLD_1, "World1", "W1" },
	{ SDLK_WORLD_2, "World2", "W2" },
	{ SDLK_WORLD_3, "World3", "W3" },
	{ SDLK_WORLD_4, "World4", "W4" },
	{ SDLK_WORLD_5, "World5", "W5" },
	{ SDLK_WORLD_6, "World6", "W6" },
	{ SDLK_WORLD_7, "World7", "W7" },
	{ SDLK_WORLD_8, "World8", "W8" },
	{ SDLK_WORLD_9, "World9", "W9" },
	{ SDLK_WORLD_10, "World10", "W10" },
	{ SDLK_WORLD_11, "World11", "W11" },
	{ SDLK_WORLD_12, "World12", "W12" },
	{ SDLK_WORLD_13, "World13", "W13" },
	{ SDLK_WORLD_14, "World14", "W14" },
	{ SDLK_WORLD_15, "World15", "W15" },
	{ SDLK_WORLD_16, "World16", "W16" },
	{ SDLK_WORLD_17, "World17", "W17" },
	{ SDLK_WORLD_18, "World18", "W18" },
	{ SDLK_WORLD_19, "World19", "W19" },
	{ SDLK_WORLD_20, "World20", "W20" },
	{ SDLK_WORLD_21, "World21", "W21" },
	{ SDLK_WORLD_22, "World22", "W22" },
	{ SDLK_WORLD_23, "World23", "W23" },
	{ SDLK_WORLD_24, "World24", "W24" },
	{ SDLK_WORLD_25, "World25", "W25" },
	{ SDLK_WORLD_26, "World26", "W26" },
	{ SDLK_WORLD_27, "World27", "W27" },
	{ SDLK_WORLD_28, "World28", "W28" },
	{ SDLK_WORLD_29, "World29", "W29" },
	{ SDLK_WORLD_30, "World30", "W30" },
	{ SDLK_WORLD_31, "World31", "W31" },
	{ SDLK_WORLD_32, "World32", "W32" },
	{ SDLK_WORLD_33, "World33", "W33" },
	{ SDLK_WORLD_34, "World34", "W34" },
	{ SDLK_WORLD_35, "World35", "W35" },
	{ SDLK_WORLD_36, "World36", "W36" },
	{ SDLK_WORLD_37, "World37", "W37" },
	{ SDLK_WORLD_38, "World38", "W38" },
	{ SDLK_WORLD_39, "World39", "W39" },
	{ SDLK_WORLD_40, "World40", "W40" },
	{ SDLK_WORLD_41, "World41", "W41" },
	{ SDLK_WORLD_42, "World42", "W42" },
	{ SDLK_WORLD_43, "World43", "W43" },
	{ SDLK_WORLD_44, "World44", "W44" },
	{ SDLK_WORLD_45, "World45", "W45" },
	{ SDLK_WORLD_46, "World46", "W46" },
	{ SDLK_WORLD_47, "World47", "W47" },
	{ SDLK_WORLD_48, "World48", "W48" },
	{ SDLK_WORLD_49, "World49", "W49" },
	{ SDLK_WORLD_50, "World50", "W50" },
	{ SDLK_WORLD_51, "World51", "W51" },
	{ SDLK_WORLD_52, "World52", "W52" },
	{ SDLK_WORLD_53, "World53", "W53" },
	{ SDLK_WORLD_54, "World54", "W54" },
	{ SDLK_WORLD_55, "World55", "W55" },
	{ SDLK_WORLD_56, "World56", "W56" },
	{ SDLK_WORLD_57, "World57", "W57" },
	{ SDLK_WORLD_58, "World58", "W58" },
	{ SDLK_WORLD_59, "World59", "W59" },
	{ SDLK_WORLD_60, "World60", "W60" },
	{ SDLK_WORLD_61, "World61", "W61" },
	{ SDLK_WORLD_62, "World62", "W62" },
	{ SDLK_WORLD_63, "World63", "W63" },
	{ SDLK_WORLD_64, "World64", "W64" },
	{ SDLK_WORLD_65, "World65", "W65" },
	{ SDLK_WORLD_66, "World66", "W66" },
	{ SDLK_WORLD_67, "World67", "W67" },
	{ SDLK_WORLD_68, "World68", "W68" },
	{ SDLK_WORLD_69, "World69", "W69" },
	{ SDLK_WORLD_70, "World70", "W70" },
	{ SDLK_WORLD_71, "World71", "W71" },
	{ SDLK_WORLD_72, "World72", "W72" },
	{ SDLK_WORLD_73, "World73", "W73" },
	{ SDLK_WORLD_74, "World74", "W74" },
	{ SDLK_WORLD_75, "World75", "W75" },
	{ SDLK_WORLD_76, "World76", "W76" },
	{ SDLK_WORLD_77, "World77", "W77" },
	{ SDLK_WORLD_78, "World78", "W78" },
	{ SDLK_WORLD_79, "World79", "W79" },
	{ SDLK_WORLD_80, "World80", "W80" },
	{ SDLK_WORLD_81, "World81", "W81" },
	{ SDLK_WORLD_82, "World82", "W82" },
	{ SDLK_WORLD_83, "World83", "W83" },
	{ SDLK_WORLD_84, "World84", "W84" },
	{ SDLK_WORLD_85, "World85", "W85" },
	{ SDLK_WORLD_86, "World86", "W86" },
	{ SDLK_WORLD_87, "World87", "W87" },
	{ SDLK_WORLD_88, "World88", "W88" },
	{ SDLK_WORLD_89, "World89", "W89" },
	{ SDLK_WORLD_90, "World90", "W90" },
	{ SDLK_WORLD_91, "World91", "W91" },
	{ SDLK_WORLD_92, "World92", "W92" },
	{ SDLK_WORLD_93, "World93", "W93" },
	{ SDLK_WORLD_94, "World94", "W94" },
	{ SDLK_WORLD_95, "World95", "W95" },

	{ SDLK_KP0, "Numpad 0", "Num0" },
	{ SDLK_KP1, "Numpad 1", "Num1" },
	{ SDLK_KP2, "Numpad 2", "Num2" },
	{ SDLK_KP3, "Numpad 3", "Num3" },
	{ SDLK_KP4, "Numpad 4", "Num4" },
	{ SDLK_KP5, "Numpad 5", "Num5" },
	{ SDLK_KP6, "Numpad 6", "Num6" },
	{ SDLK_KP7, "Numpad 7", "Num7" },
	{ SDLK_KP8, "Numpad 8", "Num8" },
	{ SDLK_KP9, "Numpad 9", "Num9" },
#endif

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

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{ SDLK_NUMLOCKCLEAR, "Num Lock", "NumLock" },
#else
	{ SDLK_NUMLOCK, "Num Lock", "NumLock" },
#endif

	{ SDLK_CAPSLOCK, "Caps Lock", "CapsLock" },

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{ SDLK_SCROLLLOCK, "Scroll Lock", "ScrlLock" },
#else
	{ SDLK_SCROLLOCK, "Scroll Lock", "ScrlLock" },
#endif

	{ SDLK_RSHIFT, "Right Shift", "RightShift" },
	{ SDLK_LSHIFT, "Left Shift", "LeftShift" },
	{ SDLK_RCTRL, "Right Ctrl", "RightCtrl" },
	{ SDLK_LCTRL, "Left Ctrl", "LeftCtrl" },
	{ SDLK_RALT, "Right Alt", "RightAlt" },
	{ SDLK_LALT, "Left Alt", "LeftAlt" },

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{ SDLK_LGUI, "Left Super", "LeftWin" },        /* "Windows" keys */
	{ SDLK_RGUI, "Right Super", "RightWin" },        
#else
	{ SDLK_LSUPER, "Left Super", "LeftWin" },        /* "Windows" keys */
	{ SDLK_RSUPER, "Right Super", "RightWin" },        
#endif

	{ SDLK_MODE, "Alt Gr", "AltGr" },

	{ SDLK_HELP, "Help", 0 }, // ?

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{ SDLK_PRINTSCREEN, "Print Screen", "PrtSc" },
#else
	{ SDLK_PRINT, "Print Screen", "PrtSc" },
#endif

	{ SDLK_SYSREQ, "SysRq", 0 },

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{ SDLK_STOP, "Break", 0 },
#else
	{ SDLK_BREAK, "Break", 0 },
#endif

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
	for (SKeycodeMapping* it = keycodeMapping; it->keycode != 0; it++)
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
		return(it->second);
	return 0;
}

CStr FindKeyName(int keycode)
{
	SKeycodeMapping* it = keycodeMapping;
	while (it->keycode != 0)
		if (it->keycode == keycode)
			return CStr(it->keyname);

	return CStr("Unknown");
}


