/* Copyright (C) 2010 Wildfire Games.
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

struct SKey
{
	SDLKey code; // keycode or MOUSE_ or UNIFIED_ value
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
	std::vector<std::pair<CStr, CConfigValueSet> > bindings = g_ConfigDB.GetValuesWithPrefix( CFG_USER, CStr( "hotkey." ));

	CParser multikeyParser;
	multikeyParser.InputTaskType( "multikey", "<[~$arg(_negate)]$value_+_>_[~$arg(_negate)]$value" );

	for( std::vector<std::pair<CStr, CConfigValueSet> >::iterator bindingsIt = bindings.begin(); bindingsIt != bindings.end(); ++bindingsIt )
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

						SKey key = { (SDLKey)mapping, negateNext };
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
#if SDL_VERSION_ATLEAST(1, 2, 13)
		if ((int)ev->ev.button.button <= SDL_BUTTON_X2)
#else
		if ((int)ev->ev.button.button <= SDL_BUTTON_WHEELDOWN)
#endif
		{
			keycode = SDLK_LAST + (int)ev->ev.button.button;
			break;
		}
		// fall through
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

	// Check whether we have any hotkeys registered for this particular keycode
	if( g_HotkeyMap.find(keycode) == g_HotkeyMap.end() )
		return( IN_PASS );

	// Inhibit the dispatch of hotkey events caused by real keys (not fake mouse button
	// events) while the console is up.

	bool consoleCapture = false;

	if( g_Console->IsActive() && keycode < SDLK_LAST )
		consoleCapture = true;

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

	std::vector<const char*> closestMapNames;
	size_t closestMapMatch = 0;

	for( std::vector<SHotkeyMapping>::iterator it = g_HotkeyMap[keycode].begin(); it < g_HotkeyMap[keycode].end(); ++it )
	{
		// If a key has been pressed, and this event triggers on its release, skip it.
		// Similarly, if the key's been released and the event triggers on a keypress, skip it.
		if( it->negated == typeKeyDown )
			continue;

		// Check to see if all auxiliary keys are down
		
		bool accept = true;

		for( std::vector<SKey>::iterator itKey = it->requires.begin(); itKey != it->requires.end(); ++itKey )
		{
			bool rqdState = !itKey->negated;

			if( (int)itKey->code < SDLK_LAST )
			{
				if( g_keys[itKey->code] != rqdState ) accept = false;
			}
			else if( (int)itKey->code < UNIFIED_SHIFT )
			{
				if( g_mouse_buttons[itKey->code - SDLK_LAST] != rqdState ) accept = false;
			}
			else if( (int)itKey->code < UNIFIED_LAST )
			{
				if( unified[itKey->code - UNIFIED_SHIFT] != rqdState ) accept = false;
			}
		}

		if( accept && !( consoleCapture && it->name != "console.toggle" ) )
		{
			// Tentatively set status to un-pressed, since it may be overridden by
			// a closer match. (The closest matches will be set to pressed later.)
			g_HotkeyStatus[it->name] = false;

			// Check if this is an equally precise or more precise match
			if( it->requires.size() + 1 >= closestMapMatch )
			{
				// Check if more precise
				if( it->requires.size() + 1 > closestMapMatch )
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
		g_HotkeyStatus[closestMapNames[i]] = true;

		SDL_Event hotkeyNotification;
		hotkeyNotification.type = SDL_HOTKEYDOWN;
		hotkeyNotification.user.data1 = const_cast<char*>(closestMapNames[i]);
		SDL_PushEvent(&hotkeyNotification);
	}

	// -- KEYUP SECTION --

	for( std::vector<SHotkeyMapping>::iterator it = g_HotkeyMap[keycode].begin(); it < g_HotkeyMap[keycode].end(); ++it )
	{
		// If it's a keydown event, won't cause HotKeyUps in anything that doesn't
		// use this key negated => skip them
		// If it's a keyup event, won't cause HotKeyUps in anything that does use
		// this key negated => skip them too.
		if( it->negated != typeKeyDown )
			continue;

		// Check to see if all auxiliary keys are down

		bool accept = true;

		for( std::vector<SKey>::iterator itKey = it->requires.begin(); itKey != it->requires.end(); ++itKey )
		{
			bool rqdState = !itKey->negated;

			if( (int)itKey->code < SDLK_LAST )
			{
				if( g_keys[itKey->code] != rqdState ) accept = false;
			}
			else if( (int)itKey->code < UNIFIED_SHIFT )
			{
				if( g_mouse_buttons[itKey->code - SDLK_LAST] != rqdState ) accept = false;
			}
			else if( (int)itKey->code < UNIFIED_LAST )
			{
				if( unified[itKey->code - UNIFIED_SHIFT] != rqdState ) accept = false;
			}
		}

		if( accept )
		{
			g_HotkeyStatus[it->name] = false;
			SDL_Event hotkeyNotification;
			hotkeyNotification.type = SDL_HOTKEYUP;
			hotkeyNotification.user.data1 = const_cast<char*>(it->name.c_str());
			SDL_PushEvent( &hotkeyNotification );
		}
	}

	return( IN_PASS );
}

bool HotkeyIsPressed(const CStr& keyname)
{
	return g_HotkeyStatus[keyname];
}
