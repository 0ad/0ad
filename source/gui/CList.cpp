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

#include "CList.h"

#include "gui/CGUI.h"
#include "gui/CGUIColor.h"
#include "gui/CGUIScrollBarVertical.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/timer.h"

CList::CList(CGUI& pGUI)
	: IGUIObject(pGUI), IGUITextOwner(pGUI), IGUIScrollBarOwner(pGUI),
	  m_Modified(false), m_PrevSelectedItem(-1), m_LastItemClickTime(0)
{
	// Add sprite_disabled! TODO
	AddSetting<float>("buffer_zone");
	AddSetting<CStrW>("font");
	AddSetting<bool>("scrollbar");
	AddSetting<CStr>("scrollbar_style");
	AddSetting<CStrW>("sound_disabled");
	AddSetting<CStrW>("sound_selected");
	AddSetting<CGUISpriteInstance>("sprite");
	AddSetting<CGUISpriteInstance>("sprite_selectarea");
	AddSetting<i32>("cell_id");
	AddSetting<EAlign>("text_align");
	AddSetting<CGUIColor>("textcolor");
	AddSetting<CGUIColor>("textcolor_selected");
	AddSetting<i32>("selected");	// Index selected. -1 is none.
	AddSetting<bool>("auto_scroll");
	AddSetting<i32>("hovered");
	AddSetting<CStrW>("tooltip");
	AddSetting<CStr>("tooltip_style");

	// Each list item has both a name (in 'list') and an associated data string (in 'list_data')
	AddSetting<CGUIList>("list");
	AddSetting<CGUIList>("list_data");

	SetSetting<bool>("scrollbar", false, true);
	SetSetting<i32>("selected", -1, true);
	SetSetting<i32>("hovered", -1, true);
	SetSetting<bool>("auto_scroll", false, true);

	// Add scroll-bar
	CGUIScrollBarVertical* bar = new CGUIScrollBarVertical(pGUI);
	bar->SetRightAligned(true);
	AddScrollBar(bar);
}

CList::~CList()
{
}

void CList::SetupText()
{
	m_Modified = true;
	const CGUIList& pList = GetSetting<CGUIList>("list");

	//ENSURE(m_GeneratedTexts.size()>=1);

	m_ItemsYPositions.resize(pList.m_Items.size() + 1);

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	m_GeneratedTexts.clear();

	const CStrW& font = GetSetting<CStrW>("font");

	const bool scrollbar = GetSetting<bool>("scrollbar");

	float width = GetListRect().GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	const float buffer_zone = GetSetting<float>("buffer_zone");

	// Generate texts
	float buffered_y = 0.f;

	for (size_t i = 0; i < pList.m_Items.size(); ++i)
	{
		CGUIText* text;

		if (!pList.m_Items[i].GetOriginalString().empty())
			text = &AddText(pList.m_Items[i], font, width, buffer_zone, this);
		else
		{
			// Minimum height of a space character of the current font size
			CGUIString align_string;
			align_string.SetValue(L" ");
			text = &AddText(align_string, font, width, buffer_zone, this);
		}

		m_ItemsYPositions[i] = buffered_y;
		buffered_y += text->GetSize().cy;
	}

	m_ItemsYPositions[pList.m_Items.size()] = buffered_y;

	// Setup scrollbar
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange(m_ItemsYPositions.back());
		GetScrollBar(0).SetScrollSpace(GetListRect().GetHeight());

		CRect rect = GetListRect();
		GetScrollBar(0).SetX(rect.right);
		GetScrollBar(0).SetY(rect.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(rect.bottom - rect.top);
	}
}

