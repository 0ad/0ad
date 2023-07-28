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

#ifndef INCLUDED_CBUTTON
#define INCLUDED_CBUTTON

#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIButtonBehavior.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectBases/IGUITextOwner.h"
#include "gui/SettingTypes/CGUIString.h"
#include "gui/SettingTypes/MouseEventMask.h"
#include "maths/Vector2D.h"

class CButton : public IGUIObject, public IGUITextOwner, public IGUIButtonBehavior
{
	GUI_OBJECT(CButton)
public:
	CButton(CGUI& pGUI);
	virtual ~CButton();

	/**
	 * @see IGUIObject#ResetStates()
	 */
	virtual void ResetStates();

	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	virtual void UpdateCachedSize();

	/**
	 * @return the object text size.
	 */
	CSize2D GetTextSize();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Draws the Button
	 */
	virtual void Draw(CCanvas2D& canvas);

	/**
	 * @see IGUIObject#IsMouseOver()
	 */
	virtual bool IsMouseOver() const;

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();

	/**
	 * Picks the text color depending on current object settings.
	 */
	const CGUIColor& ChooseColor();

	/**
	 * Placement of text.
	 */
	CVector2D m_TextPos;

	virtual void CreateJSObject();

	// Settings
	CGUISimpleSetting<float> m_BufferZone;
	CGUISimpleSetting<CGUIString> m_Caption;
	CGUISimpleSetting<CStrW> m_Font;
	CGUISimpleSetting<CGUISpriteInstance> m_Sprite;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteOver;
	CGUISimpleSetting<CGUISpriteInstance> m_SpritePressed;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteDisabled;
	CGUISimpleSetting<CGUIColor> m_TextColor;
	CGUISimpleSetting<CGUIColor> m_TextColorOver;
	CGUISimpleSetting<CGUIColor> m_TextColorPressed;
	CGUISimpleSetting<CGUIColor> m_TextColorDisabled;
	CGUIMouseEventMask m_MouseEventMask;
};

#endif // INCLUDED_CBUTTON
