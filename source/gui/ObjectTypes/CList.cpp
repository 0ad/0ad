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
#include "gui/CGUIScrollBarVertical.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SettingTypes/CGUIList.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/timer.h"

const CStr CList::EventNameSelectionChange = "SelectionChange";
const CStr CList::EventNameHoverChange = "HoverChange";
const CStr CList::EventNameMouseLeftClickItem = "MouseLeftClickItem";
const CStr CList::EventNameMouseLeftDoubleClickItem = "MouseLeftDoubleClickItem";

CList::CList(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  IGUIScrollBarOwner(*static_cast<IGUIObject*>(this)),
	  m_Modified(false),
	  m_PrevSelectedItem(-1),
	  m_LastItemClickTime(0),
	  m_BufferZone(),
	  m_Font(),
	  m_ScrollBar(),
	  m_ScrollBarStyle(),
	  m_SoundDisabled(),
	  m_SoundSelected(),
	  m_Sprite(),
	  m_SpriteSelectArea(),
	  m_CellID(),
	  m_TextAlign(),
	  m_TextColor(),
	  m_TextColorSelected(),
	  m_Selected(),
	  m_AutoScroll(),
	  m_Hovered(),
	  m_List(),
	  m_ListData()
{
	RegisterSetting("buffer_zone", m_BufferZone);
	RegisterSetting("font", m_Font);
	RegisterSetting("scrollbar", m_ScrollBar);
	RegisterSetting("scrollbar_style", m_ScrollBarStyle);
	RegisterSetting("sound_disabled", m_SoundDisabled);
	RegisterSetting("sound_selected", m_SoundSelected);
	RegisterSetting("sprite", m_Sprite);
	// Add sprite_disabled! TODO
	RegisterSetting("sprite_selectarea", m_SpriteSelectArea);
	RegisterSetting("cell_id", m_CellID);
	RegisterSetting("text_align", m_TextAlign);
	RegisterSetting("textcolor", m_TextColor);
	RegisterSetting("textcolor_selected", m_TextColorSelected);
	RegisterSetting("selected", m_Selected); // Index selected. -1 is none.
	RegisterSetting("auto_scroll", m_AutoScroll);
	RegisterSetting("hovered", m_Hovered);
	// Each list item has both a name (in 'list') and an associated data string (in 'list_data')
	RegisterSetting("list", m_List);
	RegisterSetting("list_data", m_ListData);

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

	m_ItemsYPositions.resize(m_List.m_Items.size() + 1);

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	m_GeneratedTexts.clear();

	float width = GetListRect().GetWidth();
	// remove scrollbar if applicable
	if (m_ScrollBar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	// Generate texts
	float buffered_y = 0.f;

	for (size_t i = 0; i < m_List.m_Items.size(); ++i)
	{
		CGUIText* text;

		if (!m_List.m_Items[i].GetOriginalString().empty())
			text = &AddText(m_List.m_Items[i], m_Font, width, m_BufferZone);
		else
		{
			// Minimum height of a space character of the current font size
			CGUIString align_string;
			align_string.SetValue(L" ");
			text = &AddText(align_string, m_Font, width, m_BufferZone);
		}

		m_ItemsYPositions[i] = buffered_y;
		buffered_y += text->GetSize().cy;
	}

	m_ItemsYPositions[m_List.m_Items.size()] = buffered_y;

	// Setup scrollbar
	if (m_ScrollBar)
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

void CList::ResetStates()
{
	IGUIObject::ResetStates();
	IGUIScrollBarOwner::ResetStates();
}

void CList::UpdateCachedSize()
{
	IGUIObject::UpdateCachedSize();
	IGUITextOwner::UpdateCachedSize();
}

void CList::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
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

			if (m_AutoScroll)
				UpdateAutoScroll();

			ScriptEvent(EventNameSelectionChange);
		}

		if (Message.value == "scrollbar")
			SetupText();

		// Update scrollbar
		if (Message.value == "scrollbar_style")
		{
			GetScrollBar(0).SetScrollBarStyle(m_ScrollBarStyle);
			SetupText();
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
	{
		if (!m_Enabled)
		{
			PlaySound(m_SoundDisabled);
			break;
		}

		int hovered = GetHoveredItem();
		if (hovered == -1)
			break;
		SetSetting<i32>("selected", hovered, true);
		UpdateAutoScroll();
		PlaySound(m_SoundSelected);

		if (timer_Time() - m_LastItemClickTime < SELECT_DBLCLICK_RATE && hovered == m_PrevSelectedItem)
			this->SendMouseEvent(GUIM_MOUSE_DBLCLICK_LEFT_ITEM, EventNameMouseLeftDoubleClickItem);
		else
			this->SendMouseEvent(GUIM_MOUSE_PRESS_LEFT_ITEM, EventNameMouseLeftClickItem);

		m_LastItemClickTime = timer_Time();
		m_PrevSelectedItem = hovered;
		break;
	}

	case GUIM_MOUSE_LEAVE:
	{
		if (m_Hovered == -1)
			break;

		SetSetting<i32>("hovered", -1, true);
		ScriptEvent(EventNameHoverChange);
		break;
	}

	case GUIM_MOUSE_OVER:
	{
		int hovered = GetHoveredItem();
		if (hovered == m_Hovered)
			break;

		SetSetting<i32>("hovered", hovered, true);
		ScriptEvent(EventNameHoverChange);
		break;
	}

	case GUIM_LOAD:
	{
		GetScrollBar(0).SetScrollBarStyle(m_ScrollBarStyle);
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
	DrawList(m_Selected, m_Sprite, m_SpriteSelectArea, m_TextColor);
}

void CList::DrawList(const int& selected, const CGUISpriteInstance& sprite, const CGUISpriteInstance& sprite_selectarea, const CGUIColor& textcolor)
{
	float bz = GetBufferedZ();

	// First call draw on ScrollBarOwner
	if (m_ScrollBar)
		IGUIScrollBarOwner::Draw();

	{
		CRect rect = GetListRect();

		m_pGUI.DrawSprite(sprite, m_CellID, bz, rect);

		float scroll = 0.f;
		if (m_ScrollBar)
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

				if (m_ScrollBar)
				{
					// Remove any overlapping area of the scrollbar.
					if (rect_sel.right > GetScrollBar(0).GetOuterRect().left &&
						rect_sel.right <= GetScrollBar(0).GetOuterRect().right)
						rect_sel.right = GetScrollBar(0).GetOuterRect().left;

					if (rect_sel.left >= GetScrollBar(0).GetOuterRect().left &&
						rect_sel.left < GetScrollBar(0).GetOuterRect().right)
						rect_sel.left = GetScrollBar(0).GetOuterRect().right;
				}

				m_pGUI.DrawSprite(sprite_selectarea, m_CellID, bz + 0.05f, rect_sel);
			}
		}

		for (size_t i = 0; i < m_List.m_Items.size(); ++i)
		{
			if (m_ItemsYPositions[i+1] - scroll < 0 ||
				m_ItemsYPositions[i] - scroll > rect.GetHeight())
				continue;

			// Clipping area (we'll have to substract the scrollbar)
			CRect cliparea = GetListRect();

			if (m_ScrollBar)
			{
				if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
					cliparea.right <= GetScrollBar(0).GetOuterRect().right)
					cliparea.right = GetScrollBar(0).GetOuterRect().left;

				if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
					cliparea.left < GetScrollBar(0).GetOuterRect().right)
					cliparea.left = GetScrollBar(0).GetOuterRect().right;
			}

			DrawText(i, textcolor, rect.TopLeft() - CPos(0.f, scroll - m_ItemsYPositions[i]), bz + 0.1f, cliparea);
		}
	}
}

