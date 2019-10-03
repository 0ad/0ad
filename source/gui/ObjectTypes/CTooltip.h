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

#ifndef INCLUDED_CTOOLTIP
#define INCLUDED_CTOOLTIP

#include "gui/ObjectBases/IGUITextOwner.h"
#include "gui/CGUISprite.h"
#include "gui/SettingTypes/CGUIString.h"

/**
 * Dynamic tooltips. Similar to CText.
 */
class CTooltip : public IGUIObject, public IGUITextOwner
{
	GUI_OBJECT(CTooltip)

public:
	CTooltip(CGUI& pGUI);
	virtual ~CTooltip();

protected:
	void SetupText();

	/**
	 * @see IGUIObject#UpdateCachedSize()
	 */
	void UpdateCachedSize();

	/**
	 * @see IGUIObject#HandleMessage()
	 */
	virtual void HandleMessage(SGUIMessage& Message);

	virtual void Draw();

	// Settings
	float m_BufferZone;
	CGUIString m_Caption;
	CStrW m_Font;
	CGUISpriteInstance m_Sprite;
	i32 m_Delay;
	CGUIColor m_TextColor;
	float m_MaxWidth;
	CPos m_Offset;
	EVAlign m_Anchor;
	EAlign m_TextAlign;
	bool m_Independent;
	CPos m_MousePos;
	CStr m_UseObject;
	bool m_HideObject;
};

#endif // INCLUDED_CTOOLTIP
