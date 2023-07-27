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

#include "CCheckBox.h"

#include "gui/CGUI.h"

CCheckBox::CCheckBox(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUIButtonBehavior(*static_cast<IGUIObject*>(this)),
	  m_Checked(this, "checked"),
	  m_SpriteUnchecked(this, "sprite"),
	  m_SpriteUncheckedOver(this, "sprite_over"),
	  m_SpriteUncheckedPressed(this, "sprite_pressed"),
	  m_SpriteUncheckedDisabled(this, "sprite_disabled"),
	  m_SpriteChecked(this, "sprite2"),
	  m_SpriteCheckedOver(this, "sprite2_over"),
	  m_SpriteCheckedPressed(this, "sprite2_pressed"),
	  m_SpriteCheckedDisabled(this, "sprite2_disabled")
{
}

CCheckBox::~CCheckBox()
{
}

void CCheckBox::ResetStates()
{
	IGUIObject::ResetStates();
	IGUIButtonBehavior::ResetStates();
}

void CCheckBox::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_PRESSED:
	{
		m_Checked.Set(!m_Checked, true);
		break;
	}

	default:
		break;
	}
}

void CCheckBox::Draw(CCanvas2D& canvas)
{
	m_pGUI.DrawSprite(
		m_Checked ?
			GetButtonSprite(m_SpriteChecked, m_SpriteCheckedOver, m_SpriteCheckedPressed, m_SpriteCheckedDisabled) :
			GetButtonSprite(m_SpriteUnchecked, m_SpriteUncheckedOver, m_SpriteUncheckedPressed, m_SpriteUncheckedDisabled),
		canvas,
		m_CachedActualSize);
}
