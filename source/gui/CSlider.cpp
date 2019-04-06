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
#include "CSlider.h"
#include "GUI.h"
#include "lib/ogl.h"
#include "ps/CLogger.h"


CSlider::CSlider()
	: m_IsPressed(false), m_ButtonSide(0)
{
	AddSetting(GUIST_float, "value");
	AddSetting(GUIST_float, "min_value");
	AddSetting(GUIST_float, "max_value");
	AddSetting(GUIST_int, "cell_id");
	AddSetting(GUIST_CGUISpriteInstance, "sprite");
	AddSetting(GUIST_CGUISpriteInstance, "sprite_bar");
	AddSetting(GUIST_float, "button_width");

	GUI<float>::GetSetting(this, "value", m_Value);
	GUI<float>::GetSetting(this, "min_value", m_MinValue);
	GUI<float>::GetSetting(this, "max_value", m_MaxValue);
	GUI<float>::GetSetting(this, "button_width", m_ButtonSide);
	m_Value = Clamp(m_Value, m_MinValue, m_MaxValue);
}

CSlider::~CSlider()
{
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
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
	{
		GUI<float>::GetSetting(this, "value", m_Value);
		GUI<float>::GetSetting(this, "min_value", m_MinValue);
		GUI<float>::GetSetting(this, "max_value", m_MaxValue);
		GUI<float>::GetSetting(this, "button_width", m_ButtonSide);
		m_Value = Clamp(m_Value, m_MinValue, m_MaxValue);
		break;
	}
	case GUIM_MOUSE_WHEEL_DOWN:
	{
		if (m_IsPressed)
			break;
		IncrementallyChangeValue(-0.01f);
		break;
	}
	case GUIM_MOUSE_WHEEL_UP:
	{
		if (m_IsPressed)
			break;
		IncrementallyChangeValue(0.01f);
		break;
	}
	case GUIM_MOUSE_PRESS_LEFT:
	{
		m_Mouse = GetMousePos();
		m_IsPressed = true;

		IncrementallyChangeValue((m_Mouse.x - GetButtonRect().CenterPoint().x) * GetSliderRatio());
		break;
	}
	case GUIM_MOUSE_RELEASE_LEFT:
	{
		m_IsPressed = false;
		break;
	}
	case GUIM_MOUSE_MOTION:
	{
		if (!MouseOver())
			m_IsPressed = false;
		if (m_IsPressed)
		{
			float difference = float(GetMousePos().x - m_Mouse.x) * GetSliderRatio();
			m_Mouse = GetMousePos();
			IncrementallyChangeValue(difference);
		}
		break;
	}
	default:
		break;
	}
}

void CSlider::Draw()
{
	if (!GetGUI())
		return;

	CGUISpriteInstance* sprite;
	CGUISpriteInstance* sprite_button;
	int cell_id;
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_bar", sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite_button);
	GUI<int>::GetSetting(this, "cell_id", cell_id);

	CRect slider_line(m_CachedActualSize);
	slider_line.left += m_ButtonSide / 2.0f;
	slider_line.right -= m_ButtonSide / 2.0f;
	float bz = GetBufferedZ();
	GetGUI()->DrawSprite(*sprite, cell_id, bz, slider_line);
	GetGUI()->DrawSprite(*sprite_button, cell_id, bz, GetButtonRect());
}

void CSlider::UpdateValue()
{
	GUI<float>::SetSetting(this, "value", m_Value);
	ScriptEvent("valuechange");
}

CRect CSlider::GetButtonRect()
{
	float ratio = m_MaxValue > m_MinValue ? (m_Value - m_MinValue) / (m_MaxValue - m_MinValue) : 0.0f;
	float x = m_CachedActualSize.left + ratio * (m_CachedActualSize.GetWidth() - m_ButtonSide);
	float y = m_CachedActualSize.top + (m_CachedActualSize.GetHeight() - m_ButtonSide) / 2.0;
	return CRect(x, y, x + m_ButtonSide, y + m_ButtonSide);
}
