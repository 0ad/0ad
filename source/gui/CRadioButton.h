/*
GUI Object - Radio Button
by Gustav Larsson
gee@pyro.nu

--Overview--

	GUI Object representing a radio button

--More info--

	Check GUI.h

*/

#ifndef CRadioButton_H
#define CRadioButton_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include "CCheckBox.h"

/**
 * @author Gustav Larsson
 *
 * Just like a check box, but it'll nullify its siblings (of the same kind),
 * and it won't switch itself.
 * 
 * @see CCheckBox
 */
class CRadioButton : public CCheckBox
{
	GUI_OBJECT(CRadioButton)

	// Let the class freely interact with its siblings
	friend class CRadioButton;

public:
	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);
};

#endif
