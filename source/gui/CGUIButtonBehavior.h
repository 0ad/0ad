/*
GUI Object - Button
by Gustav Larsson
gee@pyro.nu

--Overview--

	Interface class that enhance the CGUIObject with
	 buttony behavior (click and release to click a button),
	 and the GUI message GUIM_PRESSED.
	When creating a class with extended settings and
	 buttony behavior, just do a multiple inheritance.

--More info--

	Check GUI.h

*/

#ifndef CGUIButtonBehavior_H
#define CGUIButtonBehavior_H

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
 * Appends button behaviours to the <code>CGUIObject</code>.
 * Can be used with multiple inheritance alongside
 * <code>CGUISettingsObject</code> and such.
 *
 * @see CGUIObject
 */
class CGUIButtonBehavior : virtual public CGUIObject
{
public:
	CGUIButtonBehavior();
	virtual ~CGUIButtonBehavior();

	/// Handle Messages
	virtual void HandleMessage(const EGUIMessage &Message);

protected:
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