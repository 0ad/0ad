/*
GUI Object - Button
by Gustav Larsson
gee@pyro.nu

--Overview--

	Interface class that enhance the IGUIObject with
	 buttony behavior (click and release to click a button),
	 and the GUI message GUIM_PRESSED.
	When creating a class with extended settings and
	 buttony behavior, just do a multiple inheritance.

--More info--

	Check GUI.h

*/

#ifndef IGUIButtonBehavior_H
#define IGUIButtonBehavior_H

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
 * Appends button behaviours to the IGUIObject.
 * Can be used with multiple inheritance alongside
 * IGUISettingsObject and such.
 *
 * @see IGUIObject
 */
class IGUIButtonBehavior : virtual public IGUIObject
{
public:
	IGUIButtonBehavior();
	virtual ~IGUIButtonBehavior();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(const EGUIMessage &Message);

protected:
	virtual void ResetStates()
	{
		m_MouseHovering = false;
		m_Pressed = false;
	}

	/**
	 * Everybody knows how a button works, you don't simply press it,
	 * you have to first press the button, and then release it...
	 * in between those two steps you can actually leave the button
	 * area, as long as you release it within the button area... Anyway
	 * this lets us know we are done with step one (clicking).
	 */
	bool							m_Pressed;
};

#endif
