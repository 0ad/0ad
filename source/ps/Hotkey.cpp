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

namespace {
	std::unordered_map<std::string, bool> g_HotkeyStatus;

	struct PressedHotkey
	{
		PressedHotkey(const SHotkeyMapping* m, bool t) : mapping(m), retriggered(t) {};
		// NB: this points to one of g_HotkeyMap's mappings. It works because that std::unordered_map is stable once constructed.
		const SHotkeyMapping* mapping;
		// Whether the hotkey was triggered by a key release (silences "press" and "up" events).
		bool retriggered;
	};

	struct ReleasedHotkey
	{
		ReleasedHotkey(const char* n, bool t) : name(n), wasRetriggered(t) {};
		const char* name;
		bool wasRetriggered;
	};

	// 'In-flight' state used because the hotkey triggering process is split in two phase.
	// These hotkeys may still be stopped if the event responsible for triggering them is handled
	// before it can be used to generate the hotkeys.
	std::vector<PressedHotkey> newPressedHotkeys;
	// Stores the 'specificity' of the newly pressed hotkeys.
	size_t closestMapMatch = 0;
	// This is merely used to ensure consistency in EventWillFireHotkey.
	const SDL_Event_* currentEvent;

	// List of currently pressed hotkeys. This is used to quickly reset hotkeys.
	// This is an unsorted vector because there will generally be very few elements,
	// so it's presumably faster than std::set.
	std::vector<PressedHotkey> pressedHotkeys;

	// List of active keys relevant for hotkeys.
	std::vector<SDL_Scancode_> activeScancodes;
}

static_assert(std::is_integral<std::underlying_type<SDL_Scancode>::type>::value, "SDL_Scancode is not an integral enum.");
static_assert(SDL_USEREVENT_ == SDL_USEREVENT, "SDL_USEREVENT_ is not the same type as the real SDL_USEREVENT");
static_assert(UNUSED_HOTKEY_CODE == SDL_SCANCODE_UNKNOWN);

// Look up each key binding in the config file and set the mappings for
// all key combinations that trigger it.
static void LoadConfigBindings(CConfigDB& configDB)
{
	for (const std::pair<const CStr, CConfigValueSet>& configPair : configDB.GetValuesWithPrefix(CFG_COMMAND, "hotkey."))
	{
		std::string hotkeyName = configPair.first.substr(7); // strip the "hotkey." prefix

		// "unused" is kept or the A23->24 migration, this can likely be removed in A25.
		if (configPair.second.empty() || (configPair.second.size() == 1 && configPair.second.front() == "unused"))
		{
			// Unused hotkeys must still be registered in the map to appear in the hotkey editor.
			SHotkeyMapping unusedCode;
			unusedCode.name = hotkeyName;
			unusedCode.primary = SKey{ UNUSED_HOTKEY_CODE };
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

				SKey key = { scancode };
				keyCombination.push_back(key);
			}

			std::vector<SKey>::iterator itKey, itKey2;
			for (itKey = keyCombination.begin(); itKey != keyCombination.end(); ++itKey)
			{
				SHotkeyMapping bindCode;

				bindCode.name = hotkeyName;
				bindCode.primary = SKey{ itKey->code };

				for (itKey2 = keyCombination.begin(); itKey2 != keyCombination.end(); ++itKey2)
					if (itKey != itKey2) // Push any auxiliary keys
						bindCode.requires.push_back(*itKey2);

				g_HotkeyMap[itKey->code].push_back(bindCode);
			}
		}
	}
}

void LoadHotkeys(CConfigDB& configDB)
{
	pressedHotkeys.clear();
	LoadConfigBindings(configDB);
}

void UnloadHotkeys()
{
	pressedHotkeys.clear();
	g_HotkeyMap.clear();
	g_HotkeyStatus.clear();
}

