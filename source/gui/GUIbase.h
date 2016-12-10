/* Copyright (C) 2016 Wildfire Games.
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

/*
GUI Core, stuff that the whole GUI uses

--Overview--

	Contains defines, includes, types etc that the whole
	 GUI should have included.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_GUIBASE
#define INCLUDED_GUIBASE

#include <map>
#include <vector>

#include "ps/CStr.h"
#include "ps/Errors.h"
// I would like to just forward declare CSize, but it doesn't
//  seem to be defined anywhere in the predefined header.
#include "ps/Shapes.h"

class IGUIObject;

// Object settings setups
// Setup an object's ConstructObject function
#define GUI_OBJECT(obj)													\
public:																	\
	static IGUIObject* ConstructObject() { return new obj(); }


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
	GUIM_RELEASED,
	GUIM_DOUBLE_PRESSED,
	GUIM_MOUSE_MOTION,
	GUIM_LOAD,				// Called when an object is added to the GUI.
	GUIM_GOT_FOCUS,
	GUIM_LOST_FOCUS,
	GUIM_PRESSED_MOUSE_RIGHT,
	GUIM_DOUBLE_PRESSED_MOUSE_RIGHT,
	GUIM_TAB				// Used by CInput
};

/**
 * Message send to IGUIObject::HandleMessage() in order
 * to give life to Objects manually with
 * a derived HandleMessage().
 */
struct SGUIMessage
{
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

/**
 * Recurse restrictions, when we recurse, if an object
 * is hidden for instance, you might want it to skip
 * the children also
 * Notice these are flags! and we don't really need one
 * for no restrictions, because then you'll just enter 0
 */
enum
{
	GUIRR_HIDDEN		= 0x00000001,
	GUIRR_DISABLED		= 0x00000010,
	GUIRR_GHOST			= 0x00000100
};

// Text alignments
enum EAlign { EAlign_Left, EAlign_Right, EAlign_Center };
enum EVAlign { EVAlign_Top, EVAlign_Bottom, EVAlign_Center };

// Typedefs
typedef	std::map<CStr, IGUIObject*> map_pObjects;
typedef std::vector<IGUIObject*> vector_pObjects;

// Icon, you create them in the XML file with root element <setup>
//  you use them in text owned by different objects... Such as CText.
struct SGUIIcon
{
	SGUIIcon() : m_CellID(0) {}

	// Sprite name of icon
	CStr m_SpriteName;

	// Size
	CSize m_Size;

	// Cell of texture to use; ignored unless the texture has specified cell-size
	int m_CellID;
};

/**
 * Client Area is a rectangle relative to a parent rectangle
 *
 * You can input the whole value of the Client Area by
 * string. Like used in the GUI.
 */
class CClientArea
{
public:
	CClientArea();
	CClientArea(const CStr& Value);
	CClientArea(const CRect& pixel, const CRect& percent);

	/// Pixel modifiers
	CRect pixel;

	/// Percent modifiers
	CRect percent;

	/**
	 * Get client area rectangle when the parent is given
	 */
	CRect GetClientArea(const CRect& parent) const;

	/**
	 * The ClientArea can be set from a string looking like:
	 *
	 * "0 0 100% 100%"
	 * "50%-10 50%-10 50%+10 50%+10"
	 *
	 * i.e. First percent modifier, then + or - and the pixel modifier.
	 * Although you can use just the percent or the pixel modifier. Notice
	 * though that the percent modifier must always be the first when
	 * both modifiers are inputted.
	 *
	 * @return true if success, false if failure. If false then the client area
	 *			will be unchanged.
	 */
	bool SetClientArea(const CStr& Value);

	bool operator==(const CClientArea& other) const
	{
		return pixel == other.pixel && percent == other.percent;
	}
};


ERROR_GROUP(GUI);

ERROR_TYPE(GUI, NullObjectProvided);
ERROR_TYPE(GUI, InvalidSetting);
ERROR_TYPE(GUI, OperationNeedsGUIObject);
ERROR_TYPE(GUI, NameAmbiguity);
ERROR_TYPE(GUI, ObjectNeedsName);

#endif // INCLUDED_GUIBASE
