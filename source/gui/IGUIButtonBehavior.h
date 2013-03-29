/* Copyright (C) 2012 Wildfire Games.
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
GUI Object Base - Button Behavior

--Overview--

	Interface class that enhance the IGUIObject with
	 buttony behavior (click and release to click a button),
	 and the GUI message GUIM_PRESSED.
	When creating a class with extended settings and
	 buttony behavior, just do a multiple inheritance.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_IGUIBUTTONBEHAVIOR
#define INCLUDED_IGUIBUTTONBEHAVIOR

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

class CGUISpriteInstance;

//--------------------------------------------------------
//  Macros
//--------------------------------------------------------

//--------------------------------------------------------
//  Types
//--------------------------------------------------------

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * Appends button behaviours to the IGUIObject.
 * Can be used with multiple inheritance alongside
 * IGUISettingsObject and such.
 *
 * @see IGUIObject
 */
class IGUIButtonBehavior : virtual public IGUIObject
{
public:
	IGUIButtonBehavior();
	virtual ~IGUIButtonBehavior();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage &Message);

	/**
	 * This is a function that lets a button being drawn,
	 * it regards if it's over, disabled, pressed and such.
	 * You input sprite names and area and it'll output
	 * it accordingly.
	 *
	 * This class is meant to be used manually in Draw()
	 *
	 * @param rect Rectangle in which the sprite should be drawn
	 * @param z Z-value
	 * @param sprite Sprite drawn when not pressed, hovered or disabled
	 * @param sprite_over Sprite drawn when m_MouseHovering is true
	 * @param sprite_pressed Sprite drawn when m_Pressed is true
	 * @param sprite_disabled Sprite drawn when "enabled" is false
	 * @param cell_id Identifies the icon to be used (if the sprite contains
	 *                cell-using images)
	 */
	void DrawButton(const CRect &rect,
					const float &z,
					CGUISpriteInstance& sprite,
					CGUISpriteInstance& sprite_over,
					CGUISpriteInstance& sprite_pressed,
					CGUISpriteInstance& sprite_disabled,
					int cell_id);

	/**
	 * Choosing which color of the following according to object enabled/hovered/pressed status:
	 *		textcolor_disabled	-- disabled
	 *		textcolor_pressed	-- pressed
	 *		textcolor_over		-- hovered
	 */
	CColor ChooseColor();


protected:
	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates()
	{
		// Notify the gui that we aren't hovered anymore
		UpdateMouseOver(NULL);
		m_Pressed = false;
		m_PressedRight = false;
	}

	/**
	 * Everybody knows how a button works, you don't simply press it,
	 * you have to first press the button, and then release it...
	 * in between those two steps you can actually leave the button
	 * area, as long as you release it within the button area... Anyway
	 * this lets us know we are done with step one (clicking).
	 */
	bool							m_Pressed;
	bool							m_PressedRight;
};

#endif
