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

#include "CButton.h"

#include "gui/CGUIColor.h"
#include "lib/ogl.h"

CButton::CButton(CGUI& pGUI)
	: IGUIObject(pGUI), IGUIButtonBehavior(pGUI), IGUITextOwner(pGUI)
{
	AddSetting<float>("buffer_zone");
	AddSetting<CGUIString>("caption");
	AddSetting<int>("cell_id");
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
	AddSetting<EAlign>("text_align");
	AddSetting<EVAlign>("text_valign");
	AddSetting<CGUIColor>("textcolor");
	AddSetting<CGUIColor>("textcolor_over");
	AddSetting<CGUIColor>("textcolor_pressed");
	AddSetting<CGUIColor>("textcolor_disabled");
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");

	AddText();
}

CButton::~CButton()
{
}

void CButton::SetupText()
{
	ENSURE(m_GeneratedTexts.size() == 1);

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Default should not be hard-coded, but be in styles!
		font = L"default";

	const CGUIString& caption = GUI<CGUIString>::GetSetting(this, "caption");

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	m_GeneratedTexts[0] = CGUIText(m_pGUI, caption, font, m_CachedActualSize.GetWidth(), buffer_zone, this);

	CalculateTextPosition(m_CachedActualSize, m_TextPos, m_GeneratedTexts[0]);
}

void CButton::HandleMessage(SGUIMessage& Message)
{
	// Important
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);
}

void CButton::Draw()
{
	float bz = GetBufferedZ();

	int cell_id;

	// Statically initialise some strings, so we don't have to do
	// lots of allocation every time this function is called
	static const CStr strSprite("sprite");
	static const CStr strSpriteOver("sprite_over");
	static const CStr strSpritePressed("sprite_pressed");
	static const CStr strSpriteDisabled("sprite_disabled");
	static const CStr strCellId("cell_id");

	CGUISpriteInstance& sprite = GUI<CGUISpriteInstance>::GetSetting(this, strSprite);
	CGUISpriteInstance& sprite_over = GUI<CGUISpriteInstance>::GetSetting(this, strSpriteOver);
	CGUISpriteInstance& sprite_pressed = GUI<CGUISpriteInstance>::GetSetting(this, strSpritePressed);
	CGUISpriteInstance& sprite_disabled = GUI<CGUISpriteInstance>::GetSetting(this, strSpriteDisabled);

	GUI<int>::GetSetting(this, strCellId, cell_id);

	DrawButton(m_CachedActualSize, bz, sprite, sprite_over, sprite_pressed, sprite_disabled, cell_id);

	DrawText(0, ChooseColor(), m_TextPos, bz + 0.1f);
}
