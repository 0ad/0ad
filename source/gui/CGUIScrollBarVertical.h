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

#ifndef INCLUDED_CGUISCROLLBARVERTICAL
#define INCLUDED_CGUISCROLLBARVERTICAL

#include "IGUIScrollBar.h"
#include "GUI.h"

/**
 * Vertical implementation of IGUIScrollBar
 *
 * @see IGUIScrollBar
 */
class CGUIScrollBarVertical : public IGUIScrollBar
{
public:
	CGUIScrollBarVertical(CGUI* pGUI);
	virtual ~CGUIScrollBarVertical();

public:
	/**
	 * Draw the scroll-bar
	 */
	virtual void Draw();

	/**
     * If an object that contains a scrollbar has got messages, send
	 * them to the scroll-bar and it will see if the message regarded
	 * itself.
	 *
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Set m_Pos with g_mouse_x/y input, i.e. when dragging.
	 */
	virtual void SetPosFromMousePos(const CPos& mouse);

	/**
	 * @see IGUIScrollBar#HoveringButtonMinus
	 */
	virtual bool HoveringButtonMinus(const CPos& mouse);

	/**
	 * @see IGUIScrollBar#HoveringButtonPlus
	 */
	virtual bool HoveringButtonPlus(const CPos& mouse);

	/**
	 * Set Right Aligned
	 * @param align Alignment
	 */
	void SetRightAligned(const bool& align) { m_RightAligned = align; }

	/**
	 * Get the rectangle of the actual BAR.
	 * @return Rectangle, CRect
	 */
	virtual CRect GetBarRect() const;

	/**
	 * Get the rectangle of the outline of the scrollbar, every component of the
	 * scroll-bar should be inside this area.
	 * @return Rectangle, CRect
	 */
	virtual CRect GetOuterRect() const;

protected:
	/**
	 * Should the scroll bar proceed to the left or to the right of the m_X value.
	 * Notice, this has nothing to do with where the owner places it.
	 */
	bool m_RightAligned;
};

#endif // INCLUDED_CGUISCROLLBARVERTICAL
