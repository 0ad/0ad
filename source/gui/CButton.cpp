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
CButton
*/

#include "precompiled.h"
#include "GUI.h"
#include "CButton.h"

#include "lib/ogl.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CButton::CButton()
{
	AddSetting(GUIST_float,					"buffer_zone");
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_int,					"cell_id");
	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_disabled");
	AddSetting(GUIST_EAlign,				"text_align");
	AddSetting(GUIST_EVAlign,				"text_valign");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CColor,				"textcolor_over");
	AddSetting(GUIST_CColor,				"textcolor_pressed");
	AddSetting(GUIST_CColor,				"textcolor_disabled");
	AddSetting(GUIST_CStr,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip_style");

	// Add text
	AddText(new SGUIText());
}

CButton::~CButton()
{
}

void CButton::SetupText()
{
	if (!GetGUI())
		return;

	debug_assert(m_GeneratedTexts.size()>=1);

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Default should not be hard-coded, but be in styles!
		font = L"default";

	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, m_CachedActualSize.GetWidth(), buffer_zone, this);

	CalculateTextPosition(m_CachedActualSize, m_TextPos, *m_GeneratedTexts[0]);
}

void CButton::HandleMessage(const SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);
/*
	switch (Message.type)
	{
	case GUIM_PRESSED:
//		GetGUI()->TEMPmessage = "Button " + string((const TCHAR*)m_Name) + " was pressed!";
		break;

	default:
		break;
	}
*/
}

void CButton::Draw() 
{
	float bz = GetBufferedZ();

	CGUISpriteInstance *sprite, *sprite_over, *sprite_pressed, *sprite_disabled;
	int cell_id;

	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_over", sprite_over);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_pressed", sprite_pressed);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_disabled", sprite_disabled);
	GUI<int>::GetSetting(this, "cell_id", cell_id);

	DrawButton(m_CachedActualSize,
			   bz,
			   *sprite,
			   *sprite_over,
			   *sprite_pressed,
			   *sprite_disabled,
			   cell_id);

	CColor color = ChooseColor();
	IGUITextOwner::Draw(0, color, m_TextPos, bz+0.1f);
}
