/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_CCHECKBOX
#define INCLUDED_CCHECKBOX

#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectBases/IGUIButtonBehavior.h"

class CCheckBox : public IGUIObject, public IGUIButtonBehavior
{
	GUI_OBJECT(CCheckBox)

public:
	CCheckBox(CGUI& pGUI);
	virtual ~CCheckBox();

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Draws the control
	 */
	virtual void Draw(CCanvas2D& canvas);

protected:
	// Settings
	CGUISimpleSetting<bool> m_Checked;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteUnchecked;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteUncheckedOver;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteUncheckedPressed;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteUncheckedDisabled;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteChecked;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteCheckedOver;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteCheckedPressed;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteCheckedDisabled;
};

#endif // INCLUDED_CCHECKBOX
