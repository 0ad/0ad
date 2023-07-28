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

#include "precompiled.h"

#include "CButton.h"

#include "gui/CGUI.h"
#include "gui/CGUIText.h"
#include "gui/SettingTypes/CGUIColor.h"

CButton::CButton(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUIButtonBehavior(*static_cast<IGUIObject*>(this)),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  m_BufferZone(this, "buffer_zone"),
	  m_Caption(this, "caption"),
	  m_Font(this, "font"),
	  m_Sprite(this, "sprite"),
	  m_SpriteOver(this, "sprite_over"),
	  m_SpritePressed(this, "sprite_pressed"),
	  m_SpriteDisabled(this, "sprite_disabled"),
	  m_TextColor(this, "textcolor"),
	  m_TextColorOver(this, "textcolor_over"),
	  m_TextColorPressed(this, "textcolor_pressed"),
	  m_TextColorDisabled(this, "textcolor_disabled"),
	  m_MouseEventMask(this)
{
	AddText();
}

CButton::~CButton()
{
}

void CButton::SetupText()
{
	ENSURE(m_GeneratedTexts.size() == 1);

	m_GeneratedTexts[0] = CGUIText(m_pGUI, m_Caption, m_Font, m_CachedActualSize.GetWidth(), m_BufferZone, m_TextAlign, this);
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

CSize2D CButton::GetTextSize()
{
	UpdateText();
	return m_GeneratedTexts[0].GetSize();
}

void CButton::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUIButtonBehavior::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);
}

void CButton::Draw(CCanvas2D& canvas)
{
	m_pGUI.DrawSprite(
		GetButtonSprite(m_Sprite, m_SpriteOver, m_SpritePressed, m_SpriteDisabled),
		canvas,
		m_CachedActualSize);

	DrawText(canvas, 0, ChooseColor(), m_TextPos);
}

bool CButton::IsMouseOver() const
{
	if (!IGUIObject::IsMouseOver())
		return false;
	if (!m_MouseEventMask)
		return true;
	return m_MouseEventMask.IsMouseOver(m_pGUI.GetMousePos(), m_CachedActualSize);
}

const CGUIColor& CButton::ChooseColor()
{
	if (!m_Enabled)
		return *m_TextColorDisabled ? m_TextColorDisabled : m_TextColor;

	if (!m_MouseHovering)
		return m_TextColor;

	if (m_Pressed)
		return *m_TextColorPressed ? m_TextColorPressed : m_TextColor;

	return *m_TextColorOver ? m_TextColorOver : m_TextColor;
}
