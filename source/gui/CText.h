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

#ifndef INCLUDED_CTEXT
#define INCLUDED_CTEXT

#include "gui/IGUIScrollBarOwner.h"
#include "gui/IGUITextOwner.h"

/**
 * Text field that just displays static text.
 */
class CText : public IGUIScrollBarOwner, public IGUITextOwner
{
	GUI_OBJECT(CText)

public:
	CText(CGUI& pGUI);
	virtual ~CText();

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates() { IGUIScrollBarOwner::ResetStates(); }

	/**
	 * Test if mouse position is over an icon
	 */
	virtual bool MouseOverIcon();

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Draws the Text
	 */
	virtual void Draw();

	/**
	 * Placement of text. Ignored when scrollbars are active.
	 */
	CPos m_TextPos;
};

#endif // INCLUDED_CTEXT