void CList::HandleMessage(SGUIMessage& Message)
{
	IGUIScrollBarOwner::HandleMessage(Message);
	//IGUITextOwner::HandleMessage(Message); <== placed it after the switch instead!

	m_Modified = false;
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		if (Message.value == "list")
			SetupText();

		// If selected is changed, call "SelectionChange"
		if (Message.value == "selected")
		{
			// TODO: Check range

			if (GetSetting<bool>("auto_scroll"))
				UpdateAutoScroll();

			// TODO only works if lower-case, shouldn't it be made case sensitive instead?
			ScriptEvent("selectionchange");
		}

		if (Message.value == "scrollbar")
			SetupText();

		// Update scrollbar
		if (Message.value == "scrollbar_style")
		{
			GetScrollBar(0).SetScrollBarStyle(GetSetting<CStr>(Message.value));
			SetupText();
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
	{
		if (!GetSetting<bool>("enabled"))
		{
			PlaySound("sound_disabled");
			break;
		}

		int hovered = GetHoveredItem();
		if (hovered == -1)
			break;
		SetSetting<i32>("selected", hovered, true);
		UpdateAutoScroll();
		PlaySound("sound_selected");

		if (timer_Time() - m_LastItemClickTime < SELECT_DBLCLICK_RATE && hovered == m_PrevSelectedItem)
			this->SendEvent(GUIM_MOUSE_DBLCLICK_LEFT_ITEM, "mouseleftdoubleclickitem");
		else
			this->SendEvent(GUIM_MOUSE_PRESS_LEFT_ITEM, "mouseleftclickitem");

		m_LastItemClickTime = timer_Time();
		m_PrevSelectedItem = hovered;
		break;
	}

	case GUIM_MOUSE_LEAVE:
	{
		if (GetSetting<i32>("hovered") == -1)
			break;

		SetSetting<i32>("hovered", -1, true);
		ScriptEvent("hoverchange");
		break;
	}

	case GUIM_MOUSE_OVER:
	{
		int hovered = GetHoveredItem();
		if (hovered == GetSetting<i32>("hovered"))
			break;

		SetSetting<i32>("hovered", hovered, true);
		ScriptEvent("hoverchange");
		break;
	}

	case GUIM_LOAD:
	{
		GetScrollBar(0).SetScrollBarStyle(GetSetting<CStr>("scrollbar_style"));
		break;
	}

	default:
		break;
	}

	IGUITextOwner::HandleMessage(Message);
}

InReaction CList::ManuallyHandleEvent(const SDL_Event_* ev)
{
	InReaction result = IN_PASS;

	if (ev->ev.type == SDL_KEYDOWN)
	{
		int szChar = ev->ev.key.keysym.sym;

		switch (szChar)
		{
		case SDLK_HOME:
			SelectFirstElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_END:
			SelectLastElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_UP:
			SelectPrevElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_DOWN:
			SelectNextElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_PAGEUP:
			GetScrollBar(0).ScrollMinusPlenty();
			result = IN_HANDLED;
			break;

		case SDLK_PAGEDOWN:
			GetScrollBar(0).ScrollPlusPlenty();
			result = IN_HANDLED;
			break;

		default: // Do nothing
			result = IN_PASS;
		}
	}

	return result;
}

void CList::Draw()
{
	DrawList(GetSetting<i32>("selected"), "sprite", "sprite_selectarea", "textcolor");
}

void CList::DrawList(const int& selected, const CStr& _sprite, const CStr& _sprite_selected, const CStr& _textcolor)
{
	float bz = GetBufferedZ();

	// First call draw on ScrollBarOwner
	const bool scrollbar = GetSetting<bool>("scrollbar");

	if (scrollbar)
		IGUIScrollBarOwner::Draw();

	{
		CRect rect = GetListRect();

		CGUISpriteInstance& sprite = GetSetting<CGUISpriteInstance>(_sprite);
		CGUISpriteInstance& sprite_selectarea = GetSetting<CGUISpriteInstance>(_sprite_selected);

		const int cell_id = GetSetting<i32>("cell_id");
		m_pGUI.DrawSprite(sprite, cell_id, bz, rect);

		float scroll = 0.f;
		if (scrollbar)
			scroll = GetScrollBar(0).GetPos();

		if (selected >= 0 && selected+1 < (int)m_ItemsYPositions.size())
		{
			// Get rectangle of selection:
			CRect rect_sel(rect.left, rect.top + m_ItemsYPositions[selected] - scroll,
					       rect.right, rect.top + m_ItemsYPositions[selected+1] - scroll);

			if (rect_sel.top <= rect.bottom &&
				rect_sel.bottom >= rect.top)
			{
				if (rect_sel.bottom > rect.bottom)
					rect_sel.bottom = rect.bottom;
				if (rect_sel.top < rect.top)
					rect_sel.top = rect.top;

				if (scrollbar)
				{
					// Remove any overlapping area of the scrollbar.
					if (rect_sel.right > GetScrollBar(0).GetOuterRect().left &&
						rect_sel.right <= GetScrollBar(0).GetOuterRect().right)
						rect_sel.right = GetScrollBar(0).GetOuterRect().left;

					if (rect_sel.left >= GetScrollBar(0).GetOuterRect().left &&
						rect_sel.left < GetScrollBar(0).GetOuterRect().right)
						rect_sel.left = GetScrollBar(0).GetOuterRect().right;
				}

				m_pGUI.DrawSprite(sprite_selectarea, cell_id, bz+0.05f, rect_sel);
			}
		}

		const CGUIList& pList = GetSetting<CGUIList>("list");
		const CGUIColor& color = GetSetting<CGUIColor>(_textcolor);

		for (size_t i = 0; i < pList.m_Items.size(); ++i)
		{
			if (m_ItemsYPositions[i+1] - scroll < 0 ||
				m_ItemsYPositions[i] - scroll > rect.GetHeight())
				continue;

			// Clipping area (we'll have to substract the scrollbar)
			CRect cliparea = GetListRect();

			if (scrollbar)
			{
				if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
					cliparea.right <= GetScrollBar(0).GetOuterRect().right)
					cliparea.right = GetScrollBar(0).GetOuterRect().left;

				if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
					cliparea.left < GetScrollBar(0).GetOuterRect().right)
					cliparea.left = GetScrollBar(0).GetOuterRect().right;
			}

			DrawText(i, color, rect.TopLeft() - CPos(0.f, scroll - m_ItemsYPositions[i]), bz + 0.1f, cliparea);
		}
	}
}

