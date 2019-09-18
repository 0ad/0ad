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

#include "gui/CGUI.h"
#include "gui/GUI.h"
#include "lib/ogl.h"

CSlider::CSlider(CGUI& pGUI)
	: IGUIObject(pGUI), m_IsPressed(false), m_ButtonSide(0)
{
	AddSetting<float>("value");
	AddSetting<float>("min_value");
	AddSetting<float>("max_value");
	AddSetting<i32>("cell_id");
	AddSetting<CGUISpriteInstance>("sprite");
	AddSetting<CGUISpriteInstance>("sprite_bar");
	AddSetting<float>("button_width");

	m_Value = GetSetting<float>("value");
	m_MinValue = GetSetting<float>("min_value");
	m_MaxValue = GetSetting<float>("max_value");
	m_ButtonSide = GetSetting<float>("button_width");

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
		m_Value = GetSetting<float>("value");
		m_MinValue = GetSetting<float>("min_value");
		m_MaxValue = GetSetting<float>("max_value");
		m_ButtonSide = GetSetting<float>("button_width");

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
		m_Mouse = m_pGUI.GetMousePos();
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
		if (!IsMouseOver())
			m_IsPressed = false;
		if (m_IsPressed)
		{
			float difference = float(m_pGUI.GetMousePos().x - m_Mouse.x) * GetSliderRatio();
			m_Mouse = m_pGUI.GetMousePos();
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
	CGUISpriteInstance& sprite = GetSetting<CGUISpriteInstance>("sprite_bar");
	CGUISpriteInstance& sprite_button = GetSetting<CGUISpriteInstance>("sprite");
	const int cell_id = GetSetting<i32>("cell_id");

	CRect slider_line(m_CachedActualSize);
	slider_line.left += m_ButtonSide / 2.0f;
	slider_line.right -= m_ButtonSide / 2.0f;
	float bz = GetBufferedZ();
	m_pGUI.DrawSprite(sprite, cell_id, bz, slider_line);
	m_pGUI.DrawSprite(sprite_button, cell_id, bz, GetButtonRect());
}

void CSlider::UpdateValue()
{
	SetSetting<float>("value", m_Value, true);
	ScriptEvent("valuechange");
}

CRect CSlider::GetButtonRect() const
{
	float ratio = m_MaxValue > m_MinValue ? (m_Value - m_MinValue) / (m_MaxValue - m_MinValue) : 0.0f;
	float x = m_CachedActualSize.left + ratio * (m_CachedActualSize.GetWidth() - m_ButtonSide);
	float y = m_CachedActualSize.top + (m_CachedActualSize.GetHeight() - m_ButtonSide) / 2.0;
	return CRect(x, y, x + m_ButtonSide, y + m_ButtonSide);
}
