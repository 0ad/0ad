/* Copyright (C) 2021 Wildfire Games.
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
	  m_ButtonSide(this, "button_width"),
	  m_MaxValue(this, "max_value"),
	  m_MinValue(this, "min_value"),
	  m_Sprite(this, "sprite"),
	  m_SpriteDisabled(this, "sprite_disabled"),
	  m_SpriteBar(this, "sprite_bar"),
	  m_SpriteBarDisabled(this, "sprite_bar_disabled"),
	  m_Value(this, "value")
{
	m_Value.Set(Clamp<float>(m_Value, m_MinValue, m_MaxValue), false);
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
	m_Value.Set(Clamp<float>(m_Value + difference, m_MinValue, m_MaxValue), true);
	UpdateValue();
}

void CSlider::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUIButtonBehavior::HandleMessage(Message);

	switch (Message.type)
	{
	/*
	case GUIM_SETTINGS_UPDATED:
	{
		m_Value.Set(Clamp<float>(m_Value, m_MinValue, m_MaxValue), true);
		break;
	}
	*/
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
			IncrementallyChangeValue((m_Mouse.X - GetButtonRect().CenterPoint().X) * GetSliderRatio());
		}
		break;
	}
	default:
		break;
	}
}

void CSlider::Draw(CCanvas2D& canvas)
{
	CRect sliderLine(m_CachedActualSize);
	sliderLine.left += m_ButtonSide / 2.0f;
	sliderLine.right -= m_ButtonSide / 2.0f;
	m_pGUI.DrawSprite(IsEnabled() ? m_SpriteBar : m_SpriteBarDisabled, canvas, sliderLine);
	m_pGUI.DrawSprite(IsEnabled() ? m_Sprite : m_SpriteDisabled, canvas, GetButtonRect());
}

void CSlider::UpdateValue()
{
	ScriptEvent(EventNameValueChange);
}

CRect CSlider::GetButtonRect() const
{
	float ratio = m_MaxValue > m_MinValue ? (m_Value - m_MinValue) / (m_MaxValue - m_MinValue) : 0.0f;
	float x = m_CachedActualSize.left + ratio * (m_CachedActualSize.GetWidth() - m_ButtonSide);
	float y = m_CachedActualSize.top + (m_CachedActualSize.GetHeight() - m_ButtonSide) / 2.0;
	return CRect(x, y, x + m_ButtonSide, y + m_ButtonSide);
}
