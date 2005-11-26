/*
GUI Object - Drop Down (list)
by Gustav Larsson
gee@pyro.nu

--Overview--

	Works just like a list-box, but it hides
	all the elements that aren't selected. They
	can be brought up by pressing the control.

--More info--

	Check GUI.h

*/

#ifndef CDropDown_H
#define CDropDown_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include "CList.h"
//class CList;

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
 * @author Gustav Larsson
 *
 * Drop Down
 *
 * The control can be pressed, but we will not inherent
 * this behavior from IGUIButtonBehavior, because when
 * you press this control, the list with elements will
 * immediately appear, and not first after release
 * (which is the whole gist of the IGUIButtonBehavior).
 */
class CDropDown : public CList
{
	GUI_OBJECT(CDropDown)

public:
	CDropDown();
	virtual ~CDropDown();

//	virtual void ResetStates() { IGUIButtonBehavior::ResetStates(); }

	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Handle events manually to catch keyboard inputting.
	 */
	virtual InReaction ManuallyHandleEvent(const SDL_Event* ev);

	/**
	 * Draws the Button
	 */
	virtual void Draw();

	// This is one of the few classes we actually need to redefine this function
	//  this is because the size of the control changes whether it is open
	//  or closed.
	virtual bool MouseOver();

	virtual float GetBufferedZ() const;

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();
	
	// Sets up the cached GetListRect. Decided whether it should
	//  have a scrollbar, and so on.
	virtual void SetupListRect();

	// Specify a new List rectangle.
	virtual CRect GetListRect() const;

	/**
	 * Placement of text.
	 */
	CPos m_TextPos;

	// Is the dropdown opened?
	bool m_Open;

	// I didn't cache this at first, but it's just as easy as caching
	//  m_CachedActualSize, so I thought, what the heck it's used a lot.
	CRect m_CachedListRect;

	// Hide scrollbar when it's not needed
	bool m_HideScrollBar;

	// Not necessarily the element that is selected, this is just
	//  which element should be highlighted. When opening the dropdown
	//  it is set to "selected", but then when moving the mouse it will
	//  change.
	int m_ElementHighlight;
};

#endif
