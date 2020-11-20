/* Copyright (C) 2020 Wildfire Games.
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

#ifndef INCLUDED_CHOTKEYPICKER
#define INCLUDED_CHOTKEYPICKER

#include "gui/CGUI.h"
#include "lib/external_libraries/libsdl.h"
#include "ps/CStr.h"

#include <vector>

class ScriptInterface;

/**
 * When in focus, returns all currently pressed keys.
 * After a set time without changes, it will trigger a "combination" event.
 *
 * Used to create new hotkey combinations in-game. Mostly custom.
 * This object does not draw anything.
 *
 * NB: because of how input is handled, mouse clicks
 */
class CHotkeyPicker : public IGUIObject
{
	GUI_OBJECT(CHotkeyPicker)

	friend class ScriptInterface;
public:
	CHotkeyPicker(CGUI& pGUI);
	virtual ~CHotkeyPicker();

	// Do nothing.
	virtual void Draw() {};

	// Checks if the timer has passed and we need to fire a "combination" event.
	virtual void Tick();

	// React to blur/focus.
	virtual void HandleMessage(SGUIMessage& Message);

	// Pre-empt events: this is our sole purpose.
	virtual InReaction PreemptEvent(const SDL_Event_* ev);
protected:
	// Fire an event with m_KeysPressed as argument.
	void FireEvent(const CStr& event);

	// Time without changes until a "combination" event is sent.
	float m_TimeToCombination;
	// Time of the last registered key change.
	double m_LastKeyChange;

	// Keep track of which keys we are pressing, and precompute their name for JS code.
	struct Key
	{
		// The scancode is used for fast comparisons.
		SDL_Scancode code;
		// This is the name ultimately stored in the config file.
		CStr scancodeName;
	};
	std::vector<Key> m_KeysPressed;

	static const CStr EventNameCombination;
	static const CStr EventNameKeyChange;
};

#endif // INCLUDED_CHOTKEYPICKER
