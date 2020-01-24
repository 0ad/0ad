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

/*
	Interface class that enhance the IGUIObject with
	 buttony behavior (click and release to click a button),
	 and the GUI message GUIM_PRESSED.
	When creating a class with extended settings and
	 buttony behavior, just do a multiple inheritance.
*/

#ifndef INCLUDED_IGUIBUTTONBEHAVIOR
#define INCLUDED_IGUIBUTTONBEHAVIOR

#include "gui/ObjectBases/IGUIObject.h"

class CGUISpriteInstance;

/**
 * Appends button behaviours to the IGUIObject.
 * Can be used with multiple inheritance alongside
 * IGUISettingsObject and such.
 */
class IGUIButtonBehavior
{
	NONCOPYABLE(IGUIButtonBehavior);

public:
	IGUIButtonBehavior(IGUIObject& pObject);
	virtual ~IGUIButtonBehavior();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * This is a function that lets a button being drawn,
	 * it regards if it's over, disabled, pressed and such.
	 *
	 * @param sprite Sprite drawn when not pressed, hovered or disabled
	 * @param sprite_over Sprite drawn when m_MouseHovering is true
	 * @param sprite_pressed Sprite drawn when m_Pressed is true
	 * @param sprite_disabled Sprite drawn when "enabled" is false
	 */
	const CGUISpriteInstance& GetButtonSprite(const CGUISpriteInstance& sprite, const CGUISpriteInstance& sprite_over, const CGUISpriteInstance& sprite_pressed, const CGUISpriteInstance& sprite_disabled) const;

protected:
	static const CStr EventNamePress;
	static const CStr EventNamePressRight;
	static const CStr EventNameDoublePress;
	static const CStr EventNameDoublePressRight;
	static const CStr EventNameRelease;
	static const CStr EventNameReleaseRight;

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates();

	/**
	 * Everybody knows how a button works, you don't simply press it,
	 * you have to first press the button, and then release it...
	 * in between those two steps you can actually leave the button
	 * area, as long as you release it within the button area... Anyway
	 * this lets us know we are done with step one (clicking).
	 */
	bool m_Pressed;
	bool m_PressedRight;

	// Settings
	CStrW m_SoundDisabled;
	CStrW m_SoundEnter;
	CStrW m_SoundLeave;
	CStrW m_SoundPressed;
	CStrW m_SoundReleased;

private:
	/**
	 * Reference to the IGUIObject.
	 * Private, because we don't want to inherit it in multiple classes.
	 */
	IGUIObject& m_pObject;
};

#endif // INCLUDED_IGUIBUTTONBEHAVIOR