void CList::AddItem(const CStrW& str, const CStrW& data)
{
	CGUIString gui_string;
	gui_string.SetValue(str);

	// Do not send a settings-changed message
	CGUIList& pList = GetSetting<CGUIList>("list");
	pList.m_Items.push_back(gui_string);

	CGUIString data_string;
	data_string.SetValue(data);
	CGUIList& pListData = GetSetting<CGUIList>("list_data");
	pListData.m_Items.push_back(data_string);

	// TODO Temp
	SetupText();
}

bool CList::HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile)
{
	int elmt_item = pFile->GetElementID("item");

	if (child.GetNodeName() == elmt_item)
	{
		AddItem(child.GetText().FromUTF8(), child.GetText().FromUTF8());
		return true;
	}

	return false;
}

void CList::SelectNextElement()
{
	int selected = GetSetting<i32>("selected");

	const CGUIList& pList = GetSetting<CGUIList>("list");

	if (selected != static_cast<int>(pList.m_Items.size()) - 1)
	{
		++selected;
		SetSetting<i32>("selected", selected, true);
		PlaySound("sound_selected");
	}
}

void CList::SelectPrevElement()
{
	int selected = GetSetting<i32>("selected");

	if (selected > 0)
	{
		--selected;
		SetSetting<i32>("selected", selected, true);
		PlaySound("sound_selected");
	}
}

void CList::SelectFirstElement()
{
	if (GetSetting<i32>("selected") >= 0)
		SetSetting<i32>("selected", 0, true);
}

void CList::SelectLastElement()
{
	const CGUIList& pList = GetSetting<CGUIList>("list");
	const int index = static_cast<int>(pList.m_Items.size()) - 1;

	if (GetSetting<i32>("selected") != index)
		SetSetting<i32>("selected", index, true);
}

void CList::UpdateAutoScroll()
{
	const int selected = GetSetting<i32>("selected");
	const bool scrollbar = GetSetting<bool>("scrollbar");

	// No scrollbar, no scrolling (at least it's not made to work properly).
	if (!scrollbar || selected < 0 || static_cast<std::size_t>(selected) >= m_ItemsYPositions.size())
		return;

	float scroll = GetScrollBar(0).GetPos();

	// Check upper boundary
	if (m_ItemsYPositions[selected] < scroll)
	{
		GetScrollBar(0).SetPos(m_ItemsYPositions[selected]);
		return; // this means, if it wants to align both up and down at the same time
				//  this will have precedence.
	}

	// Check lower boundary
	CRect rect = GetListRect();
	if (m_ItemsYPositions[selected+1]-rect.GetHeight() > scroll)
		GetScrollBar(0).SetPos(m_ItemsYPositions[selected+1]-rect.GetHeight());
}

int CList::GetHoveredItem()
{
	const bool scrollbar = GetSetting<bool>("scrollbar");
	const float scroll = scrollbar ? GetScrollBar(0).GetPos() : 0.f;

	const CRect& rect = GetListRect();
	CPos mouse = m_pGUI.GetMousePos();
	mouse.y += scroll;

	// Mouse is over scrollbar
	if (scrollbar && GetScrollBar(0).IsVisible() &&
	    mouse.x >= GetScrollBar(0).GetOuterRect().left &&
	    mouse.x <= GetScrollBar(0).GetOuterRect().right)
		return -1;

	const CGUIList& pList = GetSetting<CGUIList>("list");
	for (size_t i = 0; i < pList.m_Items.size(); ++i)
		if (mouse.y >= rect.top + m_ItemsYPositions[i] &&
		    mouse.y < rect.top + m_ItemsYPositions[i + 1])
			return i;

	return -1;
}
