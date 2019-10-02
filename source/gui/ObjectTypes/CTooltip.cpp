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

#include "CTooltip.h"

#include "gui/CGUI.h"
#include "gui/SettingTypes/CGUIString.h"
#include "gui/CGUIText.h"

#include <algorithm>

CTooltip::CTooltip(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  m_BufferZone(),
	  m_Caption(),
	  m_Font(),
	  m_Sprite(),
	  m_Delay(),
	  m_TextColor(),
	  m_MaxWidth(),
	  m_Offset(),
	  m_Anchor(),
	  m_TextAlign(),
	  m_Independent(),
	  m_MousePos(),
	  m_UseObject(),
	  m_HideObject()
{
	// If the tooltip is an object by itself:
	RegisterSetting("buffer_zone", m_BufferZone);
	RegisterSetting("caption", m_Caption);
	RegisterSetting("font", m_Font);
	RegisterSetting("sprite", m_Sprite);
	RegisterSetting("delay", m_Delay); // in milliseconds
	RegisterSetting("textcolor", m_TextColor);
	RegisterSetting("maxwidth", m_MaxWidth);
	RegisterSetting("offset", m_Offset);
	RegisterSetting("anchor", m_Anchor);
	RegisterSetting("text_align", m_TextAlign);
	// This is used for tooltips that are hidden/revealed manually by scripts, rather than through the standard tooltip display mechanism
	RegisterSetting("independent", m_Independent);
	// Private settings:
	// This is set by GUITooltip
	RegisterSetting("_mousepos", m_MousePos);
	// If the tooltip is just a reference to another object:
	RegisterSetting("use_object", m_UseObject);
	RegisterSetting("hide_object", m_HideObject);

	// Defaults
	SetSetting<i32>("delay", 500, true);
	SetSetting<EVAlign>("anchor", EVAlign_Bottom, true);
	SetSetting<EAlign>("text_align", EAlign_Left, true);

	// Set up a blank piece of text, to be replaced with a more
	// interesting message later
	AddText();
}

CTooltip::~CTooltip()
{
}

void CTooltip::SetupText()
{
	ENSURE(m_GeneratedTexts.size() == 1);

	m_GeneratedTexts[0] = CGUIText(m_pGUI, m_Caption, m_Font, m_MaxWidth, m_BufferZone, this);

	// Position the tooltip relative to the mouse:

	const CPos& mousepos = m_Independent ? m_pGUI.GetMousePos() : m_MousePos;

	float textwidth = m_GeneratedTexts[0].GetSize().cx;
	float textheight = m_GeneratedTexts[0].GetSize().cy;

	CGUISize size;
	size.pixel.left = mousepos.x + m_Offset.x;
	size.pixel.right = size.pixel.left + textwidth;

	switch (m_Anchor)
	{
	case EVAlign_Top:
		size.pixel.top = mousepos.y + m_Offset.y;
		size.pixel.bottom = size.pixel.top + textheight;
		break;
	case EVAlign_Bottom:
		size.pixel.bottom = mousepos.y + m_Offset.y;
		size.pixel.top = size.pixel.bottom - textheight;
		break;
	case EVAlign_Center:
		size.pixel.top = mousepos.y + m_Offset.y - textheight/2.f;
		size.pixel.bottom = size.pixel.top + textwidth;
		break;
	default:
		debug_warn(L"Invalid EVAlign!");
	}


	// Reposition the tooltip if it's falling off the screen:

	extern int g_xres, g_yres;
	extern float g_GuiScale;
	float screenw = g_xres / g_GuiScale;
	float screenh = g_yres / g_GuiScale;

	if (size.pixel.top < 0.f)
		size.pixel.bottom -= size.pixel.top, size.pixel.top = 0.f;
	else if (size.pixel.bottom > screenh)
		size.pixel.top -= (size.pixel.bottom-screenh), size.pixel.bottom = screenh;

	if (size.pixel.left < 0.f)
		size.pixel.right -= size.pixel.left, size.pixel.left = 0.f;
	else if (size.pixel.right > screenw)
		size.pixel.left -= (size.pixel.right-screenw), size.pixel.right = screenw;

	SetSetting<CGUISize>("size", size, true);
}

void CTooltip::UpdateCachedSize()
{
	IGUIObject::UpdateCachedSize();
	IGUITextOwner::UpdateCachedSize();
}

void CTooltip::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUITextOwner::HandleMessage(Message);
}

void CTooltip::Draw()
{
	float z = 900.f; // TODO: Find a nicer way of putting the tooltip on top of everything else

	// Normally IGUITextOwner will handle this updating but since SetupText can modify the position
	// we need to call it now *before* we do the rest of the drawing
	if (!m_GeneratedTextsValid)
	{
		SetupText();
		m_GeneratedTextsValid = true;
	}

	m_pGUI.DrawSprite(m_Sprite, 0, z, m_CachedActualSize);

	DrawText(0, m_TextColor, m_CachedActualSize.TopLeft(), z + 0.1f);
}
