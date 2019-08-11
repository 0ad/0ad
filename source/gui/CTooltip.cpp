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
#include "CGUI.h"

#include <algorithm>

CTooltip::CTooltip(CGUI* pGUI)
	: IGUIObject(pGUI), IGUITextOwner(pGUI)
{
	// If the tooltip is an object by itself:
	AddSetting<float>("buffer_zone");
	AddSetting<CGUIString>("caption");
	AddSetting<CStrW>("font");
	AddSetting<CGUISpriteInstance>("sprite");
	AddSetting<int>("delay");
	AddSetting<CGUIColor>("textcolor");
	AddSetting<float>("maxwidth");
	AddSetting<CPos>("offset");
	AddSetting<EVAlign>("anchor");
	AddSetting<EAlign>("text_align");
	// This is used for tooltips that are hidden/revealed manually by scripts, rather than through the standard tooltip display mechanism
	AddSetting<bool>("independent");

	// If the tooltip is just a reference to another object:
	AddSetting<CStr>("use_object");
	AddSetting<bool>("hide_object");

	// Private settings:
	// This is set by GUITooltip
	AddSetting<CPos>("_mousepos");

	// Defaults
	GUI<int>::SetSetting(this, "delay", 500);
	GUI<EVAlign>::SetSetting(this, "anchor", EVAlign_Bottom);
	GUI<EAlign>::SetSetting(this, "text_align", EAlign_Left);

	// Set up a blank piece of text, to be replaced with a more
	// interesting message later
	AddText(new SGUIText());
}

CTooltip::~CTooltip()
{
}

void CTooltip::SetupText()
{
	ENSURE(m_GeneratedTexts.size() == 1);

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		font = L"default";

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	CGUIString* caption = nullptr;
	GUI<CGUIString>::GetSettingPointer(this, "caption", caption);

	float max_width = 0.f;
	GUI<float>::GetSetting(this, "maxwidth", max_width);

	*m_GeneratedTexts[0] = GetGUI()->GenerateText(*caption, font, max_width, buffer_zone, this);


	// Position the tooltip relative to the mouse:

	CPos mousepos, offset;
	EVAlign anchor;
	bool independent;

	GUI<bool>::GetSetting(this, "independent", independent);
	if (independent)
		mousepos = m_pGUI->GetMousePos();
	else
		GUI<CPos>::GetSetting(this, "_mousepos", mousepos);

	GUI<CPos>::GetSetting(this, "offset", offset);
	GUI<EVAlign>::GetSetting(this, "anchor", anchor);

	float textwidth = m_GeneratedTexts[0]->m_Size.cx;
	float textheight = m_GeneratedTexts[0]->m_Size.cy;

	CClientArea size;
	size.pixel.left = mousepos.x + offset.x;
	size.pixel.right = size.pixel.left + textwidth;
	switch (anchor)
	{
	case EVAlign_Top:
		size.pixel.top = mousepos.y + offset.y;
		size.pixel.bottom = size.pixel.top + textheight;
		break;
	case EVAlign_Bottom:
		size.pixel.bottom = mousepos.y + offset.y;
		size.pixel.top = size.pixel.bottom - textheight;
		break;
	case EVAlign_Center:
		size.pixel.top = mousepos.y + offset.y - textheight/2.f;
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

	GUI<CClientArea>::SetSetting(this, "size", size);
}

void CTooltip::HandleMessage(SGUIMessage& Message)
{
	IGUITextOwner::HandleMessage(Message);
}

void CTooltip::Draw()
{
	float z = 900.f; // TODO: Find a nicer way of putting the tooltip on top of everything else

	CGUISpriteInstance* sprite;
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);

	// Normally IGUITextOwner will handle this updating but since SetupText can modify the position
	// we need to call it now *before* we do the rest of the drawing
	if (!m_GeneratedTextsValid)
	{
		SetupText();
		m_GeneratedTextsValid = true;
	}

	GetGUI()->DrawSprite(*sprite, 0, z, m_CachedActualSize);

	CGUIColor color;
	GUI<CGUIColor>::GetSetting(this, "textcolor", color);

	DrawText(0, color, m_CachedActualSize.TopLeft(), z+0.1f);
}
