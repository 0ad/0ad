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
CCheckBox::CCheckBox(CGUI& pGUI)
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

	AddText();
}

CCheckBox::~CCheckBox()
{
}

void CCheckBox::SetupText()
{
	ENSURE(m_GeneratedTexts.size() == 1);

	m_GeneratedTexts[0] = CGUIText(
		m_pGUI,
		GUI<CGUIString>::GetSetting(this, "caption"),
		GUI<CStrW>::GetSetting(this, "font"),
		m_CachedActualSize.GetWidth() - GUI<float>::GetSetting(this, "square_side"),
		GUI<float>::GetSetting(this, "buffer_zone"),
		this);
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
		// Switch to opposite.
		GUI<bool>::SetSetting(this, "checked", !GUI<bool>::GetSetting(this, "checked"));
		break;
	}

	default:
		break;
	}
}

void CCheckBox::Draw()
{
	if (GUI<bool>::GetSetting(this, "checked"))
		DrawButton(
			m_CachedActualSize,
			GetBufferedZ(),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite2"),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite2_over"),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite2_pressed"),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite2_disabled"),
			GUI<int>::GetSetting(this, "cell_id"));
	else
		DrawButton(
			m_CachedActualSize,
			GetBufferedZ(),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite"),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite_over"),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite_pressed"),
			GUI<CGUISpriteInstance>::GetSetting(this, "sprite_disabled"),
			GUI<int>::GetSetting(this, "cell_id"));
}
