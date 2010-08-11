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
#include "Hotkey.h"

#include "lib/input.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "CConsole.h"
#include "CStr.h"
#include "ps/Globals.h"
#include "KeyName.h"

extern CConsole* g_Console;

static bool unified[5];

/* SDL-type */

struct SHotkeyMapping
{
	int mapsTo;
	bool negation;
	std::vector<int> requires;
	SHotkeyMapping() : mapsTo(-1) {}
};

typedef std::vector<SHotkeyMapping> KeyMapping;

/**
 * HK_MAX_KEYCODES: Global maximum number of keycodes, including our "fake" keycodes for
 * mouse buttons and unified modifiers.
 */
const int HK_MAX_KEYCODES = UNIFIED_SUPER + 1;

// A mapping of keycodes onto sets of SDL event codes
static KeyMapping hotkeyMap[HK_MAX_KEYCODES];

// An array of the status of virtual keys
bool hotkeys[HOTKEY_LAST];


struct SHotkeyInfo
{
	int code;
	const char* name;
	int defaultmapping1, defaultmapping2;
};

// Will phase out the default shortcuts at sometime in the near future
// (or, failing that, will update them so they can do the tricky stuff
// the config file can.)

static SHotkeyInfo hotkeyInfo[] =
{
	{ HOTKEY_EXIT, "exit", SDLK_ESCAPE, 0 },
	{ HOTKEY_SCREENSHOT, "screenshot", SDLK_PRINT, 0 },
	{ HOTKEY_BIGSCREENSHOT, "bigscreenshot", 0, 0 },
	{ HOTKEY_WIREFRAME, "wireframe", SDLK_w, 0 },
	{ HOTKEY_TOGGLEFULLSCREEN, "togglefullscreen", 0, 0 },
	{ HOTKEY_CAMERA_RESET, "camera.reset", 0, 0 },
	{ HOTKEY_CAMERA_RESET_ORIGIN, "camera.reset.origin", SDLK_h, 0 },
	{ HOTKEY_CAMERA_ZOOM_IN, "camera.zoom.in", SDLK_PLUS, SDLK_KP_PLUS },
	{ HOTKEY_CAMERA_ZOOM_OUT, "camera.zoom.out", SDLK_MINUS, SDLK_KP_MINUS },
	{ HOTKEY_CAMERA_ZOOM_WHEEL_IN, "camera.zoom.wheel.in", MOUSE_WHEELUP, 0 },
	{ HOTKEY_CAMERA_ZOOM_WHEEL_OUT, "camera.zoom.wheel.out", MOUSE_WHEELDOWN, 0 },
	{ HOTKEY_CAMERA_ROTATE, "camera.rotate", 0, 0 },
	{ HOTKEY_CAMERA_ROTATE_KEYBOARD, "camera.rotate.keyboard", 0, 0 },
	{ HOTKEY_CAMERA_ROTATE_ABOUT_TARGET, "camera.rotate.abouttarget", 0, 0 },
	{ HOTKEY_CAMERA_ROTATE_ABOUT_TARGET_KEYBOARD, "camera.rotate.abouttarget.keyboard", 0, 0 },
	{ HOTKEY_CAMERA_PAN, "camera.pan", MOUSE_MIDDLE, 0 },
	{ HOTKEY_CAMERA_PAN_KEYBOARD, "camera.pan.keyboard", 0, 0 },
	{ HOTKEY_CAMERA_LEFT, "camera.left", SDLK_LEFT, 0 },
	{ HOTKEY_CAMERA_RIGHT, "camera.right", SDLK_RIGHT, 0 },
	{ HOTKEY_CAMERA_UP, "camera.up", SDLK_UP, 0 },
	{ HOTKEY_CAMERA_DOWN, "camera.down", SDLK_DOWN, 0 },
	{ HOTKEY_CAMERA_UNIT_VIEW, "camera.unit.view", SDLK_u, 0 },
	{ HOTKEY_CAMERA_UNIT_ATTACH, "camera.unit.attach", SDLK_l, 0 },
	{ HOTKEY_CAMERA_BOOKMARK_0, "camera.bookmark.0", SDLK_F5, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_1, "camera.bookmark.1", SDLK_F6, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_2, "camera.bookmark.2", SDLK_F7, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_3, "camera.bookmark.3", SDLK_F8, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_4, "camera.bookmark.4", 0, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_5, "camera.bookmark.5", 0, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_6, "camera.bookmark.6", 0, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_7, "camera.bookmark.7", 0, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_8, "camera.bookmark.8", 0, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_9, "camera.bookmark.9", 0, 0, },
	{ HOTKEY_CAMERA_BOOKMARK_SAVE, "camera.bookmark.save", 0, 0 },
	{ HOTKEY_CAMERA_BOOKMARK_SNAP, "camera.bookmark.snap", 0, 0 },
	{ HOTKEY_CAMERA_CINEMA_ADD, "camera.cinema.add", SDLK_l, 0 },
	{ HOTKEY_CAMERA_CINEMA_DELETE, "camera.cinema.delete", SDLK_u, 0 },
	{ HOTKEY_CAMERA_CINEMA_DELETE_ALL, "camera.cinema.delete.all", SDLK_r, 0 },
	{ HOTKEY_CAMERA_CINEMA_QUEUE, "camera.cinema.write", SDLK_i, 0},
	{ HOTKEY_CONSOLE_TOGGLE, "console.toggle", SDLK_F1, 0 },
	{ HOTKEY_CONSOLE_COPY, "console.copy", 0, 0 },
	{ HOTKEY_CONSOLE_PASTE, "console.paste", 0, 0 },
	{ HOTKEY_SELECTION_ADD, "selection.add", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_SELECTION_REMOVE, "selection.remove", SDLK_LCTRL, SDLK_RCTRL },
	{ HOTKEY_SELECTION_GROUP_0, "selection.group.0", SDLK_0, 0, },
	{ HOTKEY_SELECTION_GROUP_1, "selection.group.1", SDLK_1, 0, },
	{ HOTKEY_SELECTION_GROUP_2, "selection.group.2", SDLK_2, 0, },
	{ HOTKEY_SELECTION_GROUP_3, "selection.group.3", SDLK_3, 0, },
	{ HOTKEY_SELECTION_GROUP_4, "selection.group.4", SDLK_4, 0, },
	{ HOTKEY_SELECTION_GROUP_5, "selection.group.5", SDLK_5, 0, },
	{ HOTKEY_SELECTION_GROUP_6, "selection.group.6", SDLK_6, 0, },
	{ HOTKEY_SELECTION_GROUP_7, "selection.group.7", SDLK_7, 0, },
	{ HOTKEY_SELECTION_GROUP_8, "selection.group.8", SDLK_8, 0, },
	{ HOTKEY_SELECTION_GROUP_9, "selection.group.9", SDLK_9, 0, },
	{ HOTKEY_SELECTION_GROUP_10, "selection.group.10", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_11, "selection.group.11", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_12, "selection.group.12", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_13, "selection.group.13", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_14, "selection.group.14", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_15, "selection.group.15", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_16, "selection.group.16", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_17, "selection.group.17", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_18, "selection.group.18", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_19, "selection.group.19", 0, 0, },
	{ HOTKEY_SELECTION_GROUP_ADD, "selection.group.add", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_SELECTION_GROUP_SAVE, "selection.group.save", SDLK_LCTRL, SDLK_RCTRL },
	{ HOTKEY_SELECTION_GROUP_SNAP, "selection.group.snap", SDLK_LALT, SDLK_RALT },
	{ HOTKEY_SELECTION_SNAP, "selection.snap", SDLK_HOME, 0 },
	{ HOTKEY_ORDER_QUEUE, "order.queue", SDLK_LSHIFT, SDLK_RSHIFT },
	{ HOTKEY_CONTEXTORDER_NEXT, "contextorder.next", SDLK_RIGHTBRACKET, 0 },
	{ HOTKEY_CONTEXTORDER_PREVIOUS, "contextorder.previous", SDLK_LEFTBRACKET, 0 },
	{ HOTKEY_HIGHLIGHTALL, "highlightall", SDLK_o, 0 },
	{ HOTKEY_PROFILE_TOGGLE, "profile.toggle", SDLK_F11, 0 },
	{ HOTKEY_PROFILE_SAVE, "profile.save", 0, 0 },
	{ HOTKEY_PLAYMUSIC, "playmusic", SDLK_p, 0 },
	{ HOTKEY_PAUSE, "pause", SDLK_PAUSE, 0 },
	{ HOTKEY_SPEED_INCREASE, "speed.increase", 0, 0 },
	{ HOTKEY_SPEED_DECREASE, "speed.decrease", 0, 0 },
	{ HOTKEY_KILL, "killUnit", 0, 0 },
	{ HOTKEY_CHAT, "chat", 0, 0 }
};

