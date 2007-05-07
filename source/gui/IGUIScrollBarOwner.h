/*
GUI Object Base - Scroll-bar owner

--Overview--

	Base-class this if you want scroll-bars in an object.

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_IGUISCROLLBAROWNER
#define INCLUDED_IGUISCROLLBAROWNER

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

struct SGUIScrollBarStyle;
class IGUIScrollBar;

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
 * Base-class this if you want an object to contain
 * one, or several, scroll-bars.
 *
 * @see IGUIObject
 * @see IGUIScrollBar
 */
class IGUIScrollBarOwner : virtual public IGUIObject
{
	friend class IGUIScrollBar;

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
	virtual SGUIScrollBarStyle *GetScrollBarStyle(const CStr& style) const;

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
	std::vector<IGUIScrollBar*> m_ScrollBars;
};

#endif
