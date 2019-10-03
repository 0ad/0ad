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

#include "gui/CGUI.h"

CCheckBox::CCheckBox(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUIButtonBehavior(*static_cast<IGUIObject*>(this)),
	  m_CellID(),
	  m_Checked(),
	  m_SpriteUnchecked(),
	  m_SpriteUncheckedOver(),
	  m_SpriteUncheckedPressed(),
	  m_SpriteUncheckedDisabled(),
	  m_SpriteChecked(),
	  m_SpriteCheckedOver(),
	  m_SpriteCheckedPressed(),
	  m_SpriteCheckedDisabled()
{
	RegisterSetting("cell_id", m_CellID);
	RegisterSetting("checked", m_Checked),
	RegisterSetting("sprite", m_SpriteUnchecked);
	RegisterSetting("sprite_over", m_SpriteUncheckedOver);
	RegisterSetting("sprite_pressed", m_SpriteUncheckedPressed);
	RegisterSetting("sprite_disabled", m_SpriteUncheckedDisabled);
	RegisterSetting("sprite2", m_SpriteChecked);
	RegisterSetting("sprite2_over", m_SpriteCheckedOver);
	RegisterSetting("sprite2_pressed", m_SpriteCheckedPressed);
	RegisterSetting("sprite2_disabled", m_SpriteCheckedDisabled);
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
		SetSetting<bool>("checked", !m_Checked, true);
		break;
	}

	default:
		break;
	}
}

void CCheckBox::Draw()
{
	m_pGUI.DrawSprite(
		m_Checked ?
			GetButtonSprite(m_SpriteChecked, m_SpriteCheckedOver, m_SpriteCheckedPressed, m_SpriteCheckedDisabled) :
			GetButtonSprite(m_SpriteUnchecked, m_SpriteUncheckedOver, m_SpriteUncheckedPressed, m_SpriteUncheckedDisabled),
		m_CellID,
		GetBufferedZ(),
		m_CachedActualSize);
}
