/*
GUI Object - Text [field]
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object representing a text field

--More info--

	Check GUI.h

*/

#ifndef CText_H
#define CText_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

// TODO Gee: Remove
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
 * @author Gustav Larsson
 *
 * Text field that just displays static text.
 * 
 * @see IGUIObject
 */
class CText : public IGUIScrollBarOwner, public IGUITextOwner
{
	GUI_OBJECT(CText)

public:
	CText();
	virtual ~CText();

	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();

	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Draws the Text
	 */
	virtual void Draw();
};

#endif
