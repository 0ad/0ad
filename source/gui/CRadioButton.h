/*
GUI Object - Radio Button

--Overview--

	GUI Object representing a radio button

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CRADIOBUTTON
#define INCLUDED_CRADIOBUTTON

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------
#include "GUI.h"
#include "CCheckBox.h"

/**
 * Just like a check box, but it'll nullify its siblings (of the same kind),
 * and it won't switch itself.
 * 
 * @see CCheckBox
 */
class CRadioButton : public CCheckBox
{
	GUI_OBJECT(CRadioButton)

public:
	/**
	 * Handle Messages
	 *
	 * @param Message GUI Message
	 */
	virtual void HandleMessage(const SGUIMessage &Message);
};

#endif
