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

#ifndef INCLUDED_SGUIMESSAGE
#define INCLUDED_SGUIMESSAGE

#include "ps/CStr.h"

/**
 * Message types.
 * @see SGUIMessage
 */
enum EGUIMessageType
{
	GUIM_MOUSE_OVER,
	GUIM_MOUSE_ENTER,
	GUIM_MOUSE_LEAVE,
	GUIM_MOUSE_PRESS_LEFT,
	GUIM_MOUSE_PRESS_LEFT_ITEM,
	GUIM_MOUSE_PRESS_RIGHT,
	GUIM_MOUSE_DOWN_LEFT,
	GUIM_MOUSE_DOWN_RIGHT,
	GUIM_MOUSE_DBLCLICK_LEFT,
	GUIM_MOUSE_DBLCLICK_LEFT_ITEM, // Triggered when doubleclicking on a list item
	GUIM_MOUSE_DBLCLICK_RIGHT,
	GUIM_MOUSE_RELEASE_LEFT,
	GUIM_MOUSE_RELEASE_RIGHT,
	GUIM_MOUSE_WHEEL_UP,
	GUIM_MOUSE_WHEEL_DOWN,
	GUIM_SETTINGS_UPDATED,	// SGUIMessage.m_Value = name of setting
	GUIM_PRESSED,
	GUIM_KEYDOWN,
	GUIM_RELEASED,
	GUIM_DOUBLE_PRESSED,
	GUIM_MOUSE_MOTION,
	GUIM_LOAD, // Called after all objects were added to the GUI.
	GUIM_GOT_FOCUS,
	GUIM_LOST_FOCUS,
	GUIM_PRESSED_MOUSE_RIGHT,
	GUIM_DOUBLE_PRESSED_MOUSE_RIGHT,
	GUIM_PRESSED_MOUSE_RELEASE,
	GUIM_PRESSED_MOUSE_RELEASE_RIGHT,
	GUIM_TAB,				// Used by CInput
	GUIM_TEXTEDIT
};

/**
 * Message send to IGUIObject::HandleMessage() in order
 * to give life to Objects manually with
 * a derived HandleMessage().
 */
struct SGUIMessage
{
	// This should be passed as a const reference or pointer.
	NONCOPYABLE(SGUIMessage);

	SGUIMessage(EGUIMessageType _type) : type(_type), skipped(false) {}
	SGUIMessage(EGUIMessageType _type, const CStr& _value) : type(_type), value(_value), skipped(false) {}

	/**
	 * This method can be used to allow other event handlers to process this GUI event,
	 * by default an event is not skipped (only the first handler will process it).
	 *
	 * @param skip true to allow further event handling, false to prevent it
	 */
	void Skip(bool skip = true) { skipped = skip; }

	/**
	 * Describes what the message regards
	 */
	EGUIMessageType type;

	/**
	 * Optional data
	 */
	CStr value;

	/**
	 * Flag that specifies if object skipped handling the event
	 */
	bool skipped;
};

#endif // INCLUDED_SGUIMESSAGE
