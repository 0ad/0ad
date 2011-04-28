/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
GUI Object - Check box

--Overview--

	GUI Object representing a check box

--More info--

	Check GUI.h

*/

#ifndef INCLUDED_CCHECKBOX
#define INCLUDED_CCHECKBOX

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

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates() { IGUIButtonBehavior::ResetStates(); }

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage &Message);

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