bool isPressed(const SKey& key)
{
	// Normal keycodes are below EXTRA_KEYS_BASE
	if ((int)key.code < EXTRA_KEYS_BASE)
		return g_scancodes[key.code];
	// Mouse 'keycodes' are after the modifier keys
	else if ((int)key.code < MOUSE_LAST && (int)key.code > MOUSE_BASE)
		return g_mouse_buttons[key.code - MOUSE_BASE];
	// Modifier keycodes are between the normal keys and the mouse 'keys'
	else if ((int)key.code < UNIFIED_LAST && (int)key.code > SDL_NUM_SCANCODES)
		return unified[key.code - UNIFIED_SHIFT];
	// This codepath shouldn't be taken, but not having it triggers warnings.
	else
		return false;
}

InReaction HotkeyStateChange(const SDL_Event_* ev)
{
	if (ev->ev.type == SDL_HOTKEYPRESS || ev->ev.type == SDL_HOTKEYPRESS_SILENT)
		g_HotkeyStatus[static_cast<const char*>(ev->ev.user.data1)] = true;
	else if (ev->ev.type == SDL_HOTKEYUP || ev->ev.type == SDL_HOTKEYUP_SILENT)
		g_HotkeyStatus[static_cast<const char*>(ev->ev.user.data1)] = false;
	return IN_PASS;
}

InReaction HotkeyInputPrepHandler(const SDL_Event_* ev)
{
	int scancode = SDL_SCANCODE_UNKNOWN;

	// Restore default state.
	newPressedHotkeys.clear();
	currentEvent = nullptr;

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
		return HotkeyInputPrepHandler(&phantom);
	}
	else if (scancode == SDL_SCANCODE_LCTRL || scancode == SDL_SCANCODE_RCTRL)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_CTRL);
		unified[1] = (phantom.ev.type == SDL_KEYDOWN);
		return HotkeyInputPrepHandler(&phantom);
	}
	else if (scancode == SDL_SCANCODE_LALT || scancode == SDL_SCANCODE_RALT)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_ALT);
		unified[2] = (phantom.ev.type == SDL_KEYDOWN);
		return HotkeyInputPrepHandler(&phantom);
	}
	else if (scancode == SDL_SCANCODE_LGUI || scancode == SDL_SCANCODE_RGUI)
	{
		phantom.ev.key.keysym.scancode = static_cast<SDL_Scancode>(UNIFIED_SUPER);
		unified[3] = (phantom.ev.type == SDL_KEYDOWN);
		return HotkeyInputPrepHandler(&phantom);
	}

	// Check whether we have any hotkeys registered that include this scancode.
	if (g_HotkeyMap.find(scancode) == g_HotkeyMap.end())
		return IN_PASS;

	currentEvent = ev;

	/**
	 * Hotkey behaviour spec (see also tests):
	 *  - If both 'F' and 'Ctrl+F' are hotkeys, and Ctrl & F keys are down, then the more specific one only is fired ('Ctrl+F' here).
	 *  - If 'Ctrl+F' and 'Ctrl+A' are both hotkeys, both may fire simulatenously (respectively without Ctrl).
	 *    - However, per the first point, 'Ctrl+Shift+F' would fire alone in that situation.
	 *  - "Press" is sent once, when the hotkey is initially triggered.
	 *  - "Up" is sent once, when the hotkey is released or superseded by a more specific hotkey.
	 *  - "Down" is sent repeatedly, and is also sent alongside the inital "Press".
	 *    - As a special case (see below), "Down" is not sent alongside "PressSilent".
	 *  - If 'Ctrl+F' is active, and 'Ctrl' is released, 'F' must become active again.
	 *    - However, the "Press" event is _not_ fired. Instead, "PressSilent" is.
	 *    - Likewise, once 'F' is released, the "Up" event will be a "UpSilent".
	 *      (the reason is that it is unexpected to trigger a press on key release).
	 *  - Hotkeys are allowed to fire with extra keys (e.g. Ctrl+F+A still triggers 'Ctrl+F').
	 *  - If 'F' and 'Ctrl+F' trigger the same hotkey, adding 'Ctrl' _and_ releasing 'Ctrl' will trigger new 'Press' events.
	 *    The "Up" event is only sent when both Ctrl & F are released.
	 *    - This is somewhat unexpected/buggy, but it makes the implementation easier and is easily avoidable for players.
	 *  - Wheel scrolling is 'instantaneous' behaviour and is essentially entirely separate from the above.
	 *    - It won't untrigger other hotkeys, and fires/releases on the same 'key event'.
	 * Note that mouse buttons/wheel inputs can fire hotkeys, in combinations with keys.
	 * ...Yes, this is all surprisingly complex.
	 */

	bool isReleasedKey = ev->ev.type == SDL_KEYUP || ev->ev.type == SDL_MOUSEBUTTONUP;
	// Wheel events are pressed & released in the same go.
	bool isInstantaneous = ev->ev.type == SDL_MOUSEWHEEL;

	if (!isInstantaneous)
	{
		std::vector<SDL_Scancode_>::iterator it = std::find(activeScancodes.begin(), activeScancodes.end(), scancode);
		// This prevents duplicates, assuming we might end up in a weird state - feels safer with input.
		if (isReleasedKey && it != activeScancodes.end())
			activeScancodes.erase(it);
		else if (!isReleasedKey && it == activeScancodes.end())
			activeScancodes.emplace_back(scancode);
	}

	std::set<SDL_Scancode_> triggers;
	if (!isReleasedKey || isInstantaneous)
		triggers.insert(scancode);
	else
		// If the key is released, we need to check all less precise hotkeys again, to see if we should retrigger some.
		for (SDL_Scancode_ code : activeScancodes)
			triggers.insert(code);

	// Now check if we need to trigger new hotkeys / retrigger hotkeys.
	// We'll need the match-level and the keys in play to release currently pressed hotkeys.
	closestMapMatch = 0;
	for (SDL_Scancode_ code : triggers)
		for (const SHotkeyMapping& hotkey : g_HotkeyMap[code])
		{
			// Ensure no duplications in the new list.
			if (std::find_if(newPressedHotkeys.begin(), newPressedHotkeys.end(),
							 [&hotkey](const PressedHotkey& v){ return v.mapping->name == hotkey.name; }) != newPressedHotkeys.end())
				continue;

			bool accept = true;
			for (const SKey& k : hotkey.requires)
			{
				accept = isPressed(k);
				if (!accept)
					break;
			}
			if (!accept)
				continue;

			// Check if this is an equally precise or more precise match
			if (hotkey.requires.size() + 1 >= closestMapMatch)
			{
				// Check if more precise
				if (hotkey.requires.size() + 1 > closestMapMatch)
				{
					// Throw away the old less-precise matches
					newPressedHotkeys.clear();
					closestMapMatch = hotkey.requires.size() + 1;
				}
				newPressedHotkeys.emplace_back(&hotkey, isReleasedKey);
			}
		}

	return IN_PASS;
}

