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

/*
A GUI ScrollBar Style

--Overview--

	A GUI scroll-bar style tells scroll-bars how they should look,
	width, sprites used, etc.

--Usage--

	Declare them in XML files, and reference them when declaring objects.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CGUISCROLLBARSTYLE
#define INCLUDED_CGUISCROLLBARSTYLE

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * The GUI Scroll-bar style.
 *
 * A scroll-bar style can choose whether to support horizontal, vertical
 * or both.
 *
 * @see CGUIScrollBar
 */
struct CGUIScrollBarStyle
{
	//--------------------------------------------------------
	/** @name General Settings */
	//--------------------------------------------------------
	//@{

	/**
	 * Width of bar, also both sides of the edge buttons.
	 */
	float m_Width;

	/**
	 * Scrollable with the wheel.
	 */
	bool m_ScrollWheel;

	/**
	 * How much (in percent, 0.1f = 10%) to scroll each time
	 * the wheel is admitted, or the buttons are pressed.
	 */
	float m_ScrollSpeed;

	/**
	 * Whether or not the edge buttons should appear or not. Sometimes
	 * you actually don't want them, like perhaps in a combo box.
	 */
	bool m_ScrollButtons;

	/**
	 * Sometimes there is *a lot* to scroll, but to prevent the scroll "bar"
	 * from being almost invisible (or ugly), you can set a minimum in pixel
	 * size.
	 */
	float m_MinimumBarSize;

	//@}
	//--------------------------------------------------------
	/** @name Horizontal Sprites */
	//--------------------------------------------------------
	//@{

	CGUISpriteInstance m_SpriteButtonTop;
	CGUISpriteInstance m_SpriteButtonTopPressed;
	CGUISpriteInstance m_SpriteButtonTopDisabled;

	CGUISpriteInstance m_SpriteButtonBottom;
	CGUISpriteInstance m_SpriteButtonBottomPressed;
	CGUISpriteInstance m_SpriteButtonBottomDisabled;

	CGUISpriteInstance m_SpriteScrollBackHorizontal;
	CGUISpriteInstance m_SpriteScrollBarHorizontal;

	//@}
	//--------------------------------------------------------
	/** @name Vertical Sprites */
	//--------------------------------------------------------
	//@{

	CGUISpriteInstance m_SpriteButtonLeft;
	CGUISpriteInstance m_SpriteButtonLeftPressed;
	CGUISpriteInstance m_SpriteButtonLeftDisabled;

	CGUISpriteInstance m_SpriteButtonRight;
	CGUISpriteInstance m_SpriteButtonRightPressed;
	CGUISpriteInstance m_SpriteButtonRightDisabled;

	CGUISpriteInstance m_SpriteScrollBackVertical;
	CGUISpriteInstance m_SpriteScrollBarVertical;

	//@}
};

#endif
