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

CCheckBox::CCheckBox(CGUI& pGUI)
	: IGUIObject(pGUI), IGUIButtonBehavior(pGUI)
{
	AddSetting<i32>("cell_id");
	AddSetting<bool>("checked");
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
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");
}

CCheckBox::~CCheckBox()
{
}

void CCheckBox::HandleMessage(SGUIMessage& Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
	{
		// Switch to opposite.
		SetSetting<bool>("checked", !GetSetting<bool>("checked"), true);
		break;
	}

	default:
		break;
	}
}

void CCheckBox::Draw()
{
	if (GetSetting<bool>("checked"))
		DrawButton(
			m_CachedActualSize,
			GetBufferedZ(),
			GetSetting<CGUISpriteInstance>("sprite2"),
			GetSetting<CGUISpriteInstance>("sprite2_over"),
			GetSetting<CGUISpriteInstance>("sprite2_pressed"),
			GetSetting<CGUISpriteInstance>("sprite2_disabled"),
			GetSetting<i32>("cell_id"));
	else
		DrawButton(
			m_CachedActualSize,
			GetBufferedZ(),
			GetSetting<CGUISpriteInstance>("sprite"),
			GetSetting<CGUISpriteInstance>("sprite_over"),
			GetSetting<CGUISpriteInstance>("sprite_pressed"),
			GetSetting<CGUISpriteInstance>("sprite_disabled"),
			GetSetting<i32>("cell_id"));
}
