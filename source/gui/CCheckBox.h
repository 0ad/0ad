/*
GUI Object - Check box
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object representing a check box

--More info--

	Check GUI.h

*/

#ifndef CCheckBox_H
#define CCheckBox_H

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
 * @author Gustav Larsson
 *
 * CheckBox
 * 
 * @see IGUIObject
 * @see IGUISettingsObject
 * @see IGUIButtonBehavior
 */
class CCheckBox : public IGUIButtonBehavior, public IGUITextOwner
{
	GUI_OBJECT(CCheckBox)

public:
	CCheckBox();
	virtual ~CCheckBox();

	virtual void ResetStates() { IGUIButtonBehavior::ResetStates(); }

	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);

	/**
	 * Draws the control
	 */
	virtual void Draw();

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();
};

#endif
