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

#include "CText.h"

#include "gui/CGUI.h"
#include "gui/CGUIScrollBarVertical.h"
#include "gui/GUI.h"
#include "lib/ogl.h"

CText::CText(CGUI& pGUI)
	: IGUIObject(pGUI), IGUIScrollBarOwner(pGUI), IGUITextOwner(pGUI)
{
	AddSetting<float>("buffer_zone");
	AddSetting<CGUIString>("caption");
	AddSetting<i32>("cell_id");
	AddSetting<bool>("clip");
	AddSetting<CStrW>("font");
	AddSetting<bool>("scrollbar");
	AddSetting<CStr>("scrollbar_style");
	AddSetting<bool>("scroll_bottom");
	AddSetting<bool>("scroll_top");
	AddSetting<CGUISpriteInstance>("sprite");
	AddSetting<EAlign>("text_align");
	AddSetting<EVAlign>("text_valign");
	AddSetting<CGUIColor>("textcolor");
	AddSetting<CGUIColor>("textcolor_disabled");
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");

	// Private settings
	AddSetting<CStrW>("_icon_tooltip");
	AddSetting<CStr>("_icon_tooltip_style");

	//SetSetting<bool>("ghost", true, true);
	SetSetting<bool>("scrollbar", false, true);
	SetSetting<bool>("clip", true, true);

	// Add scroll-bar
	CGUIScrollBarVertical* bar = new CGUIScrollBarVertical(pGUI);
	bar->SetRightAligned(true);
	AddScrollBar(bar);

	// Add text
	AddText();
}

CText::~CText()
{
}

void CText::SetupText()
{
	if (m_GeneratedTexts.empty())
		return;

	const bool scrollbar = GetSetting<bool>("scrollbar");

	float width = m_CachedActualSize.GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	const CGUIString& caption = GetSetting<CGUIString>("caption");
	const CStrW& font = GetSetting<CStrW>("font");
	const float buffer_zone = GetSetting<float>("buffer_zone");

	m_GeneratedTexts[0] = CGUIText(m_pGUI, caption, font, width, buffer_zone, this);

	if (!scrollbar)
		CalculateTextPosition(m_CachedActualSize, m_TextPos, m_GeneratedTexts[0]);

	// Setup scrollbar
	if (scrollbar)
	{
		const bool scroll_bottom = GetSetting<bool>("scroll_bottom");
		const bool scroll_top = GetSetting<bool>("scroll_top");

		// If we are currently scrolled to the bottom of the text,
		// then add more lines of text, update the scrollbar so we
		// stick to the bottom.
		// (Use 1.5px delta so this triggers the first time caption is set)
		bool bottom = false;
		if (scroll_bottom && GetScrollBar(0).GetPos() > GetScrollBar(0).GetMaxPos() - 1.5f)
			bottom = true;

		GetScrollBar(0).SetScrollRange(m_GeneratedTexts[0].GetSize().cy);
		GetScrollBar(0).SetScrollSpace(m_CachedActualSize.GetHeight());

		GetScrollBar(0).SetX(m_CachedActualSize.right);
		GetScrollBar(0).SetY(m_CachedActualSize.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(m_CachedActualSize.bottom - m_CachedActualSize.top);

		if (bottom)
			GetScrollBar(0).SetPos(GetScrollBar(0).GetMaxPos());
		if (scroll_top)
			GetScrollBar(0).SetPos(0.0f);
	}
}

void CText::HandleMessage(SGUIMessage& Message)
{
	IGUIScrollBarOwner::HandleMessage(Message);
	//IGUITextOwner::HandleMessage(Message); <== placed it after the switch instead!

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		if (Message.value == "scrollbar")
			SetupText();

		// Update scrollbar
		if (Message.value == "scrollbar_style")
		{
			GetScrollBar(0).SetScrollBarStyle(GetSetting<CStr>(Message.value));
			SetupText();
		}

		break;

	case GUIM_MOUSE_WHEEL_DOWN:
	{
		GetScrollBar(0).ScrollPlus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		SGUIMessage msg(GUIM_MOUSE_MOTION);
		HandleMessage(msg);
		break;
	}
	case GUIM_MOUSE_WHEEL_UP:
	{
		GetScrollBar(0).ScrollMinus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		SGUIMessage msg(GUIM_MOUSE_MOTION);
		HandleMessage(msg);
		break;
	}
	case GUIM_LOAD:
	{
		GetScrollBar(0).SetX(m_CachedActualSize.right);
		GetScrollBar(0).SetY(m_CachedActualSize.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(m_CachedActualSize.bottom - m_CachedActualSize.top);
		GetScrollBar(0).SetScrollBarStyle(GetSetting<CStr>("scrollbar_style"));
		break;
	}

	default:
		break;
	}

	IGUITextOwner::HandleMessage(Message);
}

void CText::Draw()
{
	float bz = GetBufferedZ();

	const bool scrollbar = GetSetting<bool>("scrollbar");

	if (scrollbar)
		IGUIScrollBarOwner::Draw();

	CGUISpriteInstance& sprite = GetSetting<CGUISpriteInstance>("sprite");
	const int cell_id = GetSetting<i32>("cell_id");
	const bool clip = GetSetting<bool>("clip");

	m_pGUI.DrawSprite(sprite, cell_id, bz, m_CachedActualSize);

	float scroll = 0.f;
	if (scrollbar)
		scroll = GetScrollBar(0).GetPos();

	// Clipping area (we'll have to subtract the scrollbar)
	CRect cliparea;
	if (clip)
	{
		cliparea = m_CachedActualSize;

		if (scrollbar)
		{
			// subtract scrollbar from cliparea
			if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
			    cliparea.right <= GetScrollBar(0).GetOuterRect().right)
				cliparea.right = GetScrollBar(0).GetOuterRect().left;

			if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
			    cliparea.left < GetScrollBar(0).GetOuterRect().right)
				cliparea.left = GetScrollBar(0).GetOuterRect().right;
		}
	}

	const bool enabled = GetSetting<bool>("enabled");
	const CGUIColor& color = GetSetting<CGUIColor>(enabled ? "textcolor" : "textcolor_disabled");

	if (scrollbar)
		DrawText(0, color, m_CachedActualSize.TopLeft() - CPos(0.f, scroll), bz + 0.1f, cliparea);
	else
		DrawText(0, color, m_TextPos, bz + 0.1f, cliparea);
}

bool CText::MouseOverIcon()
{
	for (const CGUIText& guitext : m_GeneratedTexts)
		for (const CGUIText::SSpriteCall& spritecall : guitext.GetSpriteCalls())
		{
			// Check mouse over sprite
			if (!spritecall.m_Area.PointInside(m_pGUI.GetMousePos() - m_CachedActualSize.TopLeft()))
				continue;

			// If tooltip exists, set the property
			if (!spritecall.m_Tooltip.empty())
			{
				SetSettingFromString("_icon_tooltip_style", spritecall.m_TooltipStyle, true);
				SetSettingFromString("_icon_tooltip", spritecall.m_Tooltip, true);
			}

			return true;
		}

	return false;
}
