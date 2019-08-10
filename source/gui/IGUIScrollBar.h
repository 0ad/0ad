/* Copyright (C) 2019 Wildfire Games.
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
A GUI ScrollBar

--Overview--

	A GUI Scrollbar, this class doesn't present all functionality
	to the scrollbar, it just controls the drawing and a wrapper
	for interaction with it.

--Usage--

	Used in everywhere scrollbars are needed, like in a combobox for instance.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_IGUISCROLLBAR
#define INCLUDED_IGUISCROLLBAR

#include "GUI.h"

/**
 * The GUI Scroll-bar style. Tells us how scroll-bars look and feel.
 *
 * A scroll-bar style can choose whether to support horizontal, vertical
 * or both.
 *
 * @see IGUIScrollBar
 */
struct SGUIScrollBarStyle
{
	// CGUISpriteInstance makes this NONCOPYABLE implicitly, make it explicit
	NONCOPYABLE(SGUIScrollBarStyle);
	MOVABLE(SGUIScrollBarStyle);
	SGUIScrollBarStyle() = default;

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

	/**
	 * Sometimes you would like your scroll bar to have a fixed maximum size
	 * so that the texture does not get too stretched, you can set a maximum
	 * in pixels.
	 */
	float m_MaximumBarSize;

	/**
	 * True if you want edge buttons, i.e. buttons that can be pressed in order
	 * to scroll.
	 */
	bool m_UseEdgeButtons;

	//@}
	//--------------------------------------------------------
	/** @name Vertical Sprites */
	//--------------------------------------------------------
	//@{

	CGUISpriteInstance m_SpriteButtonTop;
	CGUISpriteInstance m_SpriteButtonTopPressed;
	CGUISpriteInstance m_SpriteButtonTopDisabled;
	CGUISpriteInstance m_SpriteButtonTopOver;

	CGUISpriteInstance m_SpriteButtonBottom;
	CGUISpriteInstance m_SpriteButtonBottomPressed;
	CGUISpriteInstance m_SpriteButtonBottomDisabled;
	CGUISpriteInstance m_SpriteButtonBottomOver;

	CGUISpriteInstance m_SpriteBarVertical;
	CGUISpriteInstance m_SpriteBarVerticalOver;
	CGUISpriteInstance m_SpriteBarVerticalPressed;

	CGUISpriteInstance m_SpriteBackVertical;

	//@}
	//--------------------------------------------------------
	/** @name Horizontal Sprites */
	//--------------------------------------------------------
	//@{

	CGUISpriteInstance m_SpriteButtonLeft;
	CGUISpriteInstance m_SpriteButtonLeftPressed;
	CGUISpriteInstance m_SpriteButtonLeftDisabled;

	CGUISpriteInstance m_SpriteButtonRight;
	CGUISpriteInstance m_SpriteButtonRightPressed;
	CGUISpriteInstance m_SpriteButtonRightDisabled;

	CGUISpriteInstance m_SpriteBackHorizontal;
	CGUISpriteInstance m_SpriteBarHorizontal;

	//@}
};


/**
 * The GUI Scroll-bar, used everywhere there is a scroll-bar in the game.
 *
 * To include a scroll-bar to an object, inherent the object from
 * IGUIScrollBarOwner and call AddScrollBar() to add the scroll-bars.
 *
 * It's also important that the scrollbar is located within the parent
 * object's mouse over area. Otherwise the input won't be sent to the
 * scroll-bar.
 *
 * The class does not provide all functionality to the scroll-bar, many
 * things the parent of the scroll-bar, must provide. Like a combo-box.
 */
class IGUIScrollBar
{
public:
	NONCOPYABLE(IGUIScrollBar);

	IGUIScrollBar(CGUI* pGUI);
	virtual ~IGUIScrollBar();

public:
	/**
	 * Draw the scroll-bar
	 */
	virtual void Draw() = 0;

	/**
     * If an object that contains a scrollbar has got messages, send
	 * them to the scroll-bar and it will see if the message regarded
	 * itself.
	 *
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message) = 0;

	/**
	 * Set m_Pos with g_mouse_x/y input, i.e. when draggin.
	 */
	virtual void SetPosFromMousePos(const CPos& mouse) = 0;

	/**
	 * Hovering the scroll minus button
	 *
	 * @param mouse current mouse position
	 * @return True if mouse positions are hovering the button
	 */
	virtual bool HoveringButtonMinus(const CPos& UNUSED(mouse)) { return false; }

	/**
	 * Hovering the scroll plus button
	 *
	 * @param mouse current mouse position
	 * @return True if mouse positions are hovering the button
	 */
	virtual bool HoveringButtonPlus(const CPos& UNUSED(mouse)) { return false; }

	/**
	 * Get scroll-position
	 */
	float GetPos() const { return m_Pos; }

	/**
	 * Set scroll-position by hand
	 */
	virtual void SetPos(float f) { m_Pos = f; UpdatePosBoundaries(); }

	/**
	 * Get the value of m_Pos that corresponds to the bottom of the scrollable region
	 */
	float GetMaxPos() const { return std::max(0.f, m_ScrollRange - m_ScrollSpace); }

	/**
	 * Scrollbars without height shouldn't be visible
	 */
	bool IsVisible() const { return GetMaxPos() != 0.f; }

	/**
	 * Increase scroll one step
	 */
	virtual void ScrollPlus() { m_Pos += 30.f; UpdatePosBoundaries(); }