InReaction HotkeyInputActualHandler(const SDL_Event_* ev)
{
	if (!currentEvent)
		return IN_PASS;

	bool isInstantaneous = ev->ev.type == SDL_MOUSEWHEEL;

	// TODO: it's probably possible to break hotkeys somewhat if the "Up" event that would release a hotkey is handled
	// by a priori handler - it might be safer to do that in the 'Prep' phase.
	std::vector<ReleasedHotkey> releasedHotkeys;

	// For instantaneous events, we don't update the pressedHotkeys (i.e. currently active hotkeys),
	// we just fire/release the triggered hotkeys transiently.
	// Therefore, skip the whole 'check pressedHotkeys & swap with newPressedHotkeys' logic.
	if (!isInstantaneous)
	{
		for (PressedHotkey& hotkey : pressedHotkeys)
		{
			bool addingAnew = std::find_if(newPressedHotkeys.begin(), newPressedHotkeys.end(),
										   [&hotkey](const PressedHotkey& v){ return v.mapping->name == hotkey.mapping->name; }) != newPressedHotkeys.end();

			// Update the triggered status to match our current state.
			if (addingAnew)
				std::find_if(newPressedHotkeys.begin(), newPressedHotkeys.end(),
							 [&hotkey](const PressedHotkey& v){ return v.mapping->name == hotkey.mapping->name; })->retriggered = hotkey.retriggered;
			// If the already-pressed hotkey has a lower specificity than the new hotkey(s), de-activate it.
			else if (hotkey.mapping->requires.size() + 1 < closestMapMatch)
			{
				releasedHotkeys.emplace_back(hotkey.mapping->name.c_str(), hotkey.retriggered);
				continue;
			}

			// Check that the hotkey still matches all active keys.
			bool accept = isPressed(hotkey.mapping->primary);
			if (accept)
				for (const SKey& k : hotkey.mapping->requires)
				{
					accept = isPressed(k);
					if (!accept)
						break;
				}
			if (!accept && !addingAnew)
				releasedHotkeys.emplace_back(hotkey.mapping->name.c_str(), hotkey.retriggered);
			else if (accept)
			{
				// If this hotkey has higher specificity than the new hotkeys we wanted to trigger/retrigger,
				// then discard this new addition(s). This works because at any given time, all hotkeys
				// active must have the same specificity.
				if (hotkey.mapping->requires.size() + 1 > closestMapMatch)
				{
					closestMapMatch = hotkey.mapping->requires.size() + 1;
					newPressedHotkeys.clear();
					newPressedHotkeys.emplace_back(hotkey.mapping, hotkey.retriggered);
				}
				else if (!addingAnew)
					newPressedHotkeys.emplace_back(hotkey.mapping, hotkey.retriggered);
			}
		}

		pressedHotkeys.swap(newPressedHotkeys);
	}

	for (const PressedHotkey& hotkey : isInstantaneous ? newPressedHotkeys : pressedHotkeys)
	{
		// Send a KeyPress event when a hotkey is pressed initially and on mouseButton and mouseWheel events.
		if (ev->ev.type != SDL_KEYDOWN || ev->ev.key.repeat == 0)
		{
			SDL_Event_ hotkeyPressNotification;
			hotkeyPressNotification.ev.type = hotkey.retriggered ? SDL_HOTKEYPRESS_SILENT : SDL_HOTKEYPRESS;
			hotkeyPressNotification.ev.user.data1 = const_cast<char*>(hotkey.mapping->name.c_str());
			in_push_priority_event(&hotkeyPressNotification);
		}

		// Send a HotkeyDown event on every key, mouseButton and mouseWheel event.
		// The exception is on the first retriggering: hotkeys may fire transiently
		// while a user lifts fingers off multi-key hotkeys, and listeners to "hotkeydown"
		// generally don't expect that to trigger then.
		// (It might be better to check for HotkeyIsPressed, however).
		// For keys the event is repeated depending on hardware and OS configured interval.
		// On linux, modifier keys (shift, alt, ctrl) are not repeated, see https://github.com/SFML/SFML/issues/122.
		if (ev->ev.key.repeat == 0 && hotkey.retriggered)
			continue;
		SDL_Event_ hotkeyDownNotification;
		hotkeyDownNotification.ev.type = SDL_HOTKEYDOWN;
		hotkeyDownNotification.ev.user.data1 = const_cast<char*>(hotkey.mapping->name.c_str());
		in_push_priority_event(&hotkeyDownNotification);
	}

	// Release instantaneous events (e.g. mouse wheel) right away.
	if (isInstantaneous)
		for (const PressedHotkey& hotkey : newPressedHotkeys)
			releasedHotkeys.emplace_back(hotkey.mapping->name.c_str(), false);

	for (const ReleasedHotkey& hotkey : releasedHotkeys)
	{
		SDL_Event_ hotkeyNotification;
		hotkeyNotification.ev.type = hotkey.wasRetriggered ? SDL_HOTKEYUP_SILENT : SDL_HOTKEYUP;
		hotkeyNotification.ev.user.data1 = const_cast<char*>(hotkey.name);
		in_push_priority_event(&hotkeyNotification);
	}

	return IN_PASS;
}

bool EventWillFireHotkey(const SDL_Event_* ev, const CStr& keyname)
{
	// Sanity check of sort. This parameter mostly exists because it looks right from the caller's perspective.
	if (ev != currentEvent || !currentEvent)
		return false;

	return std::find_if(newPressedHotkeys.begin(), newPressedHotkeys.end(),
		[&keyname](const PressedHotkey& v){ return v.mapping->name == keyname; }) != newPressedHotkeys.end();
}

bool HotkeyIsPressed(const CStr& keyname)
{
	return g_HotkeyStatus[keyname];
}
