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

#include "gui/CGUISprite.h"
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
	 * Test if mouse position is over an icon
	 */
	virtual bool MouseOverIcon();

protected:
	/**
	 * Sets up text, should be called every time changes has been
	 * made that can change the visual.
	 */
	void SetupText();

	virtual void RegisterScriptFunctions();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	/**
	 * Draws the Text
	 */
	virtual void Draw();

	/**
	 * Script accessors to this GUI object.
	 */
	static JSFunctionSpec JSI_methods[];

	static bool GetTextSize(JSContext* cx, uint argc, JS::Value* vp);

	/**
	 * Placement of text. Ignored when scrollbars are active.
	 */
	CPos m_TextPos;

	// Settings
	float m_BufferZone;
	CGUIString m_Caption;
	i32 m_CellID;
	bool m_Clip;
	CStrW m_Font;
	bool m_ScrollBar;
	CStr m_ScrollBarStyle;
	bool m_ScrollBottom;
	bool m_ScrollTop;
	CGUISpriteInstance m_Sprite;
	EAlign m_TextAlign;
	EVAlign m_TextVAlign;
	CGUIColor m_TextColor;
	CGUIColor m_TextColorDisabled;
	CStrW m_IconTooltip;
	CStr m_IconTooltipStyle;
};

#endif // INCLUDED_CTEXT
