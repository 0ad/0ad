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
CCheckBox
*/

#include "precompiled.h"
#include "GUI.h"
#include "CCheckBox.h"

#include "ps/Font.h"
#include "ps/CLogger.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CCheckBox::CCheckBox()
{
	AddSetting(GUIST_float,					"buffer_zone");
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_int,					"cell_id");
	AddSetting(GUIST_bool,					"checked");
	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_disabled");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_disabled");
	AddSetting(GUIST_float,					"square_side");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CColor,				"textcolor_over");
	AddSetting(GUIST_CColor,				"textcolor_pressed");
	AddSetting(GUIST_CColor,				"textcolor_disabled");
	AddSetting(GUIST_CStrW,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip_style");

	// Add text
	AddText(new SGUIText());
}

CCheckBox::~CCheckBox()
{
}

void CCheckBox::SetupText()
{
	if (!GetGUI())
		return;

	ENSURE(m_GeneratedTexts.size()==1);

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Default should not be hard-coded, but be in styles!
		font = L"default";

	float square_side;
	GUI<float>::GetSetting(this, "square_side", square_side);

	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	float buffer_zone=0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, m_CachedActualSize.GetWidth()-square_side, 0.f, this);
}

void CCheckBox::HandleMessage(SGUIMessage &Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
	{
		bool checked;

		GUI<bool>::GetSetting(this, "checked", checked);
		checked = !checked;
		GUI<bool>::SetSetting(this, "checked", checked);
		
	}	break;

	default:
		break;
	}
}

void CCheckBox::Draw() 
{
	////////// Gee: janwas, this is just temp to see it
	glDisable(GL_TEXTURE_2D);
	//////////

	float square_side, buffer_zone;
	CStrW font_name;
	bool checked;
	int cell_id;
	GUI<float>::GetSetting(this, "square_side", square_side);
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<CStrW>::GetSetting(this, "font", font_name);
	GUI<bool>::GetSetting(this, "checked", checked);
	GUI<int>::GetSetting(this, "cell_id", cell_id);

	// Get line height
	CFont font (font_name);
	float line_height = (float)font.GetHeight();

	float bz = GetBufferedZ();

	// Get square
	CRect rect;

	rect.left =		m_CachedActualSize.left;
	rect.right =	rect.left + square_side;

	if (square_side >= line_height)
		rect.top =	m_CachedActualSize.top;
	else
		rect.top =	m_CachedActualSize.top + line_height/2.f - square_side/2.f;

	rect.bottom =	rect.top + square_side;

	CGUISpriteInstance *sprite, *sprite_over, *sprite_pressed, *sprite_disabled;

	if (checked)
	{
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2", sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_over", sprite_over);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_pressed", sprite_pressed);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_disabled", sprite_disabled);
	}
	else
	{
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_over", sprite_over);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_pressed", sprite_pressed);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_disabled", sprite_disabled);
	}

	DrawButton(rect, 
			   bz,
			   *sprite,
			   *sprite_over,
			   *sprite_pressed,
			   *sprite_disabled,
			   cell_id);

	CColor color = ChooseColor();

	CPos text_pos(m_CachedActualSize.left + square_side + buffer_zone, m_CachedActualSize.top);

	if (square_side > line_height)
		text_pos.y += square_side/2.f - line_height/2.f;

	IGUITextOwner::Draw(0, color, text_pos, bz+0.1f, m_CachedActualSize);
}
