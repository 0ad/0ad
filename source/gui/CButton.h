/*
GUI Object - Button

--Overview--

	GUI Object representing a simple button

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CBUTTON
#define INCLUDED_CBUTTON

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"

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
 * Button
 * 
 * @see IGUIObject
 * @see IGUIButtonBehavior
 */
class CButton : public IGUIButtonBehavior, public IGUITextOwner
{
	GUI_OBJECT(CButton)

public:
	CButton();
	virtual ~CButton();

	virtual void ResetStates() { IGUIButtonBehavior::ResetStates(); }

	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Draws the Button
	 */
	virtual void Draw();

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();

	/**
	 * Placement of text.
	 */
	CPos m_TextPos;
};

#endif
