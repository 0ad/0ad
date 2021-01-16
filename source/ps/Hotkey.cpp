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

#include "precompiled.h"
#include "Hotkey.h"

#include <boost/tokenizer.hpp>

#include "lib/external_libraries/libsdl.h"
#include "ps/CConsole.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/ConfigDB.h"
#include "ps/Globals.h"
#include "ps/KeyName.h"

static bool unified[UNIFIED_LAST - UNIFIED_SHIFT];

std::unordered_map<int, KeyMapping> g_HotkeyMap;
std::unordered_map<std::string, bool> g_HotkeyStatus;

static_assert(std::is_integral<std::underlying_type<SDL_Scancode>::type>::value, "SDL_Scancode is not an integral enum.");
static_assert(SDL_USEREVENT_ == SDL_USEREVENT, "SDL_USEREVENT_ is not the same type as the real SDL_USEREVENT");
static_assert(UNUSED_HOTKEY_CODE == SDL_SCANCODE_UNKNOWN);

// Look up each key binding in the config file and set the mappings for
// all key combinations that trigger it.
static void LoadConfigBindings()
{
	for (const std::pair<const CStr, CConfigValueSet>& configPair : g_ConfigDB.GetValuesWithPrefix(CFG_COMMAND, "hotkey."))
	{
		std::string hotkeyName = configPair.first.substr(7); // strip the "hotkey." prefix

		if (configPair.second.empty())
		{
			// Unused hotkeys must still be registered in the map to appear in the hotkey editor.
			SHotkeyMapping unusedCode;
			unusedCode.name = hotkeyName;
			unusedCode.negated = false;
			g_HotkeyMap[UNUSED_HOTKEY_CODE].push_back(unusedCode);
			continue;
		}

		for (const CStr& hotkey : configPair.second)
		{
			std::vector<SKey> keyCombination;

			// Iterate through multiple-key bindings (e.g. Ctrl+I)
			boost::char_separator<char> sep("+");
			typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
			tokenizer tok(hotkey, sep);
			for (tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
			{
				// Attempt decode as key name
				SDL_Scancode scancode = FindScancode(it->c_str());
				if (!scancode)
				{
					LOGWARNING("Hotkey mapping used invalid key '%s'", hotkey.c_str());
					continue;
				}

				SKey key = { scancode, false };
				keyCombination.push_back(key);
			}

			std::vector<SKey>::iterator itKey, itKey2;
			for (itKey = keyCombination.begin(); itKey != keyCombination.end(); ++itKey)
			{
				SHotkeyMapping bindCode;

				bindCode.name = hotkeyName;
				bindCode.negated = itKey->negated;

				for (itKey2 = keyCombination.begin(); itKey2 != keyCombination.end(); ++itKey2)
					if (itKey != itKey2) // Push any auxiliary keys
						bindCode.requires.push_back(*itKey2);

				g_HotkeyMap[itKey->code].push_back(bindCode);
			}
		}
	}
}

void LoadHotkeys()
{
	LoadConfigBindings();

	// Set up the state of the hotkeys given no key is down.
	// i.e. find those hotkeys triggered by all negations.

	for (const std::pair<const int, KeyMapping>& p : g_HotkeyMap)
		for (const SHotkeyMapping& hotkey : p.second)
		{
			if (!hotkey.negated)
				continue;

			bool allNegated = true;

			for (const SKey& k : hotkey.requires)
				if (!k.negated)
					allNegated = false;

			if (allNegated)
				g_HotkeyStatus[hotkey.name] = true;
		}
}

void UnloadHotkeys()
{
	g_HotkeyMap.clear();
	g_HotkeyStatus.clear();
}

bool isNegated(const SKey& key)
{
	// Normal keycodes are below EXTRA_KEYS_BASE
	if ((int)key.code < EXTRA_KEYS_BASE && g_keys[key.code] == key.negated)
		return false;
	// Mouse 'keycodes' are after the modifier keys
	else if ((int)key.code < MOUSE_LAST && (int)key.code > MOUSE_BASE && g_mouse_buttons[key.code - MOUSE_BASE] == key.negated)
		return false;
	// Modifier keycodes are between the normal keys and the mouse 'keys'
	else if ((int)key.code < UNIFIED_LAST && (int)key.code > SDL_NUM_SCANCODES && unified[key.code - UNIFIED_SHIFT] == key.negated)
		return false;
	else
		return true;
}

InReaction HotkeyStateChange(const SDL_Event_* ev)
{
	if (ev->ev.type == SDL_HOTKEYPRESS)
		g_HotkeyStatus[static_cast<const char*>(ev->ev.user.data1)] = true;
	else if (ev->ev.type == SDL_HOTKEYUP)
		g_HotkeyStatus[static_cast<const char*>(ev->ev.user.data1)] = false;
	return IN_PASS;
}

InReaction HotkeyInputHandler(const SDL_Event_* ev)
{
	int scancode = SDL_SCANCODE_UNKNOWN;

	switch(ev->ev.type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		scancode = ev->ev.key.keysym.scancode;
		break;

	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		// Mousewheel events are no longer buttons, but we want to maintain the order
		// expected by g_mouse_buttons for compatibility
		if (ev->ev.button.button >= SDL_BUTTON_X1)
			scancode = MOUSE_BASE + (int)ev->ev.button.button + 2;
		else
			scancode = MOUSE_BASE + (int)ev->ev.button.button;
		break;

	case SDL_MOUSEWHEEL:
		if (ev->ev.wheel.y > 0)
		{
			scancode = MOUSE_WHEELUP;
			break;
		}
		else if (ev->ev.wheel.y < 0)
		{
			scancode = MOUSE_WHEELDOWN;
			break;
		}
		else if (ev->ev.wheel.x > 0)
		{
			scancode = MOUSE_X2;
			break;
		}
		else if (ev->ev.wheel.x < 0)
		{
			scancode = MOUSE_X1;
			break;
		}
		return IN_PASS;


	default:
		return IN_PASS;
	}

	// Somewhat hackish:
	// Create phantom 'unified-modifier' events when left- or right- modifier keys are pressed
	// Just send them to this handler; don't let the imaginary event codes leak back to real SDL.

	SDL_Event_ phantom;
	phantom.ev.type = ((ev->ev.type == SDL_KEYDOWN) || (ev->ev.type == SDL_MOUSEBUTTONDOWN)) ? SDL_KEYDOWN : SDL_KEYUP;
	if (phantom.ev.type == SDL_KEYDOWN)
		phantom.ev.key.repeat = ev->ev.type == SDL_KEYDOWN ? ev->ev.key.repeat : 0;

	if (scancode == SDL_SCANCODE_LSHIFT || scancode == SDL_SCANCODE_RSHIFT)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_SHIFT);
		unified[0] = (phantom.ev.type == SDL_KEYDOWN);
		HotkeyInputHandler(&phantom);
	}
	else if (scancode == SDL_SCANCODE_LCTRL || scancode == SDL_SCANCODE_RCTRL)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_CTRL);
		unified[1] = (phantom.ev.type == SDL_KEYDOWN);
		HotkeyInputHandler(&phantom);
	}
	else if (scancode == SDL_SCANCODE_LALT || scancode == SDL_SCANCODE_RALT)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_ALT);
		unified[2] = (phantom.ev.type == SDL_KEYDOWN);
		HotkeyInputHandler(&phantom);
	}
	else if (scancode == SDL_SCANCODE_LGUI || scancode == SDL_SCANCODE_RGUI)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_SUPER);
		unified[3] = (phantom.ev.type == SDL_KEYDOWN);
		HotkeyInputHandler(&phantom);
	}

	// Check whether we have any hotkeys registered for this particular keycode
	if (g_HotkeyMap.find(scancode) == g_HotkeyMap.end())
		return (IN_PASS);

	// Inhibit the dispatch of hotkey events caused by real keys (not fake mouse button
	// events) while the console is up.

	bool consoleCapture = false;

	if (g_Console && g_Console->IsActive() && scancode < SDL_NUM_SCANCODES)
		consoleCapture = true;

	// Here's an interesting bit:
	// If you have an event bound to, say, 'F', and another to, say, 'Ctrl+F', pressing
	// 'F' while control is down would normally fire off both.

	// To avoid this, set the modifier keys for /all/ events this key would trigger
	// (Ctrl, for example, is both group-save and bookmark-save)
	// but only send a HotkeyPress/HotkeyDown event for the event with bindings most precisely
	// matching the conditions (i.e. the event with the highest number of auxiliary
	// keys, providing they're all down)

	bool typeKeyDown = ( ev->ev.type == SDL_KEYDOWN ) || ( ev->ev.type == SDL_MOUSEBUTTONDOWN ) || (ev->ev.type == SDL_MOUSEWHEEL);

	// -- KEYDOWN SECTION --

	std::vector<const char*> closestMapNames;
	size_t closestMapMatch = 0;

	for (const SHotkeyMapping& hotkey : g_HotkeyMap[scancode])
	{
		// If a key has been pressed, and this event triggers on its release, skip it.
		// Similarly, if the key's been released and the event triggers on a keypress, skip it.
		if (hotkey.negated == typeKeyDown)
			continue;

		// Check for no unpermitted keys
		bool accept = true;
		for (const SKey& k : hotkey.requires)
		{
			accept = isNegated(k);
			if (!accept)
				break;
		}

		if (accept && !(consoleCapture && hotkey.name != "console.toggle"))
		{
			// Check if this is an equally precise or more precise match
			if (hotkey.requires.size() + 1 >= closestMapMatch)
			{
				// Check if more precise
				if (hotkey.requires.size() + 1 > closestMapMatch)
				{
					// Throw away the old less-precise matches
					closestMapNames.clear();
					closestMapMatch = hotkey.requires.size() + 1;
				}

				closestMapNames.push_back(hotkey.name.c_str());
			}
		}
	}

	for (size_t i = 0; i < closestMapNames.size(); ++i)
	{
		// Send a KeyPress event when a key is pressed initially and on mouseButton and mouseWheel events.
		if (ev->ev.type != SDL_KEYDOWN || ev->ev.key.repeat == 0)
		{
			SDL_Event_ hotkeyPressNotification;
			hotkeyPressNotification.ev.type = SDL_HOTKEYPRESS;
			hotkeyPressNotification.ev.user.data1 = const_cast<char*>(closestMapNames[i]);
			in_push_priority_event(&hotkeyPressNotification);
		}

		// Send a HotkeyDown event on every key, mouseButton and mouseWheel event.
		// For keys the event is repeated depending on hardware and OS configured interval.
		// On linux, modifier keys (shift, alt, ctrl) are not repeated, see https://github.com/SFML/SFML/issues/122.
		SDL_Event_ hotkeyDownNotification;
		hotkeyDownNotification.ev.type = SDL_HOTKEYDOWN;
		hotkeyDownNotification.ev.user.data1 = const_cast<char*>(closestMapNames[i]);
		in_push_priority_event(&hotkeyDownNotification);
	}

	// -- KEYUP SECTION --

	for (const SHotkeyMapping& hotkey : g_HotkeyMap[scancode])
	{
		// If it's a keydown event, won't cause HotKeyUps in anything that doesn't
		// use this key negated => skip them
		// If it's a keyup event, won't cause HotKeyUps in anything that does use
		// this key negated => skip them too.
		if (hotkey.negated != typeKeyDown)
			continue;

		// Check for no unpermitted keys
		bool accept = true;
		for (const SKey& k : hotkey.requires)
		{
			accept = isNegated(k);
			if (!accept)
				break;
		}

		if (accept)
		{
			SDL_Event_ hotkeyNotification;
			hotkeyNotification.ev.type = SDL_HOTKEYUP;
			hotkeyNotification.ev.user.data1 = const_cast<char*>(hotkey.name.c_str());
			in_push_priority_event(&hotkeyNotification);
		}
	}

	return IN_PASS;
}

bool HotkeyIsPressed(const CStr& keyname)
{
	return g_HotkeyStatus[keyname];
}