	/**
	 * Decrease scroll one step
	 */
	virtual void ScrollMinus() { m_Pos -= 30.f; UpdatePosBoundaries(); }

	/**
	 * Increase scroll three steps
	 */
	virtual void ScrollPlusPlenty() { m_Pos += 90.f; UpdatePosBoundaries(); }

	/**
	 * Decrease scroll three steps
	 */
	virtual void ScrollMinusPlenty() { m_Pos -= 90.f; UpdatePosBoundaries(); }

	/**
	 * Set host object, must be done almost at creation of scroll bar.
	 * @param pOwner Pointer to host object.
	 */
	void SetHostObject(IGUIScrollBarOwner* pOwner) { m_pHostObject = pOwner; }

	/**
	 * Get GUI pointer
	 * @return CGUI pointer
	 */
	CGUI* GetGUI() const;

	/**
	 * Set Width
	 * @param width Width
	 */
	void SetWidth(float width) { m_Width = width; }

	/**
	 * Set X Position
	 * @param x Position in this axis
	 */
	void SetX(float x) { m_X = x; }

	/**
	 * Set Y Position
	 * @param y Position in this axis
	 */
	void SetY(float y) { m_Y = y; }

	/**
	 * Set Z Position
	 * @param z Position in this axis
	 */
	void SetZ(float z) { m_Z = z; }

	/**
	 * Set Length of scroll bar
	 * @param length Length
	 */
	void SetLength(float length) { m_Length = length; }

	/**
	 * Set content length
	 * @param range Maximum scrollable range
	 */
	void SetScrollRange(float range) { m_ScrollRange = std::max(range, 1.f); SetupBarSize(); UpdatePosBoundaries(); }

	/**
	 * Set space that is visible in the scrollable control.
	 * @param space Visible area in the scrollable control.
	 */
	void SetScrollSpace(float space) { m_ScrollSpace = space; SetupBarSize(); UpdatePosBoundaries(); }

	/**
	 * Set bar pressed
	 * @param b True if bar is pressed
	 */
	void SetBarPressed(bool b) { m_BarPressed = b; }

	/**
	 * Set Scroll bar style string
	 * @param style String with scroll bar style reference name
	 */
	void SetScrollBarStyle(const CStr& style) { m_ScrollBarStyle = style; }

	/**
	 * Get style used by the scrollbar
	 * @return Scroll bar style struct.
	 */
	const SGUIScrollBarStyle* GetStyle() const;

	/**
	 * Get the rectangle of the actual BAR. not the whole scroll-bar.
	 * @return Rectangle, CRect
	 */
	virtual CRect GetBarRect() const = 0;

	/**
	 * Get the rectangle of the outline of the scrollbar, every component of the
	 * scroll-bar should be inside this area.
	 * @return Rectangle, CRect
	 */
	virtual CRect GetOuterRect() const = 0;

protected:
	/**
	 * Sets up bar size
	 */
	void SetupBarSize();

	/**
	 * Call every time m_Pos has been updated.
	 */
	void UpdatePosBoundaries();

protected:
	//@}
	//--------------------------------------------------------
	/** @name Settings */
	//--------------------------------------------------------
	//@{

	/**
	 * Width of the scroll bar
	 */
	float m_Width;

	/**
	 * Absolute X Position
	 */
	float m_X;

	/**
	 * Absolute Y Position
	 */
	float m_Y;

	/**
	 * Absolute Z Position
	 */
	float m_Z;

	/**
	 * Total length of scrollbar, including edge buttons.
	 */
	float m_Length;

	/**
	 * Content that can be scrolled, in pixels
	 */
	float m_ScrollRange;

	/**
	 * Content that can be viewed at a time, in pixels
	 */
	float m_ScrollSpace;

	/**
	 * Use input from the scroll-wheel? True or false.
	 */
	float m_BarSize;

	/**
	 * Scroll bar style reference name
	 */
	CStr m_ScrollBarStyle;

	/**
	 * Pointer to scroll bar style used.
	 */
	SGUIScrollBarStyle *m_pStyle;

	/**
	 * Host object, prerequisite!
	 */
	IGUIScrollBarOwner *m_pHostObject;

	/**
	 * Reference to CGUI object, these cannot work stand-alone
	 */
	CGUI *m_pGUI;

	/**
	 * Mouse position when bar was pressed
	 */
	CPos m_BarPressedAtPos;

	//@}
	//--------------------------------------------------------
	/** @name States */
	//--------------------------------------------------------
	//@{

	/**
	 * If the bar is currently being pressed and dragged.
	 */
	bool m_BarPressed;

	/**
	 * Bar being hovered or not
	 */
	bool m_BarHovered;

	/**
	 * Scroll buttons hovered
	 */
	bool m_ButtonMinusHovered, m_ButtonPlusHovered;

	/**
	 * Scroll buttons pressed
	 */
	bool m_ButtonMinusPressed, m_ButtonPlusPressed;

	/**
	 * Position of scroll bar, 0 means scrolled all the way to one side.
	 * It is measured in pixels, it is up to the host to make it actually
	 * apply in pixels.
	 */
	float m_Pos;

	/**
	 * Position from 0.f to 1.f it had when the bar was pressed.
	 */
	float m_PosWhenPressed;

	//@}
};

#endif // INCLUDED_IGUISCROLLBAR
