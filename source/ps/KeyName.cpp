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

// Ooh, a file of keynames. Fun.

#include "precompiled.h"

#include "KeyName.h"
#include "CStr.h"
#include "lib/external_libraries/sdl.h"

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
/*
	{ SDLK_WORLD_0, "world 0" },
	{ SDLK_WORLD_1, "world 1" },
	{ SDLK_WORLD_2, "world 2" },
	{ SDLK_WORLD_3, "world 3" },
	{ SDLK_WORLD_4, "world 4" },
	{ SDLK_WORLD_5, "world 5" },
	{ SDLK_WORLD_6, "world 6" },
	{ SDLK_WORLD_7, "world 7" },
	{ SDLK_WORLD_8, "world 8" },
	{ SDLK_WORLD_9, "world 9" },
	{ SDLK_WORLD_10, "world 10" },
	{ SDLK_WORLD_11, "world 11" },
	{ SDLK_WORLD_12, "world 12" },
	{ SDLK_WORLD_13, "world 13" },
	{ SDLK_WORLD_14, "world 14" },
	{ SDLK_WORLD_15, "world 15" },
	{ SDLK_WORLD_16, "world 16" },
	{ SDLK_WORLD_17, "world 17" },
	{ SDLK_WORLD_18, "world 18" },
	{ SDLK_WORLD_19, "world 19" },
	{ SDLK_WORLD_20, "world 20" },
	{ SDLK_WORLD_21, "world 21" },
	{ SDLK_WORLD_22, "world 22" },
	{ SDLK_WORLD_23, "world 23" },
	{ SDLK_WORLD_24, "world 24" },
	{ SDLK_WORLD_25, "world 25" },
	{ SDLK_WORLD_26, "world 26" },
	{ SDLK_WORLD_27, "world 27" },
	{ SDLK_WORLD_28, "world 28" },
	{ SDLK_WORLD_29, "world 29" },
	{ SDLK_WORLD_30, "world 30" },
	{ SDLK_WORLD_31, "world 31" },
	{ SDLK_WORLD_32, "world 32" },
	{ SDLK_WORLD_33, "world 33" },
	{ SDLK_WORLD_34, "world 34" },
	{ SDLK_WORLD_35, "world 35" },
	{ SDLK_WORLD_36, "world 36" },
	{ SDLK_WORLD_37, "world 37" },
	{ SDLK_WORLD_38, "world 38" },
	{ SDLK_WORLD_39, "world 39" },
	{ SDLK_WORLD_40, "world 40" },
	{ SDLK_WORLD_41, "world 41" },
	{ SDLK_WORLD_42, "world 42" },
	{ SDLK_WORLD_43, "world 43" },
	{ SDLK_WORLD_44, "world 44" },
	{ SDLK_WORLD_45, "world 45" },
	{ SDLK_WORLD_46, "world 46" },
	{ SDLK_WORLD_47, "world 47" },
	{ SDLK_WORLD_48, "world 48" },
	{ SDLK_WORLD_49, "world 49" },
	{ SDLK_WORLD_50, "world 50" },
	{ SDLK_WORLD_51, "world 51" },
	{ SDLK_WORLD_52, "world 52" },
	{ SDLK_WORLD_53, "world 53" },
	{ SDLK_WORLD_54, "world 54" },
	{ SDLK_WORLD_55, "world 55" },
	{ SDLK_WORLD_56, "world 56" },
	{ SDLK_WORLD_57, "world 57" },
	{ SDLK_WORLD_58, "world 58" },
	{ SDLK_WORLD_59, "world 59" },
	{ SDLK_WORLD_60, "world 60" },
	{ SDLK_WORLD_61, "world 61" },
	{ SDLK_WORLD_62, "world 62" },
	{ SDLK_WORLD_63, "world 63" },
	{ SDLK_WORLD_64, "world 64" },
	{ SDLK_WORLD_65, "world 65" },
	{ SDLK_WORLD_66, "world 66" },
	{ SDLK_WORLD_67, "world 67" },
	{ SDLK_WORLD_68, "world 68" },
	{ SDLK_WORLD_69, "world 69" },
	{ SDLK_WORLD_70, "world 70" },
	{ SDLK_WORLD_71, "world 71" },
	{ SDLK_WORLD_72, "world 72" },
	{ SDLK_WORLD_73, "world 73" },
	{ SDLK_WORLD_74, "world 74" },
	{ SDLK_WORLD_75, "world 75" },
	{ SDLK_WORLD_76, "world 76" },
	{ SDLK_WORLD_77, "world 77" },
	{ SDLK_WORLD_78, "world 78" },
	{ SDLK_WORLD_79, "world 79" },
	{ SDLK_WORLD_80, "world 80" },
	{ SDLK_WORLD_81, "world 81" },
	{ SDLK_WORLD_82, "world 82" },
	{ SDLK_WORLD_83, "world 83" },
	{ SDLK_WORLD_84, "world 84" },
	{ SDLK_WORLD_85, "world 85" },
	{ SDLK_WORLD_86, "world 86" },
	{ SDLK_WORLD_87, "world 87" },
	{ SDLK_WORLD_88, "world 88" },
	{ SDLK_WORLD_89, "world 89" },
	{ SDLK_WORLD_90, "world 90" },
	{ SDLK_WORLD_91, "world 91" },
	{ SDLK_WORLD_92, "world 92" },
	{ SDLK_WORLD_93, "world 93" },
	{ SDLK_WORLD_94, "world 94" },
	{ SDLK_WORLD_95, "world 95" },
*/
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

	{ SDLK_NUMLOCK, "Num Lock", "NumLock" },
	{ SDLK_CAPSLOCK, "Caps Lock", "CapsLock" },
	{ SDLK_SCROLLOCK, "Scroll Lock", "ScrlLock" },
	{ SDLK_RSHIFT, "Right Shift", "RightShift" },
	{ SDLK_LSHIFT, "Left Shift", "LeftShift" },
	{ SDLK_RCTRL, "Right Ctrl", "RightCtrl" },
	{ SDLK_LCTRL, "Left Ctrl", "LeftCtrl" },
	{ SDLK_RALT, "Right Alt", "RightAlt" },
	{ SDLK_LALT, "Left Alt", "LeftAlt" },
	{ SDLK_RMETA, "Right Meta", 0 }, // ?
	{ SDLK_LMETA, "Left Meta", 0 }, // ?
	{ SDLK_LSUPER, "Left Super", "LeftWin" },        /* "Windows" keys */
	{ SDLK_RSUPER, "Right Super", "RightWin" },        
	{ SDLK_MODE, "Alt Gr", "AltGr" },
	{ SDLK_COMPOSE, "Compose", 0 }, // ?

	{ SDLK_HELP, "Help", 0 }, // ?
	{ SDLK_PRINT, "Print Screen", "PrtSc" },
	{ SDLK_SYSREQ, "SysRq", 0 },
	{ SDLK_BREAK, "Break", 0 },
	{ SDLK_MENU, "Menu", 0 }, // ?
	{ SDLK_POWER, "Power", 0 }, // ?
	{ SDLK_EURO, "Euro", 0 },
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
	{ UNIFIED_META, "Meta", "AnyMeta" },
	{ UNIFIED_SUPER, "Super", "AnyWindows" },
	{ 0, 0, 0 },
};

void InitKeyNameMap()
{
	SKeycodeMapping* it = keycodeMapping;
	while( it->keycode != 0 )
	{
		keymap.insert( std::pair<CStr,int>( CStr( it->keyname ).LowerCase(), it->keycode ) );
		if( it->altkeyname )
			keymap.insert( std::pair<CStr,int>( CStr( it->altkeyname ).LowerCase(), it->keycode ) );
		it++;
	};
}

int FindKeyCode( const CStr& keyname )
{
	std::map<CStr,int>::iterator it;
	it = keymap.find( keyname.LowerCase() );
	if( it != keymap.end() )
		return( it->second );
	return( 0 );
}

CStr FindKeyName( int keycode )
{	
	SKeycodeMapping* it = keycodeMapping;
	while( it->keycode != 0 )
	{
		if( it->keycode == keycode )
			return( CStr( it->keyname ) );
	}
	return( CStr( "Unknown" ) );
}


