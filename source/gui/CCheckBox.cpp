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

#include "precompiled.h"

#include "CCheckBox.h"

#include "gui/CGUIColor.h"
#include "graphics/FontMetrics.h"
#include "ps/CLogger.h"
#include "ps/CStrIntern.h"

/**
 * TODO: Since there is no call to DrawText, the checkbox won't render any text.
 * Thus the font, caption, textcolor and other settings have no effect.
 */
CCheckBox::CCheckBox(CGUI* pGUI)
	: IGUIObject(pGUI), IGUITextOwner(pGUI), IGUIButtonBehavior(pGUI)
{
	AddSetting<float>("buffer_zone");
	AddSetting<CGUIString>("caption");
	AddSetting<int>("cell_id");
	AddSetting<bool>("checked");
	AddSetting<CStrW>("font");
	AddSetting<CStrW>("sound_disabled");
	AddSetting<CStrW>("sound_enter");
	AddSetting<CStrW>("sound_leave");
	AddSetting<CStrW>("sound_pressed");
	AddSetting<CStrW>("sound_released");
	AddSetting<CGUISpriteInstance>("sprite");
	AddSetting<CGUISpriteInstance>("sprite_over");
	AddSetting<CGUISpriteInstance>("sprite_pressed");
	AddSetting<CGUISpriteInstance>("sprite_disabled");
	AddSetting<CGUISpriteInstance>("sprite2");
	AddSetting<CGUISpriteInstance>("sprite2_over");
	AddSetting<CGUISpriteInstance>("sprite2_pressed");
	AddSetting<CGUISpriteInstance>("sprite2_disabled");
	AddSetting<float>("square_side");
	AddSetting<CGUIColor>("textcolor");
	AddSetting<CGUIColor>("textcolor_over");
	AddSetting<CGUIColor>("textcolor_pressed");
	AddSetting<CGUIColor>("textcolor_disabled");
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");

	AddText(new SGUIText());
}

CCheckBox::~CCheckBox()
{
}

void CCheckBox::SetupText()
{
	if (!GetGUI())
		return;

	ENSURE(m_GeneratedTexts.size() == 1);

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Default should not be hard-coded, but be in styles!
		font = L"default";

	float square_side;
	GUI<float>::GetSetting(this, "square_side", square_side);

	CGUIString* caption = nullptr;
	GUI<CGUIString>::GetSettingPointer(this, "caption", caption);

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	*m_GeneratedTexts[0] = GetGUI()->GenerateText(*caption, font, m_CachedActualSize.GetWidth()-square_side, 0.f, this);
}

void CCheckBox::HandleMessage(SGUIMessage& Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
	{
		bool checked;

		// Switch to opposite.
		GUI<bool>::GetSetting(this, "checked", checked);
		checked = !checked;
		GUI<bool>::SetSetting(this, "checked", checked);
		break;
	}

	default:
		break;
	}
}

void CCheckBox::Draw()
{
	float bz = GetBufferedZ();
	bool checked;
	int cell_id;
	CGUISpriteInstance* sprite;
	CGUISpriteInstance* sprite_over;
	CGUISpriteInstance* sprite_pressed;
	CGUISpriteInstance* sprite_disabled;

	GUI<bool>::GetSetting(this, "checked", checked);
	GUI<int>::GetSetting(this, "cell_id", cell_id);

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

	DrawButton(m_CachedActualSize,
			   bz,
			   *sprite,
			   *sprite_over,
			   *sprite_pressed,
			   *sprite_disabled,
			   cell_id);
}