/* SDL-type ends */

/* GUI-type */

struct SHotkeyMappingGui
{
	CStr mapsTo;
	bool negation;
	std::vector<int> requires;
	SHotkeyMappingGui() : mapsTo(-1) {}
};

typedef std::vector<SHotkeyMappingGui> GuiMapping;

// A mapping of keycodes onto sets of hotkey name strings (e.g. '[hotkey.]camera.reset')
static GuiMapping hotkeyMapGui[HK_MAX_KEYCODES];

typedef std::vector<CStr> GuiObjectList; // A list of GUI objects
typedef std::map<CStr,GuiObjectList> GuiHotkeyMap; // A mapping of name strings to lists of GUI objects that they trigger

static GuiHotkeyMap guiHotkeyMap;

// Look up a key binding in the config file and set the mappings for
// all key combinations that trigger it.
static void setBindings( const CStr& hotkeyName, int integerMapping = -1 )
{
	CConfigValueSet* binding = g_ConfigDB.GetValues( CFG_USER, CStr( "hotkey." ) + hotkeyName );
	if( binding )
	{
		int mapping;
		
		CConfigValueSet::iterator it;
		CParser multikeyParser;
		multikeyParser.InputTaskType( "multikey", "<[!$arg(_negate)][~$arg(_negate)]$value_+_>_[!$arg(_negate)][~$arg(_negate)]$value" );

		// Iterate through the bindings for this event...

		for( it = binding->begin(); it != binding->end(); it++ )
		{
			std::string hotkey;
			if( it->GetString( hotkey ) )
			{
				std::vector<int> keyCombination;

				CParserLine multikeyIdentifier;
				multikeyIdentifier.ParseString( multikeyParser, hotkey );

				// Iterate through multiple-key bindings (e.g. Ctrl+I)

				bool negateNext = false;

				for( size_t t = 0; t < multikeyIdentifier.GetArgCount(); t++ )
				{
					
					if( multikeyIdentifier.GetArgString( (int)t, hotkey ) )
					{
						if( hotkey == "_negate" )
						{
							negateNext = true;
							continue;
						}

						// Attempt decode as key name
						mapping = FindKeyCode( hotkey );	

						// Attempt to decode as a negation of a keyname
						// Yes, it's going a bit far, perhaps.
						// Too powerful for most uses, probably.
						// However, it got some hardcoding out of the engine.
						// Thus it makes me happy.
						
						if( !mapping )
							if( !it->GetInt( mapping ) )	// Attempt decode as key code
							{
								LOG(CLogger::Warning, L"hotkey", L"Couldn't map '%hs'", hotkey.c_str() );
								continue;
							}

						if( negateNext ) mapping |= HOTKEY_NEGATION_FLAG;

						negateNext = false;

						keyCombination.push_back( mapping );
					}
				}
				
				std::vector<int>::iterator itKey, itKey2;

				SHotkeyMapping bindCode;
				SHotkeyMappingGui bindName;

				for( itKey = keyCombination.begin(); itKey != keyCombination.end(); itKey++ )
				{
					bindName.mapsTo = hotkeyName;
					bindName.negation = ( ( *itKey & HOTKEY_NEGATION_FLAG ) ? true : false );
					bindName.requires.clear();
					if( integerMapping != -1 )
					{
						bindCode.mapsTo = integerMapping;
						bindCode.negation = ( ( *itKey & HOTKEY_NEGATION_FLAG ) ? true : false );
						bindCode.requires.clear();
					}
					for( itKey2 = keyCombination.begin(); itKey2 != keyCombination.end(); itKey2++ )
					{
						// Push any auxiliary keys.
						if( itKey != itKey2 )
						{
							bindName.requires.push_back( *itKey2 );
							if( integerMapping != -1 )
								bindCode.requires.push_back( *itKey2 );
						}
					}

					hotkeyMapGui[*itKey & ~HOTKEY_NEGATION_FLAG].push_back( bindName );
					if( integerMapping != -1 )
						hotkeyMap[*itKey & ~HOTKEY_NEGATION_FLAG].push_back( bindCode );
				}
			}
		}
	}
	else if( integerMapping != -1 )
	{
		SHotkeyMapping bind[2];
		bind[0].mapsTo = integerMapping;
		bind[1].mapsTo = integerMapping;
		bind[0].requires.clear();
		bind[1].requires.clear();
		bind[0].negation = false;
		bind[1].negation = false;
		hotkeyMap[ hotkeyInfo[integerMapping].defaultmapping1 ].push_back( bind[0] );
		if( hotkeyInfo[integerMapping].defaultmapping2 )
			hotkeyMap[ hotkeyInfo[integerMapping].defaultmapping2 ].push_back( bind[1] );
	}
}
void LoadHotkeys()
{
	InitKeyNameMap();

	for(int i = 0; i < HOTKEY_LAST; i++ )
		setBindings( hotkeyInfo[i].name, i );
	
	// Set up the state of the hotkeys given no key is down.
	// i.e. find those hotkeys triggered by all negations.

	std::vector<SHotkeyMapping>::iterator it;
	std::vector<int>::iterator j;
	bool allNegated;

	for(int i = 1; i < HK_MAX_KEYCODES; i++ )
	{
		for( it = hotkeyMap[i].begin(); it != hotkeyMap[i].end(); it++ )
		{
			if( !it->negation )
				continue;

			allNegated = true;

			for( j = it->requires.begin(); j != it->requires.end(); j++ )
				if( !( *j & HOTKEY_NEGATION_FLAG ) )
					allNegated = false;
			
			debug_assert((size_t)it->mapsTo < ARRAY_SIZE(hotkeys));
			
			if( allNegated )
				hotkeys[it->mapsTo] = true;
		}
	}
}

