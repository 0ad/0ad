/*
A GUI ScrollBar Style
by Gustav Larsson
gee@pyro.nu

--Overview--

	A GUI scroll-bar style tells scroll-bars how they should look,
	width, sprites used, etc.

--Usage--

	Declare them in XML files, and reference them when declaring objects.

--More info--

	Check GUI.h

*/

#ifndef CGUIScrollBarStyle_H
#define CGUIScrollBarStyle_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

/**
 * @author Gustav Larsson
 *
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

	CStr m_SpriteButtonTop;
	CStr m_SpriteButtonTopPressed;
	CStr m_SpriteButtonTopDisabled;

	CStr m_SpriteButtonBottom;
	CStr m_SpriteButtonBottomPressed;
	CStr m_SpriteButtonBottomDisabled;

	CStr m_SpriteScrollBackHorizontal;
	CStr m_SpriteScrollBarHorizontal;

	//@}
	//--------------------------------------------------------
	/** @name Verical Sprites */
	//--------------------------------------------------------
	//@{

	CStr m_SpriteButtonLeft;
	CStr m_SpriteButtonLeftPressed;
	CStr m_SpriteButtonLeftDisabled;

	CStr m_SpriteButtonRight;
	CStr m_SpriteButtonRightPressed;
	CStr m_SpriteButtonRightDisabled;

	CStr m_SpriteScrollBackVertical;
	CStr m_SpriteScrollBarVertical;

	//@}
};

#endif
