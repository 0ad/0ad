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

#ifndef IGUIScrollBar_H
#define IGUIScrollBar_H

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
 * The GUI Scroll-bar style. Tells us how scroll-bars look and feel.
 *
 * A scroll-bar style can choose whether to support horizontal, vertical
 * or both.
 *
 * @see IGUIScrollBar
 */
struct SGUIScrollBarStyle
{
	//--------------------------------------------------------
	/** @name General Settings */
	//--------------------------------------------------------
	//@{

	/**
	 * Width of bar, also both sides of the edge buttons.
	 */
	int m_Width;

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
	int m_MinimumBarSize;

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


/**
 * @author Gustav Larsson
 *
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
	IGUIScrollBar();
	virtual ~IGUIScrollBar();

public:
	/**
	 * Draw the scroll-bar
	 */
	virtual void Draw()=0;

	/**
     * If an object that contains a scrollbar has got messages, send
	 * them to the scroll-bar and it will see if the message regarded
	 * itself.
	 *
	 * @param Message SGUIMessage
	 * @return true if messages handled the scroll-bar some. False if
	 *		   the message should be processed by the object.
	 */
	virtual bool HandleMessage(const SGUIMessage &Message)=0;

	/**
	 * Set m_Pos with mouse_x/y input, i.e. when draggin.
	 */
	virtual void SetPosFromMousePos(int _x, int _y)=0;

	/**
	 * Hovering the scroll minus button
	 *
	 * @param m_x mouse x
	 * @param m_y mouse y
	 * @return True if mouse positions are hovering the button
	 */
	virtual bool HoveringButtonMinus(int m_x, int m_y) { return false; }

	/**
	 * Hovering the scroll plus button
	 *
	 * @param m_x mouse x
	 * @param m_y mouse y
	 * @return True if mouse positions are hovering the button
	 */
	virtual bool HoveringButtonPlus(int m_x, int m_y) { return false; }

	/**
	 * Scroll towards 1.0 one step
	 */
	virtual void ScrollPlus() { m_Pos += 0.1f; UpdatePosBoundaries(); }

	/**
	 * Scroll towards 0.0 one step
	 */
	virtual void ScrollMinus() { m_Pos -= 0.1f; UpdatePosBoundaries(); }

	/**
	 * Set host object, must be done almost at creation of scroll bar.
	 * @param pOwner Pointer to host object.
	 */
	void SetHostObject(IGUIScrollBarOwner * pOwner) { m_pHostObject = pOwner; }

	/**
	 * Set Width
	 * @param width Width
	 */
	void SetWidth(const int16 &width) { m_Width = width; }
	
	/**
	 * Set X Position
	 * @param x Position in this axis
	 */
	void SetX(const int &x) {m_X = x; }

	/**
	 * Set Y Position
	 * @param y Position in this axis
	 */
	void SetY(const int &y) {m_Y = y; }

	/**
	 * Set Z Position
	 * @param z Position in this axis
	 */
	void SetZ(const float &z) {m_Z = z; }

	/**
	 * Set Length of scroll bar
	 * @param length Length
	 */
	void SetLength(const float &length) {m_Length = length; }

	/**
	 * Set bar pressed
	 * @param pressed True if bar is pressed
	 */
	void SetBarPressed(const bool &b) { m_BarPressed = b; }

	/**
	 * Set Scroll bar style string
	 * @param style String with scroll bar style reference name
	 */
	void SetScrollBarStyle(const CStr &style) { m_ScrollBarStyle = style; }

protected:
	/**
	 * Get the rectangle of the actual BAR. not the whole scroll-bar.
	 */
	virtual CRect GetBarRect() const = 0;

	/**
	 * Get style used by the scrollbar
	 * @return Scroll bar style struct.
	 */
	const SGUIScrollBarStyle & GetStyle() const;

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
	int16 m_Width;	

	/**
	 * Absolute X Position
	 */
	int m_X;

	/**
	 * Absolute Y Position
	 */
	int m_Y;

	/**
	 * Absolute Z Position
	 */
	float m_Z;

	/**
	 * Total length of scrollbar, including edge buttons.
	 */
	int m_Length;

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
	 * Mouse position when bar was pressed
	 */
	int m_BarPressedAtX, m_BarPressedAtY;

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
	 * Position of scroll bar, 0 means scrolled all the way to one side... 1 means
	 * scrolled all the way to the other side.
	 */
	float m_Pos;

	/**
	 * Position from 0.f to 1.f it had when the bar was pressed.
	 */
	float m_PosWhenPressed;

	//@}
};

#endif
