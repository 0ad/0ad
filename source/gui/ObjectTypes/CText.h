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

#ifndef INCLUDED_CTEXT
#define INCLUDED_CTEXT

#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/ObjectBases/IGUIScrollBarOwner.h"
#include "gui/ObjectBases/IGUITextOwner.h"
#include "gui/SettingTypes/CGUIString.h"

/**
 * Text field that just displays static text.
 */
class CText : public IGUIObject, public IGUIScrollBarOwner, public IGUITextOwner
{
	GUI_OBJECT(CText)
public:
	CText(CGUI& pGUI);
	virtual ~CText();

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

	virtual const CStrW& GetTooltipText() const;
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
	virtual void Draw(CCanvas2D& canvas);

	virtual void CreateJSObject();

	/**
	 * Placement of text. Ignored when scrollbars are active.
	 */
	CVector2D m_TextPos;

	CGUISimpleSetting<float> m_BufferZone;
	CGUISimpleSetting<CGUIString> m_Caption;
	CGUISimpleSetting<bool> m_Clip;
	CGUISimpleSetting<CStrW> m_Font;
	CGUISimpleSetting<bool> m_ScrollBar;
	CGUISimpleSetting<CStr> m_ScrollBarStyle;
	CGUISimpleSetting<bool> m_ScrollBottom;
	CGUISimpleSetting<bool> m_ScrollTop;
	CGUISimpleSetting<CGUISpriteInstance> m_Sprite;
	CGUISimpleSetting<CGUISpriteInstance> m_SpriteOverlay;
	CGUISimpleSetting<CGUIColor> m_TextColor;
	CGUISimpleSetting<CGUIColor> m_TextColorDisabled;
};

#endif // INCLUDED_CTEXT
