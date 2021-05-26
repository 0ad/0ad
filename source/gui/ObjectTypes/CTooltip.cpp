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

#include "CTooltip.h"

#include "gui/CGUI.h"
#include "gui/SettingTypes/CGUIString.h"
#include "gui/CGUIText.h"

#include <algorithm>

CTooltip::CTooltip(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  m_BufferZone(this, "buffer_zone"),
	  m_Caption(this, "caption"),
	  m_Font(this, "font"),
	  m_Sprite(this, "sprite"),
	  m_Delay(this, "delay", 500), // in milliseconds
	  m_TextColor(this, "textcolor"),
	  m_MaxWidth(this, "maxwidth"),
	  m_Offset(this, "offset"),
	  m_Anchor(this, "anchor", EVAlign::BOTTOM),
	  // This is used for tooltips that are hidden/revealed manually by scripts, rather than through the standard tooltip display mechanism
	  m_Independent(this, "independent"),
	  // Private settings:
	  // This is set by GUITooltip
	  m_MousePos(this, "_mousepos"),
	  // If the tooltip is just a reference to another object:
	  m_UseObject(this, "use_object"),
	  m_HideObject(this, "hide_object")
{
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

	m_GeneratedTexts[0] = CGUIText(m_pGUI, m_Caption, m_Font, m_MaxWidth, m_BufferZone, m_TextAlign, this);

	// Position the tooltip relative to the mouse:

	const CVector2D& mousepos = m_Independent ? m_pGUI.GetMousePos() : m_MousePos;

	float textwidth = m_GeneratedTexts[0].GetSize().Width;
	float textheight = m_GeneratedTexts[0].GetSize().Height;

	CGUISize size;
	size.pixel.left = mousepos.X + m_Offset->X;
	size.pixel.right = size.pixel.left + textwidth;

	switch (m_Anchor)
	{
	case EVAlign::TOP:
		size.pixel.top = mousepos.Y + m_Offset->Y;
		size.pixel.bottom = size.pixel.top + textheight;
		break;
	case EVAlign::BOTTOM:
		size.pixel.bottom = mousepos.Y + m_Offset->Y;
		size.pixel.top = size.pixel.bottom - textheight;
		break;
	case EVAlign::CENTER:
		size.pixel.top = mousepos.Y + m_Offset->Y - textheight/2.f;
		size.pixel.bottom = size.pixel.top + textwidth;
		break;
	default:
		debug_warn(L"Invalid EVAlign!");
	}


	// Reposition the tooltip if it's falling off in the GUI window.
	const CSize2D windowSize = m_pGUI.GetWindowSize();

	if (size.pixel.top < 0.f)
	{
		size.pixel.bottom -= size.pixel.top;
		size.pixel.top = 0.f;
	}
	else if (size.pixel.bottom > windowSize.Height)
	{
		size.pixel.top -= size.pixel.bottom - windowSize.Height;
		size.pixel.bottom = windowSize.Height;
	}

	if (size.pixel.left < 0.f)
	{
		size.pixel.right -= size.pixel.left;
		size.pixel.left = 0.f;
	}
	else if (size.pixel.right > windowSize.Width)
	{
		size.pixel.left -= size.pixel.right - windowSize.Width;
		size.pixel.right = windowSize.Width;
	}

	m_Size.Set(size, true);
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
	// Normally IGUITextOwner will handle this updating but since SetupText can modify the position
	// we need to call it now *before* we do the rest of the drawing
	if (!m_GeneratedTextsValid)
	{
		SetupText();
		m_GeneratedTextsValid = true;
	}

	m_pGUI.DrawSprite(m_Sprite, m_CachedActualSize);
	DrawText(0, m_TextColor, m_CachedActualSize.TopLeft());
}

float CTooltip::GetBufferedZ() const
{
	// TODO: Find a nicer way of putting the tooltip on top of everything else.
	return 900.f;
}
