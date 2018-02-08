/* Copyright (C) 2015 Wildfire Games.
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

CTooltip::CTooltip()
{
	// If the tooltip is an object by itself:
	AddSetting(GUIST_float,					"buffer_zone");
	AddSetting(GUIST_CGUIString,			"caption");
	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_int,					"delay");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_float,					"maxwidth");
	AddSetting(GUIST_CPos,					"offset");
	AddSetting(GUIST_EVAlign,				"anchor");
	AddSetting(GUIST_EAlign,				"text_align");
	// This is used for tooltips that are hidden/revealed manually by scripts, rather than through the standard tooltip display mechanism
	AddSetting(GUIST_bool,					"independent");

	// If the tooltip is just a reference to another object:
	AddSetting(GUIST_CStr,					"use_object");
	AddSetting(GUIST_bool,					"hide_object");

	// Private settings:
	AddSetting(GUIST_CPos,					"_mousepos");

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
	if (!GetGUI())
		return;

	ENSURE(m_GeneratedTexts.size() == 1);

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		font = L"default";

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	CGUIString caption;
	GUI<CGUIString>::GetSetting(this, "caption", caption);

	float max_width = 0.f;
	GUI<float>::GetSetting(this, "maxwidth", max_width);

	*m_GeneratedTexts[0] = GetGUI()->GenerateText(caption, font, max_width, buffer_zone, this);


	// Position the tooltip relative to the mouse:

	CPos mousepos, offset;
	EVAlign anchor;
	bool independent;

	GUI<bool>::GetSetting(this, "independent", independent);
	if (independent)
		mousepos = GetMousePos();
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
	if (!GetGUI())
		return;

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

	CColor color;
	GUI<CColor>::GetSetting(this, "textcolor", color);

	DrawText(0, color, m_CachedActualSize.TopLeft(), z+0.1f);
}
