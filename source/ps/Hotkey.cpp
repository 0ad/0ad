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
#include "Hotkey.h"

#include "lib/input.h"
#include "ConfigDB.h"
#include "CLogger.h"
#include "CConsole.h"
#include "CStr.h"
#include "ps/Globals.h"
#include "KeyName.h"

static bool unified[UNIFIED_LAST - UNIFIED_SHIFT];

#if SDL_VERSION_ATLEAST(2, 0, 0)
#define SDLKEY SDL_Keycode
#else
#define SDLKEY SDLKey
#endif

struct SKey
{
	SDLKEY code; // keycode or MOUSE_ or UNIFIED_ value
	bool negated; // whether the key must be pressed (false) or unpressed (true)
};

// Hotkey data associated with an externally-specified 'primary' keycode
struct SHotkeyMapping
{
	CStr name; // name of the hotkey
	bool negated; // whether the primary key must be pressed (false) or unpressed (true)
	std::vector<SKey> requires; // list of non-primary keys that must also be active
};

typedef std::vector<SHotkeyMapping> KeyMapping;

// A mapping of keycodes onto the hotkeys that are associated with that key.
// (A hotkey triggered by a combination of multiple keys will be in this map
// multiple times.)
static std::map<int, KeyMapping> g_HotkeyMap;

// The current pressed status of hotkeys
std::map<std::string, bool> g_HotkeyStatus;

// Look up each key binding in the config file and set the mappings for
// all key combinations that trigger it.
static void LoadConfigBindings()
{
	std::map<CStr, CConfigValueSet> bindings = g_ConfigDB.GetValuesWithPrefix( CFG_COMMAND, "hotkey." );

	CParser multikeyParser;
	multikeyParser.InputTaskType( "multikey", "<[~$arg(_negate)]$value_+_>_[~$arg(_negate)]$value" );

	for( std::map<CStr, CConfigValueSet>::iterator bindingsIt = bindings.begin(); bindingsIt != bindings.end(); ++bindingsIt )
	{
		std::string hotkeyName = bindingsIt->first.substr(7); // strip the "hotkey." prefix

		for( CConfigValueSet::iterator it = bindingsIt->second.begin(); it != bindingsIt->second.end(); ++it )
		{
			std::string hotkey;
			if( it->GetString( hotkey ) )
			{
				std::vector<SKey> keyCombination;

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
						int mapping = FindKeyCode( hotkey );

						// Attempt to decode as a negation of a keyname
						// Yes, it's going a bit far, perhaps.
						// Too powerful for most uses, probably.
						// However, it got some hardcoding out of the engine.
						// Thus it makes me happy.

						if( !mapping )
						{
							LOGWARNING(L"Hotkey mapping used invalid key '%hs'", hotkey.c_str() );
							continue;
						}

						SKey key = { (SDLKEY)mapping, negateNext };
						keyCombination.push_back(key);

						negateNext = false;

					}
				}

				std::vector<SKey>::iterator itKey, itKey2;

				for( itKey = keyCombination.begin(); itKey != keyCombination.end(); ++itKey )
				{
					SHotkeyMapping bindCode;

					bindCode.name = hotkeyName;
					bindCode.negated = itKey->negated;

					for( itKey2 = keyCombination.begin(); itKey2 != keyCombination.end(); ++itKey2 )
					{
						// Push any auxiliary keys.
						if( itKey != itKey2 )
							bindCode.requires.push_back( *itKey2 );
					}

					g_HotkeyMap[itKey->code].push_back( bindCode );
				}
			}
		}
	}
}

void LoadHotkeys()
{
	InitKeyNameMap();

	LoadConfigBindings();

	// Set up the state of the hotkeys given no key is down.
	// i.e. find those hotkeys triggered by all negations.

	for( std::map<int, KeyMapping>::iterator mapIt = g_HotkeyMap.begin(); mapIt != g_HotkeyMap.end(); ++mapIt )
	{
		KeyMapping& hotkeyMap = mapIt->second;

		for( std::vector<SHotkeyMapping>::iterator it = hotkeyMap.begin(); it != hotkeyMap.end(); ++it )
		{
			if( !it->negated )
				continue;

			bool allNegated = true;

			for( std::vector<SKey>::iterator j = it->requires.begin(); j != it->requires.end(); ++j )
				if( !j->negated )
					allNegated = false;
			
			if( allNegated )
				g_HotkeyStatus[it->name] = true;
		}
	}
}