void HotkeyRegisterGuiObject( const CStr& objName, const CStr& hotkeyName )
{
	GuiObjectList& boundTo = guiHotkeyMap[hotkeyName];
	if( boundTo.empty() )
	{
		// Load keybindings from the config file
		setBindings( hotkeyName );
	}
	boundTo.push_back( objName );
}

InReaction HotkeyInputHandler( const SDL_Event_* ev )
{
	int keycode = 0;

	switch( ev->ev.type )
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		keycode = (int)ev->ev.key.keysym.sym;
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		if ((int)ev->ev.button.button <= SDL_BUTTON_WHEELDOWN)
		{
			keycode = SDLK_LAST + (int)ev->ev.button.button;
			break;
		}
		// fall through
	default:
		return IN_PASS;
	}

	// Somewhat hackish:
	// Create phantom 'unified-modifier' events when left- or right- modifier keys are pressed
	// Just send them to this handler; don't let the imaginary event codes leak back to real SDL.

	SDL_Event_ phantom;
	phantom.ev.type = ( ( ev->ev.type == SDL_KEYDOWN ) || ( ev->ev.type == SDL_MOUSEBUTTONDOWN ) ) ? SDL_KEYDOWN : SDL_KEYUP;
	if( ( keycode == SDLK_LSHIFT ) || ( keycode == SDLK_RSHIFT ) )
	{
		phantom.ev.key.keysym.sym = (SDLKey)UNIFIED_SHIFT;
		unified[0] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
	else if( ( keycode == SDLK_LCTRL ) || ( keycode == SDLK_RCTRL ) )
	{
		phantom.ev.key.keysym.sym = (SDLKey)UNIFIED_CTRL;
		unified[1] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
	else if( ( keycode == SDLK_LALT ) || ( keycode == SDLK_RALT ) )
	{
		phantom.ev.key.keysym.sym = (SDLKey)UNIFIED_ALT;
		unified[2] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
	else if( ( keycode == SDLK_LMETA ) || ( keycode == SDLK_RMETA ) )
	{
		phantom.ev.key.keysym.sym = (SDLKey)UNIFIED_META;
		unified[3] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
	else if( ( keycode == SDLK_LSUPER ) || ( keycode == SDLK_RSUPER ) )
	{
		phantom.ev.key.keysym.sym = (SDLKey)UNIFIED_SUPER;
		unified[4] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}

	// Inhibit the dispatch of hotkey events caused by printable or control keys
	// while the console is up. (But allow multiple-key - 'Ctrl+F' events, and whatever
	// key toggles the console.)

	bool consoleCapture = false, isCapturable;

	if( g_Console->IsActive() && ( 
	    ( keycode == 8 ) || ( keycode == 9 ) || ( keycode == 13 ) || /* Editing */
		( ( keycode >= 32 ) && ( keycode < 273 ) ) ||				 /* Printable (<128), 'World' (<256) */
		( ( keycode >= 273 ) && ( keycode < 282 ) &&				 /* Numeric keypad (<273), navigation */
		  ( keycode != SDLK_INSERT ) ) ) )							 /* keys (<282) except insert */
		consoleCapture = true;

	std::vector<SHotkeyMapping>::iterator it;
	std::vector<SHotkeyMappingGui>::iterator itGUI;

	SDL_Event hotkeyNotification;

	// Here's an interesting bit:
	// If you have an event bound to, say, 'F', and another to, say, 'Ctrl+F', pressing
	// 'F' while control is down would normally fire off both.

	// To avoid this, set the modifier keys for /all/ events this key would trigger
	// (Ctrl, for example, is both group-save and bookmark-save)
	// but only send a HotkeyDown event for the event with bindings most precisely
	// matching the conditions (i.e. the event with the highest number of auxiliary
	// keys, providing they're all down)

	bool typeKeyDown = ( ev->ev.type == SDL_KEYDOWN ) || ( ev->ev.type == SDL_MOUSEBUTTONDOWN );
	
	// -- KEYDOWN SECTION -- 

	// SDL-events bit

	size_t closestMap = 0;	// avoid "uninitialized" warning
	size_t closestMapMatch = 0;

	for( it = hotkeyMap[keycode].begin(); it < hotkeyMap[keycode].end(); it++ )
	{
		// If a key has been pressed, and this event triggers on it's release, skip it.
		// Similarly, if the key's been released and the event triggers on a keypress, skip it.
		if( it->negation == typeKeyDown )
			continue;

		// Check to see if all auxiliary keys are down
		
		std::vector<int>::iterator itKey;
		bool accept = true;
		isCapturable = true;

		for( itKey = it->requires.begin(); itKey != it->requires.end(); itKey++ )
		{
			int keyCode = *itKey & ~HOTKEY_NEGATION_FLAG; // Clear the negation-modifier bit
			bool rqdState = ( *itKey & HOTKEY_NEGATION_FLAG ) == 0;

			// debug_assert( !rqdState );

			if( keyCode < SDLK_LAST )
			{
				if( g_keys[keyCode] != rqdState ) accept = false;
			}
			else if( keyCode < UNIFIED_SHIFT )
			{
				if( g_mouse_buttons[keyCode-SDLK_LAST] != rqdState ) accept = false;
			}
			else if( (size_t)(keyCode-UNIFIED_SHIFT) < ARRAY_SIZE(unified) )
			{
				if( unified[keyCode-UNIFIED_SHIFT] != rqdState ) accept = false;
			}
			else
			{
				debug_printf(L"keyCode = %i\n", keyCode);
				debug_warn(L"keyCode out of range in GUI hotkey requirements");
			}

			// If this event requires a multiple keypress (with the exception
			// of shift+key combinations) the console won't inhibit it.
			if( rqdState && ( *itKey != SDLK_RSHIFT ) && ( *itKey != SDLK_LSHIFT ) )
				isCapturable = false;
		}

		if( it->mapsTo == HOTKEY_CONSOLE_TOGGLE ) isCapturable = false; // Because that would be silly.

		debug_assert((size_t)it->mapsTo < ARRAY_SIZE(hotkeys));
		
		if( accept && !( isCapturable && consoleCapture ) )
		{
			hotkeys[it->mapsTo] = true;
			if( it->requires.size() >= closestMapMatch )
			{
				// Only if it's a more precise match, and it either isn't capturable or the console won't capture it.
				closestMap = it->mapsTo;
				closestMapMatch = it->requires.size() + 1;
			}
		}
	}

	if( closestMapMatch )
	{
		hotkeyNotification.type = SDL_HOTKEYDOWN;
		hotkeyNotification.user.code = (int)closestMap;
		SDL_PushEvent( &hotkeyNotification );
	}
	// GUI bit... could do with some optimization later.

	CStr closestMapName = -1;
	closestMapMatch = 0;

	for( itGUI = hotkeyMapGui[keycode].begin(); itGUI != hotkeyMapGui[keycode].end(); itGUI++ )
	{	
		// If a key has been pressed, and this event triggers on it's release, skip it.
		// Similarly, if the key's been released and the event triggers on a keypress, skip it.
		if( itGUI->negation == typeKeyDown )
			continue;

		// Check to see if all auxiliary keys are down

		std::vector<int>::iterator itKey;
		bool accept = true;
		isCapturable = true;

		for( itKey = itGUI->requires.begin(); itKey != itGUI->requires.end(); itKey++ )
		{
			int keyCode = *itKey & ~HOTKEY_NEGATION_FLAG; // Clear the negation-modifier bit
			bool rqdState = ( *itKey & HOTKEY_NEGATION_FLAG ) == 0;

			if( keyCode < SDLK_LAST )
			{
				if( g_keys[keyCode] != rqdState ) accept = false;
			}
			else if( keyCode < UNIFIED_SHIFT )
			{
				if( g_mouse_buttons[keyCode-SDLK_LAST] != rqdState ) accept = false;
			}
			else if( (size_t)(keyCode-UNIFIED_SHIFT) < ARRAY_SIZE(unified) )
			{
				if( unified[keyCode-UNIFIED_SHIFT] != rqdState ) accept = false;
			}
			else
			{
				debug_printf(L"keyCode = %i\n", keyCode);
				debug_warn(L"keyCode out of range in GUI hotkey requirements");
			}

			// If this event requires a multiple keypress (with the exception
			// of shift+key combinations) the console won't inhibit it.
			if( rqdState && ( *itKey != SDLK_RSHIFT ) && ( *itKey != SDLK_LSHIFT ) )
				isCapturable = false;
		}

		if( accept && !( isCapturable && consoleCapture ) )
		{
			if( itGUI->requires.size() >= closestMapMatch )
			{
				closestMapName = itGUI->mapsTo;
				closestMapMatch = itGUI->requires.size() + 1;
			}
		}
	}
	// GUI-objects bit
	// This fragment is an obvious candidate for rewriting when speed becomes an issue.

	if( closestMapMatch )
	{
		GuiHotkeyMap::iterator map_it;
		GuiObjectList::iterator obj_it;
		map_it = guiHotkeyMap.find( closestMapName );
		if( map_it != guiHotkeyMap.end() )
		{
			GuiObjectList& targets = map_it->second;
			for( obj_it = targets.begin(); obj_it != targets.end(); obj_it++ )
			{
				hotkeyNotification.type = SDL_GUIHOTKEYPRESS;
				hotkeyNotification.user.data1 = &(*obj_it);
				SDL_PushEvent( &hotkeyNotification );
			}
		}	
	}

	// -- KEYUP SECTION --

	for( it = hotkeyMap[keycode].begin(); it < hotkeyMap[keycode].end(); it++ )
	{
		// If it's a keydown event, won't cause HotKeyUps in anything that doesn't
		// use this key negated => skip them
		// If it's a keyup event, won't cause HotKeyUps in anything that does use
		// this key negated => skip them too.
		if( it->negation != typeKeyDown )
			continue;

		// Check to see if all auxiliary keys are down

		std::vector<int>::iterator itKey;
		bool accept = true;

		for( itKey = it->requires.begin(); itKey != it->requires.end(); itKey++ )
		{
			if( *itKey < SDLK_LAST )
			{
				if( !g_keys[*itKey] ) accept = false;
			}
			else if( *itKey < UNIFIED_SHIFT )
			{
				if( !g_mouse_buttons[(*itKey)-SDLK_LAST] ) accept = false;
			}
			else if( *itKey < HOTKEY_NEGATION_FLAG )
			{
				if( !unified[(*itKey)-UNIFIED_SHIFT] ) accept = false;
			}
		}

		debug_assert((size_t)it->mapsTo < ARRAY_SIZE(hotkeys));
		
		if( accept )
		{
			hotkeys[it->mapsTo] = false;
			hotkeyNotification.type = SDL_HOTKEYUP;
			hotkeyNotification.user.code = it->mapsTo;
			SDL_PushEvent( &hotkeyNotification );
		}
	}

	return( IN_PASS );
}

CStr HotkeyGetName(int hotkey)
{
	if (hotkey < 0 || hotkey >= HOTKEY_LAST)
		return "";
	return hotkeyInfo[hotkey].name;
}

bool HotkeyRespondsTo(int hotkey, int sdlkey)
{
	for (KeyMapping::iterator it = hotkeyMap[sdlkey].begin(); it != hotkeyMap[sdlkey].end(); ++it)
		if (it->mapsTo == hotkey)
			return true;
	return false;
}


bool HotkeyIsPressed(const CStr& keyname)
{
	return hotkeys[FindKeyCode(keyname)];
}