void CList::AddItem(const CStrW& str, const CStrW& data)
{
	CGUIString gui_string;
	gui_string.SetValue(str);

	// Do not send a settings-changed message
	m_List.m_Items.push_back(gui_string);

	CGUIString data_string;
	data_string.SetValue(data);

	m_ListData.m_Items.push_back(data_string);

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
	if (m_Selected != static_cast<int>(m_List.m_Items.size()) - 1)
	{
		SetSetting<i32>("selected", m_Selected + 1, true);
		PlaySound(m_SoundSelected);
	}
}

void CList::SelectPrevElement()
{
	if (m_Selected > 0)
	{
		SetSetting<i32>("selected", m_Selected - 1, true);
		PlaySound(m_SoundSelected);
	}
}

void CList::SelectFirstElement()
{
	if (m_Selected >= 0)
		SetSetting<i32>("selected", 0, true);
}

void CList::SelectLastElement()
{
	const int index = static_cast<int>(m_List.m_Items.size()) - 1;

	if (m_Selected != index)
		SetSetting<i32>("selected", index, true);
}

void CList::UpdateAutoScroll()
{
	// No scrollbar, no scrolling (at least it's not made to work properly).
	if (!m_ScrollBar || m_Selected < 0 || static_cast<std::size_t>(m_Selected) >= m_ItemsYPositions.size())
		return;

	float scroll = GetScrollBar(0).GetPos();

	// Check upper boundary
	if (m_ItemsYPositions[m_Selected] < scroll)
	{
		GetScrollBar(0).SetPos(m_ItemsYPositions[m_Selected]);
		return; // this means, if it wants to align both up and down at the same time
				//  this will have precedence.
	}

	// Check lower boundary
	CRect rect = GetListRect();
	if (m_ItemsYPositions[m_Selected+1]-rect.GetHeight() > scroll)
		GetScrollBar(0).SetPos(m_ItemsYPositions[m_Selected+1]-rect.GetHeight());
}

int CList::GetHoveredItem()
{
	const float scroll = m_ScrollBar ? GetScrollBar(0).GetPos() : 0.f;

	const CRect& rect = GetListRect();
	CPos mouse = m_pGUI.GetMousePos();
	mouse.y += scroll;

	// Mouse is over scrollbar
	if (m_ScrollBar && GetScrollBar(0).IsVisible() &&
	    mouse.x >= GetScrollBar(0).GetOuterRect().left &&
	    mouse.x <= GetScrollBar(0).GetOuterRect().right)
		return -1;

	for (size_t i = 0; i < m_List.m_Items.size(); ++i)
		if (mouse.y >= rect.top + m_ItemsYPositions[i] &&
		    mouse.y < rect.top + m_ItemsYPositions[i + 1])
			return i;

	return -1;
}
