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

#include "gui/CGUI.h"
#include "gui/CGUIText.h"
#include "gui/SettingTypes/CGUIColor.h"

CButton::CButton(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUIButtonBehavior(*static_cast<IGUIObject*>(this)),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  m_BufferZone(),
	  m_CellID(),
	  m_Caption(),
	  m_Font(),
	  m_Sprite(),
	  m_SpriteOver(),
	  m_SpritePressed(),
	  m_SpriteDisabled(),
	  m_TextAlign(),
	  m_TextVAlign(),
	  m_TextColor(),
	  m_TextColorOver(),
	  m_TextColorPressed(),
	  m_TextColorDisabled()
{
	RegisterSetting("buffer_zone", m_BufferZone);
	RegisterSetting("cell_id", m_CellID);
	RegisterSetting("caption", m_Caption);
	RegisterSetting("font", m_Font);
	RegisterSetting("sprite", m_Sprite);
	RegisterSetting("sprite_over", m_SpriteOver);
	RegisterSetting("sprite_pressed", m_SpritePressed);
	RegisterSetting("sprite_disabled", m_SpriteDisabled);
	RegisterSetting("text_align", m_TextAlign);
	RegisterSetting("text_valign", m_TextVAlign);
	RegisterSetting("textcolor", m_TextColor);
	RegisterSetting("textcolor_over", m_TextColorOver);
	RegisterSetting("textcolor_pressed", m_TextColorPressed);
	RegisterSetting("textcolor_disabled", m_TextColorDisabled);

	AddText();
}

CButton::~CButton()
{
}

void CButton::SetupText()
{
	ENSURE(m_GeneratedTexts.size() == 1);

	m_GeneratedTexts[0] = CGUIText(m_pGUI, m_Caption, m_Font, m_CachedActualSize.GetWidth(), m_BufferZone, this);
	CalculateTextPosition(m_CachedActualSize, m_TextPos, m_GeneratedTexts[0]);
}

void CButton::ResetStates()
{
	IGUIObject::ResetStates();
	IGUIButtonBehavior::ResetStates();
}

void CButton::UpdateCachedSize()
{
	IGUIObject::UpdateCachedSize();
	IGUITextOwner::UpdateCachedSize();
}

void CButton::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);
}

void CButton::Draw()
{
	const float bz = GetBufferedZ();

	m_pGUI.DrawSprite(
		GetButtonSprite(m_Sprite, m_SpriteOver, m_SpritePressed, m_SpriteDisabled),
		m_CellID,
		bz,
		m_CachedActualSize);

	DrawText(0, ChooseColor(), m_TextPos, bz + 0.1f);
}

const CGUIColor& CButton::ChooseColor()
{
	if (!m_Enabled)
		return m_TextColorDisabled || m_TextColor;

	if (!m_MouseHovering)
		return m_TextColor;

	if (m_Pressed)
		return m_TextColorPressed || m_TextColor;

	return m_TextColorOver || m_TextColor;
}
