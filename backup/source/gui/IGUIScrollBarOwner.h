/*
GUI Object Base - Scroll-bar owner
by Gustav Larsson
gee@pyro.nu

--Overview--

	Base-class this if you want scroll-bars in an object.

--More info--

	Check GUI.h

*/

#ifndef IGUIScrollBarOwner_H
#define IGUIScrollBarOwner_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

struct SGUIScrollBarStyle;

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
 * Base-class this if you want an object to contain
 * one, or several, scroll-bars.
 *
 * @see IGUIObject
 * @see IGUIScrollBar
 */
class IGUIScrollBarOwner : virtual public IGUIObject
{
public:
	IGUIScrollBarOwner();
	virtual ~IGUIScrollBarOwner();

	virtual void Draw();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * 
	 */
	virtual void ResetStates();

	/**
	 * Interface for the m_ScrollBar to use.
	 */
	virtual const SGUIScrollBarStyle & GetScrollBarStyle(const CStr &style) const;

	/**
	 * Add a scroll-bar
	 */
	virtual void AddScrollBar(IGUIScrollBar * scrollbar);

	/**
	 * Get Scroll Bar reference (it should be transparent it's actually
	 * pointers).
	 */
	virtual IGUIScrollBar & GetScrollBar(const int &index)
	{
		return *m_ScrollBars[index];
	}

protected:

	/**
	 * Predominately you will only have one, but you can have
	 * as many as you like.
	 */
	vector<IGUIScrollBar*> m_ScrollBars;
};

#endif