bool isNegated(const SKey& key)
{
	// Normal keycodes are below EXTRA_KEYS_BASE
	if ((int)key.code < EXTRA_KEYS_BASE && g_keys[key.code] == key.negated)
		return false;
	// Mouse 'keycodes' are after the modifier keys
	else if ((int)key.code > UNIFIED_LAST && g_mouse_buttons[key.code - UNIFIED_LAST] == key.negated)
		return false;
	// Modifier keycodes are between the normal keys and the mouse 'keys'
	else if ((int)key.code < UNIFIED_LAST && (int)key.code > CUSTOM_SDL_KEYCODE && unified[key.code - UNIFIED_SHIFT] == key.negated)
		return false;
	else
		return true;
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
		keycode = MOUSE_BASE + (int)ev->ev.button.button;
		break;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	case SDL_MOUSEWHEEL:
		if (ev->ev.wheel.y > 0)
		{
			keycode = MOUSE_WHEELUP;
			break;
		}
		else if (ev->ev.wheel.y < 0)
		{
			keycode = MOUSE_WHEELDOWN;
			break;
		}
		return IN_PASS;
#endif

	case SDL_HOTKEYDOWN:
		g_HotkeyStatus[static_cast<const char*>(ev->ev.user.data1)] = true;
		return IN_PASS;

	case SDL_HOTKEYUP:
		g_HotkeyStatus[static_cast<const char*>(ev->ev.user.data1)] = false;
		return IN_PASS;

	default:
		return IN_PASS;
	}

	// Rather ugly hack to make the '"' key work better on a MacBook Pro on Windows so it doesn't
	// always close the console. (Maybe this would be better handled in wsdl or something?)
	if (keycode == SDLK_BACKQUOTE && (ev->ev.key.keysym.unicode == '\'' || ev->ev.key.keysym.unicode == '"'))
		keycode = ev->ev.key.keysym.unicode;

	// Somewhat hackish:
	// Create phantom 'unified-modifier' events when left- or right- modifier keys are pressed
	// Just send them to this handler; don't let the imaginary event codes leak back to real SDL.

	SDL_Event_ phantom;
	phantom.ev.type = ( ( ev->ev.type == SDL_KEYDOWN ) || ( ev->ev.type == SDL_MOUSEBUTTONDOWN ) ) ? SDL_KEYDOWN : SDL_KEYUP;
	if( ( keycode == SDLK_LSHIFT ) || ( keycode == SDLK_RSHIFT ) )
	{
		phantom.ev.key.keysym.sym = (SDLKEY)UNIFIED_SHIFT;
		unified[0] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
	else if( ( keycode == SDLK_LCTRL ) || ( keycode == SDLK_RCTRL ) )
	{
		phantom.ev.key.keysym.sym = (SDLKEY)UNIFIED_CTRL;
		unified[1] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
	else if( ( keycode == SDLK_LALT ) || ( keycode == SDLK_RALT ) )
	{
		phantom.ev.key.keysym.sym = (SDLKEY)UNIFIED_ALT;
		unified[2] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}
#if SDL_VERSION_ATLEAST(2, 0, 0)
	else if( ( keycode == SDLK_LGUI ) || ( keycode == SDLK_RGUI ) )
#else // SDL 1.2
	else if( ( keycode == SDLK_LSUPER ) || ( keycode == SDLK_RSUPER ) || ( keycode == SDLK_LMETA ) || ( keycode == SDLK_RMETA) )
#endif
	{
		phantom.ev.key.keysym.sym = (SDLKEY)UNIFIED_SUPER;
		unified[3] = ( phantom.ev.type == SDL_KEYDOWN );
		HotkeyInputHandler( &phantom );
	}

	// Check whether we have any hotkeys registered for this particular keycode
	if( g_HotkeyMap.find(keycode) == g_HotkeyMap.end() )
		return( IN_PASS );

	// Inhibit the dispatch of hotkey events caused by real keys (not fake mouse button
	// events) while the console is up.

	bool consoleCapture = false;

	if( g_Console->IsActive() && keycode < CUSTOM_SDL_KEYCODE )
		consoleCapture = true;

	// Here's an interesting bit:
	// If you have an event bound to, say, 'F', and another to, say, 'Ctrl+F', pressing
	// 'F' while control is down would normally fire off both.

	// To avoid this, set the modifier keys for /all/ events this key would trigger
	// (Ctrl, for example, is both group-save and bookmark-save)
	// but only send a HotkeyDown event for the event with bindings most precisely
	// matching the conditions (i.e. the event with the highest number of auxiliary
	// keys, providing they're all down)

#if SDL_VERSION_ATLEAST(2, 0, 0)
	bool typeKeyDown = ( ev->ev.type == SDL_KEYDOWN ) || ( ev->ev.type == SDL_MOUSEBUTTONDOWN ) || (ev->ev.type == SDL_MOUSEWHEEL);
#else
	bool typeKeyDown = ( ev->ev.type == SDL_KEYDOWN ) || ( ev->ev.type == SDL_MOUSEBUTTONDOWN );
#endif

	// -- KEYDOWN SECTION -- 

	std::vector<const char*> closestMapNames;
	size_t closestMapMatch = 0;

	for (std::vector<SHotkeyMapping>::iterator it = g_HotkeyMap[keycode].begin(); it < g_HotkeyMap[keycode].end(); ++it)
	{
		// If a key has been pressed, and this event triggers on its release, skip it.
		// Similarly, if the key's been released and the event triggers on a keypress, skip it.
		if (it->negated == typeKeyDown)
			continue;

		// Check for no unpermitted keys
		bool accept = true;
		for (std::vector<SKey>::iterator itKey = it->requires.begin(); itKey != it->requires.end() && accept; ++itKey)
			accept = isNegated(*itKey);

		if (accept && !(consoleCapture && it->name != "console.toggle"))
		{
			// Check if this is an equally precise or more precise match
			if (it->requires.size() + 1 >= closestMapMatch)
			{
				// Check if more precise
				if (it->requires.size() + 1 > closestMapMatch)
				{
					// Throw away the old less-precise matches
					closestMapNames.clear();
					closestMapMatch = it->requires.size() + 1;
				}

				closestMapNames.push_back(it->name.c_str());
			}
		}
	}

	for (size_t i = 0; i < closestMapNames.size(); ++i)
	{
		SDL_Event_ hotkeyNotification;
		hotkeyNotification.ev.type = SDL_HOTKEYDOWN;
		hotkeyNotification.ev.user.data1 = const_cast<char*>(closestMapNames[i]);
		in_push_priority_event(&hotkeyNotification);
	}

	// -- KEYUP SECTION --

	for (std::vector<SHotkeyMapping>::iterator it = g_HotkeyMap[keycode].begin(); it < g_HotkeyMap[keycode].end(); ++it)
	{
		// If it's a keydown event, won't cause HotKeyUps in anything that doesn't
		// use this key negated => skip them
		// If it's a keyup event, won't cause HotKeyUps in anything that does use
		// this key negated => skip them too.
		if (it->negated != typeKeyDown)
			continue;

		// Check for no unpermitted keys
		bool accept = true;
		for (std::vector<SKey>::iterator itKey = it->requires.begin(); itKey != it->requires.end() && accept; ++itKey)
			accept = isNegated(*itKey);

		if (accept)
		{
			SDL_Event_ hotkeyNotification;
			hotkeyNotification.ev.type = SDL_HOTKEYUP;
			hotkeyNotification.ev.user.data1 = const_cast<char*>(it->name.c_str());
			in_push_priority_event(&hotkeyNotification);
		}
	}

	return IN_PASS;
}

bool HotkeyIsPressed(const CStr& keyname)
{
	return g_HotkeyStatus[keyname];
}
