/* Copyright (C) 2020 Wildfire Games.
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

#include "CSlider.h"

#include "gui/CGUI.h"
#include "maths/MathUtil.h"

const CStr CSlider::EventNameValueChange = "ValueChange";

CSlider::CSlider(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUIButtonBehavior(*static_cast<IGUIObject*>(this)),
	  m_ButtonSide(),
	  m_CellID(),
	  m_MaxValue(),
	  m_MinValue(),
	  m_Sprite(),
	  m_SpriteBar(),
	  m_Value()
{
	RegisterSetting("button_width", m_ButtonSide);
	RegisterSetting("cell_id", m_CellID);
	RegisterSetting("max_value", m_MaxValue);
	RegisterSetting("min_value", m_MinValue);
	RegisterSetting("sprite", m_Sprite);
	RegisterSetting("sprite_bar", m_SpriteBar);
	RegisterSetting("value", m_Value);

	m_Value = Clamp(m_Value, m_MinValue, m_MaxValue);
}

CSlider::~CSlider()
{
}

void CSlider::ResetStates()
{
	IGUIObject::ResetStates();
	IGUIButtonBehavior::ResetStates();
}

float CSlider::GetSliderRatio() const
{
	return (m_MaxValue - m_MinValue) / (m_CachedActualSize.GetWidth() - m_ButtonSide);
}

void CSlider::IncrementallyChangeValue(const float difference)
{
	m_Value = Clamp(m_Value + difference, m_MinValue, m_MaxValue);
	UpdateValue();
}

void CSlider::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
	{
		m_Value = Clamp(m_Value, m_MinValue, m_MaxValue);
		break;
	}
	case GUIM_MOUSE_WHEEL_DOWN:
	{
		if (m_Pressed)
			break;
		IncrementallyChangeValue(-0.01f);
		break;
	}
	case GUIM_MOUSE_WHEEL_UP:
	{
		if (m_Pressed)
			break;
		IncrementallyChangeValue(0.01f);
		break;
	}
	case GUIM_MOUSE_PRESS_LEFT:
		FALLTHROUGH;
	case GUIM_MOUSE_MOTION:
	{
		if (m_Pressed)
		{
			m_Mouse = m_pGUI.GetMousePos();
			IncrementallyChangeValue((m_Mouse.x - GetButtonRect().CenterPoint().x) * GetSliderRatio());
		}
		break;
	}
	default:
		break;
	}
}

void CSlider::Draw()
{
	CRect slider_line(m_CachedActualSize);
	slider_line.left += m_ButtonSide / 2.0f;
	slider_line.right -= m_ButtonSide / 2.0f;
	float bz = GetBufferedZ();
	m_pGUI.DrawSprite(m_SpriteBar, m_CellID, bz, slider_line);
	m_pGUI.DrawSprite(m_Sprite, m_CellID, bz, GetButtonRect());
}

void CSlider::UpdateValue()
{
	SetSetting<float>("value", m_Value, true);
	ScriptEvent(EventNameValueChange);
}

CRect CSlider::GetButtonRect() const
{
	float ratio = m_MaxValue > m_MinValue ? (m_Value - m_MinValue) / (m_MaxValue - m_MinValue) : 0.0f;
	float x = m_CachedActualSize.left + ratio * (m_CachedActualSize.GetWidth() - m_ButtonSide);
	float y = m_CachedActualSize.top + (m_CachedActualSize.GetHeight() - m_ButtonSide) / 2.0;
	return CRect(x, y, x + m_ButtonSide, y + m_ButtonSide);
}
