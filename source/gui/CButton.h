/* Copyright (C) 2019 Wildfire Games.
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

#ifndef INCLUDED_CBUTTON
#define INCLUDED_CBUTTON

#include "GUI.h"

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
	CButton(CGUI& pGUI);
	virtual ~CButton();

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates() { IGUIButtonBehavior::ResetStates(); }

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

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

#endif // INCLUDED_CBUTTON
