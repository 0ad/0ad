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
	CGUIScrollBarVertical();
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
	 * @param Message SGUIMessage
	 * @return true if messages handled the scroll-bar some. False if
	 *		   the message should be processed by the object.
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Set m_Pos with g_mouse_x/y input, i.e. when draggin.
	 */
	virtual void SetPosFromMousePos(const CPos &mouse);

	/**
	 * @see IGUIScrollBar#HoveringButtonMinus
	 */
	virtual bool HoveringButtonMinus(const CPos &mouse);

	/**
	 * @see IGUIScrollBar#HoveringButtonPlus
	 */
	virtual bool HoveringButtonPlus(const CPos &mouse);

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
	 * Get the rectangle of the outline of the scrollbar, every component of the
	 * scroll-bar should be inside this area.
	 * @return Rectangle, CRect
	 */
	virtual CRect GetOuterRect() const;

	/**
	 * Should the scroll bar proceed to the left or to the right of the m_X value.
	 * Notice, this has nothing to do with where the owner places it.
	 */
	bool m_RightAligned;
};

#endif
