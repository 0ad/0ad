/*
A GUI ScrollBar
by Gustav Larsson
gee@pyro.nu

--Overview--

	A GUI Scrollbar, this class doesn't present all functionality
	to the scrollbar, it just controls the drawing and a wrapper
	for interaction with it.

--Usage--

	Used in everywhere scrollbars are needed, like in a combobox for instance.

--More info--

	Check GUI.h

*/

#ifndef CGUIScrollBarVertical_H
#define CGUIScrollBarVertical_H

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
 * Vertical implementation of IGUIScrollBar
 *
 * @see IGUIScrollBar
 */
 class CGUIScrollBarVertical : public IGUIScrollBar
{
public:
	CGUIScrollBarVertical() {}
	virtual ~CGUIScrollBarVertical() {}

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
	 * @param Message SGUIMessage
	 * @return true if messages handled the scroll-bar some. False if
	 *		   the message should be processed by the object.
	 */
	virtual bool HandleMessage(const SGUIMessage &Message);

	/**
	 * Set m_Pos with mouse_x/y input, i.e. when draggin.
	 */
	virtual void SetPosFromMousePos(int _x, int _y);

	/**
	 * @see IGUIScrollBar#HoveringButtonMinus
	 */
	virtual bool HoveringButtonMinus(int m_x, int m_y);

	/**
	 * @see IGUIScrollBar#HoveringButtonPlus
	 */
	virtual bool HoveringButtonPlus(int m_x, int m_y);

	/**
	 * Set Right Aligned
	 * @param align Alignment
	 */
	void SetRightAligned(const bool &align) { m_RightAligned = align; }

protected:
	/**
	 * Get the rectangle of the actual BAR.
	 * @return Rectangle, CRect
	 */
	virtual CRect GetBarRect() const;

	/**
	 * Should the scroll bar proceed to the left or to the right of the m_X value.
	 * Notice, this has nothing to do with where the owner places it.
	 */
	bool m_RightAligned;
};

#endif
